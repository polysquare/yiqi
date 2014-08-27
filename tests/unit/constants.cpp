/*
 * constants.cpp:
 *
 * Test that lookuping up constants through enums
 * and strings works as expected
 *
 * See LICENCE.md for Copyright information
 */

#include <functional>
#include <iostream>

#include <gtest/gtest.h>

#include <constants.h>
#include <instrumentation_tools_available.h>

using ::testing::WithParamInterface;
using ::testing::ValuesIn;

namespace yconst = yiqi::constants;

namespace yiqi
{
    namespace constants
    {
        std::ostream &
        operator<< (std::ostream &os, InstrumentationToolName const &tool)
        {
            int const toolInt = static_cast <int> (tool.tool);

            os << "Instrumentation Tool Name with " << std::endl
               << " tool (as int): " << toolInt << std::endl
               << " name (as char const *): " << tool.name;
            return os;
        }

        std::ostream &
        operator<< (std::ostream &os, InstrumentationTool tool)
        {
            int const toolInt = static_cast <int> (tool);

            os << "tool (as int): " << toolInt;
            return os;
        }
    }
}

class ConstantsLookup :
    public ::testing::Test,
    public WithParamInterface <yconst::InstrumentationToolName>
{
};

TEST_P (ConstantsLookup, LookupByStringMatchesEnumValue)
{
    yconst::InstrumentationTool const ExpectedEnumValue (GetParam ().tool);
    char const                        *LookupString (GetParam ().name);

    EXPECT_EQ (ExpectedEnumValue, yconst::ToolFromString (LookupString));
}


TEST_P (ConstantsLookup, LookupByEnumMatchesStringValue)
{
    char const                        *ExpectedStringValue (GetParam ().name);
    yconst::InstrumentationTool const LookupEnum (GetParam ().tool);

    EXPECT_STREQ (ExpectedStringValue, yconst::StringFromTool (LookupEnum));
}

TEST (ConstantsLookup, LookupByStringNotMatchingAnyKnownToolThrows)
{
    EXPECT_THROW ({
        char const *WrongLookupString = "____does_not_exist";
        yconst::ToolFromString (WrongLookupString);
    }, std::logic_error);
}

INSTANTIATE_TEST_CASE_P (AvailableTools, ConstantsLookup,
                         ValuesIn (yconst::InstrumentationToolNames ()));
