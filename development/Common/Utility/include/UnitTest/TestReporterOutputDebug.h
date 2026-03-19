///////////////////////////////////////////////////////////////////////
// TestReporterOutputDebug.h
//
//  Copyright 9/8/2017 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      Test reporter output to OutputDebug (VC++)
//
//
//  #include "UnitTest/TestReporterOutputDebug.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "TestReporterStdout.h"
#include "CompositeTestReporter.h"
#include "StringHelpers.h"


namespace yaget
{
    class TestReporterOutputDebug : public UnitTest::TestReporter
    {
    private:
        void ReportFailure(UnitTest::TestDetails const& details, char const* failure) override
        {
            std::string message = std::format("{}({}): error: Failure in '{}': {}\n", details.filename, details.lineNumber, details.testName, failure);
            ::OutputDebugStringA(message.c_str());
        }

        void ReportTestStart(UnitTest::TestDetails const& test) override
        {
            std::string message = std::format("========= Unit Test Started,  Suite: '{}', Test: '{}' =====\n", test.suiteName, test.testName);
            ::OutputDebugStringA(message.c_str());
        }

        void ReportTestFinish(UnitTest::TestDetails const& test, float /*secondsElapsed*/) override
        {
            std::string message = std::format("    ===== Unit Test Finished, Suite: '{}', Test: '{}' =====\n", test.suiteName, test.testName);
            ::OutputDebugStringA(message.c_str());
        }

        void ReportSummary(int const totalTestCount, int const failedTestCount, int const failureCount, float const secondsElapsed) override
        {
            if (failureCount > 0)
            {
                std::string message = std::format("FAILURE: {} out of {} tests failed ({} failures).\n", failedTestCount, totalTestCount, failureCount);
                ::OutputDebugStringA(message.c_str());

            }
            else
            {
                std::string message = std::format("Success: {} tests passed.\n", totalTestCount);
                ::OutputDebugStringA(message.c_str());
            }

            std::string message = std::format("Test time: {:.2f} seconds.\n", secondsElapsed);
            ::OutputDebugStringA(message.c_str());
        }
    };

} // namespace yaget
