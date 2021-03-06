/*
 * yiqi_simple_string_replacement_generator.cpp
 *
 * Tool used to parse an input tools_declarations.h.in and
 * generate output tools_declarations.h
 *
 * See LICENCE.md for Copyright information.
 */

#include <algorithm>
#include <cstring>
#include <functional>
#include <locale>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <boost/algorithm/string/replace.hpp>

#include <libgen.h>

#include "generators_common.h"

namespace yg = yiqi::generators;
namespace ygu = yiqi::generators::util;
namespace ygt = yiqi::generators::transformations;

namespace
{
    std::string GetBasename (std::string const &str)
    {
        /* There's no guarunteee that basename won't modify its argument
         * so we need to create a copy here */
        char        *inputCopy = ::strdup (str.c_str ());
        char        *base = ::basename (inputCopy);
        std::string returnValue (base);

        ::free (inputCopy);
        return returnValue;
    }

    std::string RemoveSuffixFromInputFileName (std::string const &inputName)
    {
        size_t position = inputName.find_last_of ('.');

        if (position == std::string::npos)
            throw std::logic_error ("File name does not have a suffix");

        return inputName.substr (0, position);
    }

    void InsertHeader (std::string       &str,
                       std::string const &inputFileName)
    {
        std::string inputBasename (GetBasename (inputFileName));
        char const  *headerBegin = "/*\n"
                                   " * ";
        std::string fileName (RemoveSuffixFromInputFileName (inputBasename));
        char const  *headerMiddle = "\n"
                                    " *\n"
                                    " * This file was autogenerated from\n"
                                    " * ";
        char const *headerEnd = "\n"
                               " * Do not edit this file.\n"
                               " *\n"
                               " * See LICENCE.md for Copyright information.\n"
                               " */";

        std::stringstream ss;

        ss << headerBegin
           << fileName
           << headerMiddle
           << inputBasename
           << headerEnd;

        str.insert (0, ss.str ());
    }

    typedef std::vector <std::pair <std::string, std::string>> Tokens;

    void ReplaceTokens (std::string  &input,
                        Tokens const &tokens)
    {
        for (auto const &kvp : tokens)
        {
            std::stringstream markedFirst;
            markedFirst << "@" << kvp.first << "@";
            boost::replace_all (input, markedFirst.str (), kvp.second);
        }
    }

    Tokens
    GenerateTokensFromOptions (std::vector <std::string> const &extraOpts)
    {
        if (extraOpts.size () % 2 != 0)
            throw std::logic_error ("Must be an even number of extra options");

        std::vector <std::pair <std::string, std::string>> tokens;
        tokens.reserve (extraOpts.size () / 2);

        auto const &end (extraOpts.end ());

        for (auto it = extraOpts.begin (); it != end; std::advance (it, 2))
            tokens.push_back (std::make_pair (*it, *(it + 1)));

        return tokens;
    }
}

int main (int argc, char **argv)
{
    std::string inputFileName, outputFileName;
    std::vector <std::string> extraOptions;

    try
    {
        using namespace std::placeholders;

        yg::GetOptions (argc,
                        argv,
                        inputFileName,
                        outputFileName,
                        extraOptions);

        Tokens tokensMap (GenerateTokensFromOptions (extraOptions));

        std::string inputFileData (yg::ReadInputFile (inputFileName));

        yg::ApplyTransformations (inputFileData,
                                  ygt::DeleteComments,
                                  std::bind (InsertHeader,
                                             _1, std::ref (inputFileName)),
                                  std::bind (ReplaceTokens,
                                             _1, std::ref (tokensMap)),
                                  ygt::CompressNewlines);
        yg::WriteToOutputFile (outputFileName,
                               inputFileData);

    }
    catch (std::exception const &e)
    {
        std::cerr << e.what () << std::endl;
        return 1;
    }
    catch (...)
    {
        std::cerr << "Unknown error occurred";
        return 1;
    }

    return 0;
}
