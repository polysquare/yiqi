/*
 * system_api_mock.h:
 * A Google Mock implementation of yiqi::system::api::SystemCalls
 *
 * See LICENCE.md for Copyright information
 */

#ifndef YIQI_SYSTEM_API_MOCK_H
#define YIQI_SYSTEM_API_MOCK_H

#include <string>

#include <gmock/gmock.h>

#include "system_api.h"

namespace yiqi
{
    namespace mock
    {
        namespace system
        {
            namespace api
            {
                class SystemCalls :
                    public yiqi::system::api::SystemCalls
                {
                    public:

                        SystemCalls ();
                        ~SystemCalls ();

                        MOCK_CONST_METHOD1 (ExeExists,
                                            bool (std::string const &));
                        MOCK_CONST_METHOD2 (ExecInPlace,
                                            void (char const         *,
                                                  char const * const *));
                        MOCK_CONST_METHOD0 (GetExecutablePath, std::string ());
                };
            }
        }
    }
}

#endif // YIQI_SYSTEM_API_MOCK_H