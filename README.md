Yíqì (仪器)
====

Instrumentation for code under test. Yíqì (仪器) is a Mandarin word for "Instrument" according to Google Translate.

Under construction
==================

Yiqi is currently under construction, but a working version should be ready soon.

Status
======
[![Build Status](https://travis-ci.org/polysquare/yiqi.svg?branch=master)](https://travis-ci.org/polysquare/yiqi)
[![Coverage Status](https://coveralls.io/repos/polysquare/yiqi/badge.png?branch=master)](https://coveralls.io/r/polysquare/yiqi?branch=master)

Purpose
=======

The purpose of Yiqi is to provide a small amount of framework that you can integrate into your tests written using Google Test (and perhaps other testing frameworks in future) to provide instrumentation analysis for when your tests run using valgrind. Possible usages include call profiling (callgrind), cache profiling (cachegrind), memory error detection (CGCheck, memcheck) and heap profiling (massif).

Running tests under these tools is a good idea because it means that you can profile typical usages of your system in a predictable and automated manner, as opposed to running your application through valgrind.

What makes Yiqi different from simply just running your tests under these tools is that Yiqi allows you to specify where client code begins and ends, so your test code doesn't clutter up the statistics that you're trying to gather from your client code. This allows you to keep track of instrumentation records for a system under test accurately without having to worry about changes or details in the test framework affecting the results of that instrumentation.

Proposed Usage
==============

Link your tests to the yiqi_main library. Then you can provide the tooling you wish to use on the command line:

./your_test_binary --yiqi_tool memcheck

By default, Yiqi will not perform instrumentation. However, should instrumentation be required, it will re-exec your test under valgrind for you.

Yiqi needs to know when your client code begins and ends so it can skip providing statistics on test code. To do that, you can either use the provided macro CLIENT_CODE, or if you don't like macros, provide a functor () to ExecuteClientCode (); For example:

    TEST_F (Fixture, Test)
    {
        int x = 0;
        int y = 1;

        int result = 0;

        CLIENT_CODE ({
             result = client::do_something (x, y);
        })

        EXPECT_EQ (1, result);
    }

Or, without the macro:

    TEST_F (Fixture, Test)
    {
        int x = 0;
        int y = 1;

        int result = 0;

        ExecuteClientCode ([&]() {
            result = client::do_something (x, y);
        }

        EXPECT_EQ (1, result);
    }

Yiqi will print some information that come from the instrumentation and fail your test if there are serious errors (for example, improper memory usage or definite leaks) that instrumentation detects. It wil also add this data to the gtest xml output, so that it can be tracked by continous-integration systems.

Caveats
=======

Yiqi probably won't work with any test framework that requires gtest_main to be overriden like xorg-gtest. Though maybe we can get around that be LD_PRELOAD'ing another main () if that's even possible.

Yiqi will require all tests to inherit from the fixture yiqi::InstrumentableTest if they wish to use instrumentation. That means no fixture-less tests.

The xml data format hasn't been defined yet.
