/*
 * construction.cpp:
 * Implementation of the options -> construction parameters logic
 *
 * See LICENCE.md for Copyright information
 */

#include <array>
#include <sstream>
#include <string>

#include "construction.h"
#include "constants.h"
#include "instrumentation_tool.h"
#include "instrumentation_tools_available.h"

namespace yconst = yiqi::constants;
namespace yc = yiqi::construction;
namespace yit = yiqi::instrumentation::tools;
namespace yitp = yiqi::instrumentation::tools::programs;
namespace yitc = yiqi::instrumentation::tools::controllers;
namespace po = boost::program_options;

namespace
{
    std::string GetNoneString ()
    {
        auto const noneTool (yconst::InstrumentationTool::None);
        auto const noneString (yconst::StringFromTool (noneTool));
        return noneString;
    }
}

po::options_description
yc::FetchOptionsDescription ()
{
    po::options_description description ("Options");
    description.add_options ()
        (yconst::YiqiToolOption,
         po::value <std::string> ()->default_value (GetNoneString ()),
         "Tool");

    return description;
}

std::string
yc::ParseOptionsForToolName (int                argc,
                             char const * const *argv,
                             yc::Options const  &description)
{
    po::variables_map       variableMap;
    po::command_line_parser parser (argc, argv);

    po::store (parser.options (description).run (),
               variableMap);
    po::notify (variableMap);

    if (variableMap.count (yconst::YiqiToolOption))
        return variableMap[yconst::YiqiToolOption].as <std::string> ();

    return GetNoneString ();
}

namespace
{
    class ToolNotAvailableError :
        public std::exception
    {
        public:

            ToolNotAvailableError (yconst::InstrumentationTool id);
            virtual ~ToolNotAvailableError () throw () {};
            char const * what () const throw ();

        private:

            std::string msg;
    };
}

ToolNotAvailableError::ToolNotAvailableError (yconst::InstrumentationTool id) :
  msg ([](yconst::InstrumentationTool id) {
           std::stringstream ss;
           ss << "Requested unimplemented tool "
               << static_cast <unsigned int> (id);
           return ss.str ();
       } (id))
{
}

char const *
ToolNotAvailableError::what () const throw ()
{
    return msg.c_str ();
}

yit::FactoryPackage const &
yc::FactoryPackageForTool (yconst::InstrumentationTool toolID)
{
    try
    {
        auto const &fp (yit::FactoryPackagesArray ());

        yit::FactoryPackage const &factory =
            fp.at (static_cast <unsigned int> (toolID));
        return factory;
    }
    catch (std::out_of_range const &err)
    {
        throw ToolNotAvailableError (toolID);
    }
}
