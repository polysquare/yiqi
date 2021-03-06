# /src/CMakeLists.txt
# Build the Yiqi main() library and the Yiqi Library
#
# See LICENCE.md for Copyright information

set (YIQI_MAIN_SRCS
     ${CMAKE_CURRENT_SOURCE_DIR}/yiqi_main.cpp)

polysquare_add_library (${YIQI_MAIN_LIBRARY} STATIC
                        SOURCES ${YIQI_MAIN_SRCS}
                        LIBRARIES ${GTEST_LIBRARY}
                        INTERNAL_INCLUDE_DIRS
                        ${YIQI_INTERNAL_INCLUDE_DIRECTORY}
                        ${YIQI_INTERNAL_SOURCE_DIRECTORY}
                        EXTERNAL_INCLUDE_DIRS
                        ${YIQI_EXTERNAL_INCLUDE_DIRS})

# Get available tools and generate tools_declarations.h and
# tools_factory.cpp
set (AVAILABLE_TOOLS)
yiqi_tools_available (AVAILABLE_TOOLS)

set (YIQI_TOOLS_DECLARATIONS_H_OUTPUT
     ${CMAKE_CURRENT_BINARY_DIR}/tools_declarations.h)
set (YIQI_TOOLS_DECLARATIONS_H_INPUT
     ${CMAKE_CURRENT_SOURCE_DIR}/tools_declarations.h.in)

add_custom_command (OUTPUT ${YIQI_TOOLS_DECLARATIONS_H_OUTPUT}
                    COMMAND $<TARGET_FILE:${YIQI_TOOLS_DECLARATIONS_GENERATOR}>
                    ARGS
                    -i ${YIQI_TOOLS_DECLARATIONS_H_INPUT}
                    -o ${YIQI_TOOLS_DECLARATIONS_H_OUTPUT}
                    ${AVAILABLE_TOOLS}
                    DEPENDS
                    ${YIQI_TOOLS_DECLARATIONS_GENERATOR}
                    ${YIQI_TOOLS_DECLARATIONS_H_INPUT}
                    COMMENT "Generating Yiqi Tools Declarations")

set (YIQI_TOOLS_FACTORY_H_OUTPUT
     ${CMAKE_CURRENT_BINARY_DIR}/tools_factory.cpp)
set (YIQI_TOOLS_FACTORY_H_INPUT
     ${CMAKE_CURRENT_SOURCE_DIR}/tools_factory.cpp.in)

add_custom_command (OUTPUT ${YIQI_TOOLS_FACTORY_H_OUTPUT}
                    COMMAND $<TARGET_FILE:${YIQI_TOOLS_FACTORY_GENERATOR}>
                    ARGS
                    -i ${YIQI_TOOLS_FACTORY_H_INPUT}
                    -o ${YIQI_TOOLS_FACTORY_H_OUTPUT}
                    ${AVAILABLE_TOOLS}
                    DEPENDS
                    ${YIQI_TOOLS_FACTORY_GENERATOR}
                    ${YIQI_TOOLS_FACTORY_H_INPUT}
                    COMMENT "Generating Yiqi Tools Factories")

yiqi_tools_are_available (YIQI_VALGRIND_TOOLS_AVAILABLE
                          "memcheck"
                          "callgrind"
                          "cachegrind")

# Sources that depend on valgrind
if (YIQI_VALGRIND_TOOLS_AVAILABLE)

    set (YIQI_LIBRARY_SRCS
         ${YIQI_LIBRARY_SRCS}
         ${CMAKE_CURRENT_SOURCE_DIR}/instrumentation_tool_valgrind_base.cpp
         ${CMAKE_CURRENT_SOURCE_DIR}/instrumentation_memcheck.cpp
         ${CMAKE_CURRENT_SOURCE_DIR}/instrumentation_callgrind.cpp
         ${CMAKE_CURRENT_SOURCE_DIR}/instrumentation_cachegrind.cpp)

endif (YIQI_VALGRIND_TOOLS_AVAILABLE)

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

    set (YIQI_LIBRARY_SRCS
         ${YIQI_LIBRARY_SRCS}
         ${CMAKE_CURRENT_SOURCE_DIR}/darwin_operating_system.cpp)

elseif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")

    set (YIQI_LIBRARY_SRCS
         ${YIQI_LIBRARY_SRCS}
         ${CMAKE_CURRENT_SOURCE_DIR}/gnu_operating_system.cpp)

endif (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

set (YIQI_LIBRARY_SRCS
     ${YIQI_LIBRARY_SRCS}
     ${CMAKE_CURRENT_SOURCE_DIR}/commandline.cpp
     ${CMAKE_CURRENT_SOURCE_DIR}/construction.cpp
     ${CMAKE_CURRENT_SOURCE_DIR}/constants.cpp
     ${CMAKE_CURRENT_SOURCE_DIR}/environment.cpp
     ${CMAKE_CURRENT_SOURCE_DIR}/init.cpp
     ${CMAKE_CURRENT_SOURCE_DIR}/instrumentation_tools_available.cpp
     ${CMAKE_CURRENT_SOURCE_DIR}/instrumentation_none.cpp
     ${CMAKE_CURRENT_SOURCE_DIR}/instrumentation_timer.cpp
     ${CMAKE_CURRENT_SOURCE_DIR}/instrumentation_passthrough.cpp
     ${CMAKE_CURRENT_SOURCE_DIR}/platform_gtest.cpp
     ${CMAKE_CURRENT_SOURCE_DIR}/reexecution.cpp
     ${CMAKE_CURRENT_SOURCE_DIR}/systempaths.cpp
     ${CMAKE_CURRENT_SOURCE_DIR}/system_unix.cpp
     ${CMAKE_CURRENT_BINARY_DIR}/tools_factory.cpp
     ${CMAKE_CURRENT_SOURCE_DIR}/commandline.h
     ${CMAKE_CURRENT_SOURCE_DIR}/construction.h
     ${CMAKE_CURRENT_SOURCE_DIR}/constants.h
     ${CMAKE_CURRENT_SOURCE_DIR}/instrumentation_tool.h
     ${CMAKE_CURRENT_SOURCE_DIR}/reexecution.h
     ${CMAKE_CURRENT_SOURCE_DIR}/scopeguard.h
     ${CMAKE_CURRENT_SOURCE_DIR}/systempaths.h
     ${CMAKE_CURRENT_SOURCE_DIR}/system_api.h
     ${CMAKE_CURRENT_SOURCE_DIR}/system_implementation.h
     ${CMAKE_CURRENT_BINARY_DIR}/tools_declarations.h
     ${CMAKE_CURRENT_SOURCE_DIR}/operating_system_implementation.h
     ${CMAKE_CURRENT_SOURCE_DIR}/operating_system_wrapper.h)

polysquare_add_library (${YIQI_LIBRARY} STATIC
                        SOURCES ${YIQI_LIBRARY_SRCS}
                        INTERNAL_INCLUDE_DIRS
                        ${YIQI_INTERNAL_SOURCE_BINARY_DIRECTORY}
                        ${YIQI_INTERNAL_INCLUDE_DIRECTORY}
                        ${YIQI_INTERNAL_SOURCE_DIRECTORY}
                        EXTERNAL_INCLUDE_DIRS
                        ${YIQI_EXTERNAL_INCLUDE_DIRS}
                        LIBRARIES ${YIQI_EXTERNAL_LIBRARIES})
