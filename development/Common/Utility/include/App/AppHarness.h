//////////////////////////////////////////////////////////////////////
// AppHarness.h
//
//  Copyright 6/30/2019 Edgar Glowacki
//
//  Maintained by: Edgar
//
//  NOTES:
//      Simple wrapper function to initialize, setup basic stuff, 
//      wrap in try/catch running if user callback (boiler plate code)
//
//
//  #include "App/AppHarness.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "YagetCore.h"
#include "Meta/CompilerAlgo.h"
#include "App/AppUtilities.h"
#include "StringHelpers.h"
#include "Exception/Exception.h"
#include <functional>


namespace yaget::app
{
    namespace helpers
    {
        using Callback = std::function<int()>;

        template<typename... Args>
        int Harness(const char* lpCmdLine, args::Options& options, Callback callback)
        {
            const char* configData = nullptr;
            size_t configSize = 0;

            using LogOutputs = std::tuple<Args...>;
            meta::for_each_type<LogOutputs>([](const auto& logType)
            {
                using LogType = std::decay_t<decltype(*logType)>;
                ylog::Manager::RegisterOutputType<LogType>(&ylog::CreateOutputInstance<LogType>);
            });

            if (system::InitializeSetup(lpCmdLine, options, configData, configSize) != system::InitializationResult::OK)
            {
                return -1;
            }

            int returnResult = 0;
            try
            {
                YLOG_NOTICE("MAIN", util::DisplayCurrentConfiguration(&options).c_str());
                returnResult = callback();
            }
            catch (const ex::standard& e)
            {
                YLOG_ERROR("MAIN", "Application terminated, Exception Error: '%s'", e.what());
                if (platform::IsDebuggerAttached())
                {
                    platform::DebuggerBreak();
                }

                std::string message = fmt::format("Yaget Engine runtime error\nExamine log at: '{}'\n{}", util::ExpendEnv("$(LogFolder)", nullptr), e.what());
                std::string errorTitle = fmt::format("{} Runtime Error", util::ExpendEnv("$(AppName)", nullptr));
                util::DisplayDialog(errorTitle.c_str(), message.c_str());

                return 1;
            }
            catch (const std::exception& e)
            {
                YAGET_ASSERT(false, "Application terminated, std::exception thrown: '%s'", e.what());
                return 1;
            }

            //render::Device::DebugReport();
            return returnResult;
        }

        template<typename... Args>
        int Harness(const wchar_t* lpCmdLine, args::Options& options, Callback callback)
        {
            return app::helpers::Harness<Args...>(conv::wide_to_utf8(lpCmdLine).c_str(), options, callback);
        }

        template<typename... Args>
        int Harness(int argc, char* argv[], args::Options& options, Callback callback)
        {
            std::string lineCommands;
            for (int i = 1; i < argc; ++i)
            {
                lineCommands += argv[i];
                lineCommands += " ";
            }

            return Harness<Args...>(lineCommands.c_str(), options, callback);
        }

        inline std::string ParseCommandLineParameters(int argc, char* argv[], const std::string& extraParameters = {})
        {
            std::string lineCommands;
            for (int i = 1; i < argc; ++i)
            {
                lineCommands += argv[i];
                lineCommands += i == argc - 1 ? "" : " ";
            }

            if (extraParameters.length())
            {
                lineCommands += lineCommands.empty() ? extraParameters : " " + extraParameters;
            }

            return lineCommands;
        }


    } // namespace helpers
} // namespace yaget::app

