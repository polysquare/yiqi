/*
 * reexecute_through_passthrough.cpp:
 *
 * Test running a sample binary through an
 * executable YIQI_PASSTHROUGH_EXECUTABLE passed by
 * the compiler. Verify by watching the stdout of that
 * executable
 *
 * See LICENCE.md for Copyright information
 */

#include <functional>
#include <iostream>
#include <system_error>

#include <gmock/gmock.h>

#include <boost/noncopyable.hpp>

#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/stream.hpp>

#include <unistd.h>
#include <fcntl.h>

#include <commandline.h>
#include <constants.h>
#include <instrumentation_tools_available.h>
#include <operating_system_wrapper.h>
#include <operating_system_implementation.h>

#include <passthrough_constants.h>

#include <acceptance_tests_config.h>

using ::testing::_;
using ::testing::AllOf;
using ::testing::Contains;
using ::testing::ElementsAreArray;
using ::testing::Matcher;
using ::testing::HasSubstr;

namespace bio = boost::iostreams;
namespace ycom = yiqi::commandline;
namespace yconst = yiqi::constants;
namespace yta = yiqi::testing::acceptance;
namespace ytp = yiqi::testing::passthrough;
namespace ysysunix = yiqi::system::unix;

typedef ysysunix::OperatingSystemWrapper OSWrapper;

namespace
{
    class Pipe
    {
        public:

            Pipe (OSWrapper &userspace)
            {
                int result = userspace.pipe (mPipe);
                assert (result != -1);
            }

            ~Pipe ()
            {
                for (size_t i = 0; i < 2; ++i)
                {
                    if (mPipe[i])
                    {
                        int result = close (mPipe[i]);
                        assert (result != -1);
                    }
                }
            }

            /* Read end descriptor is read-only */
            int ReadEnd ()
            {
                return mPipe[0];
            }

            /* Write end descriptor is writable, we need to close it
             * from other objects */
            int & WriteEnd ()
            {
                return mPipe[1];
            }

        private:

            int mPipe[2];

            Pipe (Pipe const &) = delete;
            Pipe & operator= (Pipe const &) = delete;
    };

    class FileDescriptorBackup
    {
        public:

            FileDescriptorBackup (int fd) :
                mOriginalFd (fd),
                mBackupFd (0)
            {
                mBackupFd = dup (mOriginalFd);
                assert (mBackupFd != -1);
            }

            ~FileDescriptorBackup ()
            {
                /* Redirect backed up fd to old fd location */
                if (mBackupFd)
                {
                    int result = dup2 (mBackupFd, mOriginalFd);
                    assert ("Failed to restore file descriptor " &&
                            result != -1);
                }
            }

        private:

            int mOriginalFd;
            int mBackupFd;

            FileDescriptorBackup (FileDescriptorBackup const &) = delete;

            FileDescriptorBackup &
            operator= (FileDescriptorBackup const &) = delete;

    };

    class RedirectedDescriptor
    {
        public:

            RedirectedDescriptor (int from,
                                  int &to) :
                mToFd (to)
            {
                /* Make 'to' take the old file descriptor's place */
                int result = dup2 (to, from);
                assert (result != -1);
            }

            ~RedirectedDescriptor ()
            {
                if (mToFd)
                {
                    int result = close (mToFd);
                    assert ("Failed to close redirect-to file descriptor" &&
                            result != -1);
                }

                mToFd = 0;
            }

        private:

            int &mToFd;

            RedirectedDescriptor (RedirectedDescriptor const &) = delete;

            RedirectedDescriptor &
            operator= (RedirectedDescriptor const &) = delete;
    };

    pid_t launchBinary (OSWrapper          &userspace,
                        std::string const  &executable,
                        char const * const *argv,
                        char const * const *env,
                        int                &stdoutWriteEnd)
    {
        FileDescriptorBackup stdout (STDOUT_FILENO);

        /* Close the originals once they have been backed up
         * We have to do this here and not in the FileDescriptorBackup
         * constructors because of an order-of-operations issue -
         * namely if we close an original file descriptor name
         * before duplicating another one, then there's a possibility
         * that the duplicated other one will get the same name as
         * the one we just closed, making us unable to restore
         * the closed one properly */

        int closeStdoutResult = close (STDOUT_FILENO);
        assert (closeStdoutResult != -1);

        /* Replace the current process stderr and stdout with the write end
         * of the pipes. Now when someone tries to write to stderr or stdout
         * they'll write to our pipes instead */
        RedirectedDescriptor pipedStdout (STDOUT_FILENO, stdoutWriteEnd);

        /* Fork process, child gets a copy of the pipe write ends
         * - the original pipe write ends will be closed on exec
         * but the duplicated write ends now taking the place of
         * stderr and stdout will not be */
        pid_t child = fork ();
        assert (child != -1);

        /* Child process */
        if (child == 0)
        {
            if (userspace.execve (executable.c_str (),
                                  argv,
                                  env) == -1)
            {
                std::cerr << "execvpe failed with error "
                          << errno
                          << "\n"
                          << " - binary "
                          << executable
                          << std::endl;
                abort ();
            }
        }

        /* The old file descriptors for the stderr and stdout
         * are put back in place, and pipe write ends closed
         * as the child is using them at return */

        return child;
    }

    int launchBinaryAndWaitForReturn (OSWrapper                 &userspace,
                                      std::string const         &executable,
                                      ycom::NullTermArray const &argv,
                                      ycom::NullTermArray const &env,
                                      int                       &stdoutWrite)
    {
        int status = 0;
        pid_t child = launchBinary (userspace,
                                    executable,
                                    argv.underlyingArray (),
                                    env.underlyingArray (),
                                    stdoutWrite);

        do
        {
            /* Wait around for the child to get a signal */
            pid_t waitChild = waitpid (child, &status, 0);
            if (waitChild == child)
            {
                /* If it died unexpectedly, say so */
                if (WIFSIGNALED (status))
                {
                    std::stringstream ss;
                    ss << "child killed by signal "
                       << WTERMSIG (status);
                    throw std::runtime_error (ss.str ());
                }
            }
            else
            {
                /* waitpid () failed */
                throw std::runtime_error (strerror (errno));
            }

            /* Keep going until it exited */
        } while (!WIFEXITED (status) && !WIFSIGNALED (status));

        /* Return the exit code */
        return WEXITSTATUS (status);
    }
}

class ChildOutputTest :
    public ::testing::Test
{
    public:

        ChildOutputTest () :
            userspace (ysysunix::MakeOSWrapper ()),
            env (userspace->environment ()),
            childStdoutPipe (*userspace)
        {
        }

        virtual void SetUp ()
        {
            argv.append (GetExecutable ());
        }

        virtual std::string GetExecutable () const = 0;

    protected:

        std::vector <std::string> GetChildOutput ();
        int LaunchBinaryWithInternalArgv (std::string const &);

        std::unique_ptr <ysysunix::OperatingSystemWrapper> userspace;

        ycom::NullTermArray argv;
        ycom::NullTermArray env;
        std::string         lineBuffer;


        Pipe               childStdoutPipe;
};

class DirectlyExecutePassthrough :
    public ChildOutputTest
{
    public:

        std::string GetExecutable () const
        {
            return yta::passthrough;
        }
};

std::vector <std::string>
ChildOutputTest::GetChildOutput ()
{
    typedef bio::stream_buffer <bio::file_descriptor_source>  ChildStream;
    ChildStream streambuf (childStdoutPipe.ReadEnd (),
                           bio::never_close_handle);

    std::vector <std::string> lines;

    std::istream stream (&streambuf);
    while (stream)
    {
        std::string line;
        std::getline (stream, line);
        if (!line.empty ())
            lines.push_back (line);
    }

    return lines;
}

int
ChildOutputTest::LaunchBinaryWithInternalArgv (std::string const &executable)
{
  return launchBinaryAndWaitForReturn (*userspace,
                                       executable,
                                       argv,
                                       env,
                                       childStdoutPipe.WriteEnd ());
}

TEST_F (DirectlyExecutePassthrough, ExecuteNoThrow)
{
    EXPECT_NO_THROW ({
        LaunchBinaryWithInternalArgv (yta::passthrough);
    });
}

TEST_F (DirectlyExecutePassthrough, ExecuteNoArgsReturns1)
{
    EXPECT_EQ (1,
               LaunchBinaryWithInternalArgv (yta::passthrough));
}

TEST_F (DirectlyExecutePassthrough, ExecuteWithNoExecArgsReturns1)
{
    std::string const MockArgument ("--ARGUMENT");
    argv.append (MockArgument);

    EXPECT_EQ (1,
               LaunchBinaryWithInternalArgv (yta::passthrough));
}

TEST_F (DirectlyExecutePassthrough, ExecuteNoArgsOutputHeader)
{
    LaunchBinaryWithInternalArgv (yta::passthrough);

    std::vector <std::string> output (GetChildOutput ());

    Matcher <std::string> expectedElements[] =
    {
        HasSubstr (ytp::StartupMessage)
    };

    EXPECT_THAT (output, ElementsAreArray (expectedElements));
}

TEST_F (DirectlyExecutePassthrough, ExecuteWithArgsOutputArg)
{
    std::string const MockArgument ("--ARGUMENT");
    argv.append (MockArgument);

    LaunchBinaryWithInternalArgv (yta::passthrough);

    std::vector <std::string> output (GetChildOutput ());

    Matcher <std::string> expectedElements[] =
    {
        _,
        AllOf (HasSubstr (ytp::OptionHeader),
               HasSubstr (MockArgument))
    };

    EXPECT_THAT (output, ElementsAreArray (expectedElements));
}

namespace
{
    std::string const passthroughTool (
        yconst::StringFromTool (yconst::InstrumentationTool::Passthrough));
    std::string const dashdashYiqiToolOption (
        std::string ("--") + yconst::YiqiToolOption);
}


class DirectlyExecuteSimpleTest :
    public ChildOutputTest
{
    public:

        DirectlyExecuteSimpleTest ()
        {
            env.removeAnyMatching ([](char const *str) -> bool {
                                       return strncmp (str,
                                                       "PATH=", 5) == 0;
                                   });
            ycom::InsertEnvironmentPair (env,
                                         "PATH",
                                         yta::pthruPath.c_str ());
        }

        std::string GetExecutable () const
        {
            return yta::simpleTest;
        }
};

TEST_F (DirectlyExecuteSimpleTest, ExecuteNoThrow)
{
    EXPECT_NO_THROW ({
        LaunchBinaryWithInternalArgv (yta::simpleTest);
    });
}

TEST_F (DirectlyExecuteSimpleTest, ExecuteWithToolArgsRunningUnderMsg)
{
    argv.append (dashdashYiqiToolOption);
    argv.append (passthroughTool);
    LaunchBinaryWithInternalArgv (yta::simpleTest);

    std::vector <std::string> output (GetChildOutput ());

    EXPECT_THAT (output,
                 Contains (
                     AllOf (
                         HasSubstr (yconst::YiqiRunningUnderHeader),
                                    HasSubstr (passthroughTool))));
}


TEST_F (DirectlyExecuteSimpleTest, ExecuteWithToolArgsOptionPassthroughMsg)
{
    argv.append (dashdashYiqiToolOption);
    argv.append (passthroughTool);
    LaunchBinaryWithInternalArgv (yta::simpleTest);

    std::vector <std::string> output (GetChildOutput ());

    EXPECT_THAT (output,
                 Contains (HasSubstr (ytp::OptionHeader)));
}
