//////////////////////////////////////////////////////////////////////
// TestHelpers.h
//
//  Copyright 11/27/2019 Edgar Glowacki
//
//  Maintained by: Edgar
//
//  NOTES:
//      Utility and helper functions to simplify using tests
//
//
//  #include "TestHelpers/TestHelpers.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "YagetCore.h"
#include "App/ConsoleApplication.h"
#include "Items/ItemsDirector.h"
#include "VTS/ToolVirtualTransportSystem.h"


namespace yaget::test
{
    // Call this to initialize testing environment by adding default log outputs
    void InitializeEnvironment(const char* configBlockData = nullptr, std::size_t size = 0);
    void ResetEnvironment();

    // RTTI support for cleaning up test environment if different between tests.
    class Environment
    {
    public:
        Environment(const char* configBlockData = nullptr, std::size_t size = 0)
        {
            InitializeEnvironment(configBlockData, size);
        }
        
        ~Environment()
        {
            ResetEnvironment();
        }
    };

    // M is Messaging
    // SC is SystemsCoordinator
    template <typename M, typename SC>
    class ApplicationFramework
    {
    public:
        ApplicationFramework(const char* testName)
            : mIdGameCache({})
            , mMessaging()
            , mVts({}, {})
            , mDirector{}
            , mOptions("TestOptions")
            , mApplication("Test Window", mDirector, mVts, mOptions)
            , mSystemCoordinator(mMessaging, mApplication)
            , mTestName(std::string("Test.") + testName)
        {
            metrics::MarkAddMessage(mTestName + " Start", metrics::MessageScope::Global, 1);
        }

        ~ApplicationFramework()
        {
            metrics::MarkAddMessage(mTestName + " End", metrics::MessageScope::Global, 1);
        }

        yaget::IdGameCache& Ids() { return mIdGameCache; }
        SC& SystemsCoordinator() { return mSystemCoordinator; }

    private:
        yaget::IdGameCache mIdGameCache;
        M mMessaging;
        yaget::io::tool::VirtualTransportSystemDefault mVts;
        yaget::items::BlankDefaultDirector mDirector;
        yaget::args::Options mOptions;
        yaget::app::BlankApplication mApplication;
        SC mSystemCoordinator;
        std::string mTestName;
    };


} // namespace yaget::test
