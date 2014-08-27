/*
 * commandline.cpp:
 * Tests for building the command line
 *
 * See LICENCE.md for Copyright information
 */

#include <functional>

#include <gmock/gmock.h>

#include "commandline.h"
#include "instrumentation_tool.h"
#include "test_util.h"
#include "value_type_test.h"

#include "instrumentation_mock.h"

using ::testing::_;
using ::testing::AtLeast;
using ::testing::ElementsAreArray;
using ::testing::IsNull;
using ::testing::Matcher;
using ::testing::ReturnRef;
using ::testing::StrEq;

namespace ycom = yiqi::commandline;
namespace yit = yiqi::instrumentation::tools;
namespace ymock = yiqi::mock;
namespace ymockit = yiqi::mock::instrumentation::tools;
namespace ytest = yiqi::testing;

namespace
{
    static std::string const MockWrapper ("mock");
    static std::string const MockOptions ("--mock");
    static std::string const NilString ("");
    static std::vector <std::string> const NoOptions;

    std::string const MockItem1 ("MockItem1");
    std::string const MockItem2 ("MockItem2");

    typedef ytest::CommandLineArguments CommandLineArguments;

    CommandLineArguments
    GenerateCommandLine (std::vector <std::string> const &args)
    {
        return ytest::GenerateCommandLine (args);
    }
}

class BuildCommandLine :
    public ::testing::Test
{
    public:

        BuildCommandLine ()
        {
            /* We don't care about whether or not these functions are
             * called, we care about how the system under test handles
             * them */
            EXPECT_CALL (instrumentation, WrapperBinary ())
                .Times (AtLeast (0));
            EXPECT_CALL (instrumentation, WrapperOptions ())
                .Times (AtLeast (0));
        }

    protected:

        ymockit::Program instrumentation;
};

TEST_F (BuildCommandLine, ThrowOnNoArgs)
{
    CommandLineArguments originalArgs;
    EXPECT_THROW ({
        ycom::BuildCommandLine (ytest::ArgumentCount (originalArgs),
                                ytest::Arguments (originalArgs),
                                instrumentation);
    },
    std::runtime_error);
}

TEST_F (BuildCommandLine, NoWrapperOrOptionsJustArgvReturnsArgvAt0)
{
    CommandLineArguments originalArgs (GenerateCommandLine (NoOptions));

    ON_CALL (instrumentation, WrapperBinary ())
        .WillByDefault (ReturnRef (NilString));
    ON_CALL (instrumentation, WrapperOptions ())
        .WillByDefault (ReturnRef (NilString));

    auto args (ycom::BuildCommandLine (ytest::ArgumentCount (originalArgs),
                                       ytest::Arguments (originalArgs),
                                       instrumentation));

    Matcher <std::string> matchers[] = { StrEq (ytest::MockProgramName) };

    EXPECT_THAT (args,
                 ElementsAreArray (matchers));
}

TEST_F (BuildCommandLine, ReturnWrapperAndOptions)
{
    CommandLineArguments originalArgs (GenerateCommandLine (NoOptions));

    ON_CALL (instrumentation, WrapperBinary ())
        .WillByDefault (ReturnRef (MockWrapper));
    ON_CALL (instrumentation, WrapperOptions ())
        .WillByDefault (ReturnRef (MockOptions));

    auto args (ycom::BuildCommandLine (ytest::ArgumentCount (originalArgs),
                                       ytest::Arguments (originalArgs),
                                       instrumentation));

    Matcher <std::string> matchers[] =
    {
        StrEq (MockWrapper),
        StrEq (MockOptions),
    StrEq (ytest::MockProgramName)
    };

    EXPECT_THAT (args,
                 ElementsAreArray (matchers));
}

namespace
{
    ycom::CommandArguments const MockArgs =
    {
        MockItem1,
        MockItem2
    };
}

TEST (CommandLine, VectorToArgv)
{
    Matcher <char const *> matchers[] =
    {
        StrEq (MockItem1),
        StrEq (MockItem2),
        _
    };

    ycom::ArgvVector const argv (ycom::StringVectorToArgv (MockArgs));

    EXPECT_THAT (argv,
                 ElementsAreArray (matchers));
}

TEST (CommandLine, VectorIsNullTerminated)
{
    ycom::ArgvVector const argv (ycom::StringVectorToArgv (MockArgs));

    ASSERT_EQ (3UL, argv.size ());
    EXPECT_THAT (argv[2], IsNull ());
}

namespace
{
    static char const * const NonDefaultValuesp[] =
    {
        MockItem1.c_str (),
        NULL
    };
}

namespace yiqi
{
    namespace testing
    {
        template <>
        class ValueTypeConstructor <ycom::NullTermArray>
        {
            public:

                ycom::NullTermArray ConstructWithNonDefaultData ();
                bool HasNonDefaultConstructor ();
                bool Mutate (ycom::NullTermArray &env);
                bool Overloaded ();
        };
    }

    namespace commandline
    {
        std::ostream & operator<< (std::ostream &os, NullTermArray const &env);
    }
}

std::ostream &
ycom::operator<< (std::ostream &os, NullTermArray const &env)
{
    std::string const nullTerminator ("null-terminator");
    auto envVector (ytest::ToVector <char const *> (
                        env.underlyingArray (),
                        env.underlyingArrayLen ()));
    for (size_t i = 0; i < envVector.size (); ++i)
    {
        char const *element = envVector[i];
        os << "element #" << i << " : "
           << (element ? std::string (envVector[i]) :
                         nullTerminator) << " "
           << std::endl;
    }

    return os;
}

namespace
{
    typedef ycom::NullTermArray NullTermArray;
}

ycom::NullTermArray
ytest::ValueTypeConstructor <NullTermArray>::ConstructWithNonDefaultData ()
{
    return ycom::NullTermArray (NonDefaultValuesp);
}

bool
ytest::ValueTypeConstructor <NullTermArray>::HasNonDefaultConstructor ()
{
    return true;
}

bool
ytest::ValueTypeConstructor <NullTermArray>::Mutate (ycom::NullTermArray &e)
{
    e.append (MockItem2);
    return true;
}

bool
ytest::ValueTypeConstructor <NullTermArray>::Overloaded ()
{
    return true;
}

typedef ::testing::Types <NullTermArray> YComValueConformanceTypes;
INSTANTIATE_TYPED_TEST_CASE_P (NullTermArray,
                               ValueTypeConformance,
                               YComValueConformanceTypes);

INSTANTIATE_TYPED_TEST_CASE_P (NullTermArray,
                               ValueTypeConformanceDeathTest,
                               YComValueConformanceTypes);


class NullTermArrayDefault :
    public ::testing::Test
{
    protected:

        ycom::NullTermArray nullTermArray;
};

TEST_F (NullTermArrayDefault, OneElementOnConstruction)
{
    EXPECT_EQ (1UL, nullTermArray.underlyingArrayLen ());
}

TEST_F (NullTermArrayDefault, FirstElementIsNullOnConstruction)
{
    EXPECT_THAT (nullTermArray.underlyingArray ()[0], IsNull ());
}

TEST_F (NullTermArrayDefault, InsertAndUndoInsert)
{
    std::vector <std::string> const vector =
    {
        MockItem1,
        MockItem2,
    };

    nullTermArray.append (vector);
    nullTermArray.eraseAppended (vector);
    ASSERT_EQ (1UL, nullTermArray.underlyingArrayLen ());

    EXPECT_THAT (nullTermArray.underlyingArray ()[0],
                 IsNull ());
}


class NullTermArrayNonDefault :
    public ::testing::Test
{
    public:

        NullTermArrayNonDefault () :
            nullTermArray (NonDefaultValuesp)
        {
        }

    protected:

        ycom::NullTermArray nullTermArray;
};

TEST_F (NullTermArrayNonDefault, RemoveDefaultValue)
{
    nullTermArray.removeAnyMatching ([](char const *str) -> bool {
                                         return strstr (NonDefaultValuesp[0],
                                                        str) != NULL;
                                     });

    ASSERT_EQ (1UL, nullTermArray.underlyingArrayLen ());
    EXPECT_THAT (nullTermArray.underlyingArray ()[0], IsNull ());
}

TEST_F (NullTermArrayNonDefault, TwoElementsOnConstruction)
{
    EXPECT_EQ (2UL, nullTermArray.underlyingArrayLen ());
}

TEST_F (NullTermArrayNonDefault, FirstElementMatchesEnvp)
{
    EXPECT_THAT (nullTermArray.underlyingArray ()[0],
                 StrEq (NonDefaultValuesp[0]));
}

TEST_F (NullTermArrayNonDefault, SecondElementIsNullTerminator)
{
    EXPECT_THAT (nullTermArray.underlyingArray ()[1],
                 IsNull ());
}

TEST_F (NullTermArrayNonDefault, InsertNewBeforeNullTerminator)
{
    nullTermArray.append (MockItem2);
    ASSERT_EQ (3UL, nullTermArray.underlyingArrayLen ());
    EXPECT_THAT (nullTermArray.underlyingArray ()[1],
                 StrEq (MockItem2));
}

TEST_F (NullTermArrayNonDefault, InsertNewVecBeforeNullTerminator)
{
    std::vector <std::string> const vector =
    {
        MockItem1,
        MockItem2,
    };

    nullTermArray.append (vector);
    ASSERT_EQ (4UL, nullTermArray.underlyingArrayLen ());
    EXPECT_THAT (nullTermArray.underlyingArray ()[1],
                 StrEq (MockItem1));
    EXPECT_THAT (nullTermArray.underlyingArray ()[2],
                 StrEq (MockItem2));
}

TEST_F (NullTermArrayNonDefault, InsertAndUndoInsertPartial)
{
    nullTermArray.append (MockItem1);

    std::vector <std::string> const vector =
    {
        MockItem1,
        MockItem2,
    };

    nullTermArray.append (vector);
    nullTermArray.eraseAppended (vector);
    ASSERT_EQ (3UL, nullTermArray.underlyingArrayLen ());

    EXPECT_THAT (nullTermArray.underlyingArray ()[0],
                 StrEq (NonDefaultValuesp[0]));
    EXPECT_THAT (nullTermArray.underlyingArray ()[2],
                 IsNull ());
}

TEST_F (NullTermArrayNonDefault, InsertAndUndoInsert)
{
    std::vector <std::string> const vector =
    {
        MockItem1,
        MockItem2,
    };

    nullTermArray.append (vector);
    nullTermArray.eraseAppended (vector);
    ASSERT_EQ (2UL, nullTermArray.underlyingArrayLen ());

    EXPECT_THAT (nullTermArray.underlyingArray ()[0],
                 StrEq (NonDefaultValuesp[0]));
    EXPECT_THAT (nullTermArray.underlyingArray ()[1],
                 IsNull ());
}

TEST_F (NullTermArrayNonDefault, LastElementIsNullTerminator)
{
    nullTermArray.append (MockItem2);
    ASSERT_EQ (3UL, nullTermArray.underlyingArrayLen ());
    EXPECT_THAT (nullTermArray.underlyingArray ()[2],
                 IsNull ());
}

namespace
{
    std::string const MockerEnvKey ("MOCKER");
    std::string const MockEnvValue ("mock");
}

TEST (nullTermArrayOfEnvironment, InsertNewBeforeNullTerminator)
{
    ycom::NullTermArray environment (NonDefaultValuesp);
    std::string const ExpectedValue (MockerEnvKey + "=" + MockEnvValue);

    ycom::InsertEnvironmentPair (environment,
                                 MockerEnvKey.c_str (),
                                 MockEnvValue.c_str ());
    ASSERT_EQ (3UL, environment.underlyingArrayLen ());
    EXPECT_THAT (environment.underlyingArray ()[1],
                 StrEq (ExpectedValue));
}
