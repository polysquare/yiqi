/*
 * instrumentation_none.h:
 * Provides an implementation of a yiqi::instrumentation::tools::Tool
 * which does not perform any instrumentation at all
 *
 * See LICENCE.md for Copyright information
 */

#include "constants.h"
#include "instrumentation_tool.h"
#include "instrumentation_tools_available.h"

namespace yconst = yiqi::constants;
namespace yit = yiqi::instrumentation::tools;
namespace yitp = yiqi::instrumentation::tools::programs;
namespace yitc = yiqi::instrumentation::tools::controllers;

namespace
{
    class NoneProgram :
        public yit::Program
    {
        private:

            std::string const & WrapperBinary () const;
            std::string const & WrapperOptions () const;
            std::string const & Name () const;
            yconst::InstrumentationTool ToolIdentifier () const;
    };

    class NoneController :
        public yit::Controller
    {
        private:

            ToolID ToolIdentifier () const;
            void Start () override;
            Status Stop () override;
    };
}

std::string const &
NoneProgram::Name () const
{
    static std::string const name =
        yconst::StringFromTool (ToolIdentifier ());
    return name;
}

yconst::InstrumentationTool
NoneProgram::ToolIdentifier () const
{
    return yconst::InstrumentationTool::None;
}

std::string const &
NoneProgram::WrapperBinary () const
{
    static std::string const wrapper ("");
    return wrapper;
}

std::string const &
NoneProgram::WrapperOptions () const
{
    static std::string const options ("");
    return options;
}

yconst::InstrumentationTool
NoneController::ToolIdentifier () const
{
    return yconst::InstrumentationTool::None;
}

void
NoneController::Start ()
{
}

NoneController::Status NoneController::Stop ()
{
    return NoneController::Status ();
}

yitp::Unique
yitp::MakeNone ()
{
    return yitp::Unique (new NoneProgram ());
}

yitc::Unique
yitc::MakeNone ()
{
    return yitc::Unique (new NoneController ());
}
