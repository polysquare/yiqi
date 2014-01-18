/*
 * yiqi_tools_declarations_generator.cpp
 *
 * Tool used to parse an input tools_declarations.h.in and
 * generate output tools_declarations.h
 *
 * See LICENCE.md for Copyright information.
 */

#include <cstring>
#include <functional>
#include <locale>
#include <sstream>
#include <string>
#include <vector>

#include "generators_common.h"

namespace yg = yiqi::generators;
namespace ygu = yiqi::generators::util;
namespace ygt = yiqi::generators::transformations;

namespace
{
    void InsertHeader (std::string &str)
    {
        char const *header = "/*\n"
                             " * tools_declarations.h\n"
                             " *\n"
                             " * This file was autogenerated from\n"
                             " * tools_declarations.h.in. It contains the\n"
                             " * declarations of all available tools to yiqi\n"
                             " * at the time this build was compiled.\n"
                             " * Do not edit this file.\n"
                             " *\n"
                             " * See LICENCE.md for Copyright information.\n"
                             " */";

        str.insert (0, header);
    }

    void GenerateInstrumentationToolsEnum (std::string             &string,
                                           ygu::StringVector const &tools)
    {
        std::string const searchToken ("INSTRUMENTATION_TOOLS_ID_DECL");
        auto transform =
            [](std::string const &str, size_t i, size_t n) -> std::string {
                std::stringstream ss;

                ss << str << " = " << (i - 1);
                if (i != n)
                    ss << ",";

                std::string modified (ss.str ());

                /* Capitalize the first letter */
                modified[0] = std::toupper (modified[0]);
                return modified;
            };
        ygu::GenerateFromTools (string,
                                searchToken,
                                transform,
                                tools);
    }

    void GenerateProgramsDeclaration (std::string             &string,
                                      ygu::StringVector const &tools)
    {
        static char const *header = "Unique Make";
        static size_t const headerSize = std::strlen (header);
        std::string searchToken ("INSTRUMENTATION_TOOLS_PROGRAM_DECL");
        auto transform =
            [](std::string const &str, size_t i, size_t n) ->std::string {
                std::stringstream ss;

                ss << header << str << " ();";
                std::string modified (ss.str ());
                modified[headerSize] = std::toupper(modified[headerSize]);
                return modified;
            };
        ygu::GenerateFromTools (string,
                                searchToken,
                                transform,
                                tools);
    }

    void GenerateControllersDeclaration (std::string             &string,
                                         ygu::StringVector const &tools)
    {
        static char const *header = "Unique Make";
        static size_t const headerSize = std::strlen (header);
        const std::string searchToken ("INSTRUMENTATION_TOOLS_CONTROLLER_DECL");
        auto transform =
            [](std::string const &str, size_t i, size_t n) ->std::string {
                std::stringstream ss;

                ss << header << str << " ();";
                std::string modified (ss.str ());
                modified[headerSize] = std::toupper(modified[headerSize]);
                return modified;
            };
        ygu::GenerateFromTools (string,
                                searchToken,
                                transform,
                                tools);
    }
}

int main (int argc, char **argv)
{
    std::string inputFileName, outputFileName;
    std::vector <std::string> tools;

    try
    {
        using namespace std::placeholders;

        yg::GetOptions (argc,
                        argv,
                        inputFileName,
                        outputFileName,
                        tools);

        std::string inputFileData (yg::ReadInputFile (inputFileName));
        yg::ApplyTransformations (inputFileData,
                                  ygt::DeleteComments,
                                  ygt::RemoveLineIfEndingWithDEL,
                                  InsertHeader,
                                  std::bind (GenerateInstrumentationToolsEnum,
                                               _1, std::ref (tools)),
                                  std::bind (GenerateProgramsDeclaration,
                                              _1, std::ref (tools)),
                                  std::bind (GenerateControllersDeclaration,
                                               _1, std::ref (tools)),
                                  std::bind (ygt::GenerateInstrumentationToolsN,
                                               _1, tools.size ()),
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