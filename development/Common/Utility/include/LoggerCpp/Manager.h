/**
 * @file    Manager.h
 * @ingroup LoggerCpp
 * @brief   The static class that manage the registered Channel and Output objects
 *
 * Copyright (c) 2013 Sebastien Rombauts (sebastien.rombauts@gmail.com)
 *
 * Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
 * or copy at http://opensource.org/licenses/MIT)
 */
#pragma once

#include "LoggerCpp/Log.h"
#include "LoggerCpp/Channel.h"
#include "LoggerCpp/Output.h"
#include "LoggerCpp/Config.h"
#include "Meta/CompilerAlgo.h"
#include <condition_variable>

namespace yaget
{
    namespace ylog
    {

        template<typename T>
        ylog::Output* CreateOutputInstance(const ylog::Config::Ptr& configPtr)
        {
            return new T(configPtr);
        }

        /**
         * @brief   The static class that manage the registered channels and outputs
         * @ingroup LoggerCpp
         *
         *  The Manager keeps a map of all the named Channel objects
         * and share them on demand by new Logger objects created with the same name.
         *
         *  Thus the Manager is able to change the Log::Level of selected Channel object,
         * impacting all the Logger objects using it.
         *
         * The Manager also keeps a list of all configured Output object to output the Log objects.
         */
        struct Manager
        {
        public:
            /**
             * @brief Create and configure the Output objects.
             *
             * @see setChannelConfig()
             *
             * @param[in] aConfigList   List of Config for Output objects
             */
            static void configure(const Config::Vector& aConfigList);

            /**
             * @brief Return the Channel corresponding to the provided name
             *
             * Create a new Channel or get the existing one.
             *
             * @param[in] apChannelName String to identify the underlying Channel of a Logger
             *
             * @return Pointer to the corresponding Channel (never nullptr)
             */
            static Channel::Ptr get(const char* apChannelName);

            /**
             * @brief Output the Log to all the active Output objects.
             *
             * Dispatch the Log to OutputConsole/OutputFile/OutputVS/OutputMemory...
             *
             * @param[in] aChannelPtr   The underlying Channel of the Log
             * @param[in] aLog          The Log to output
             */
            static void output(const Channel::Ptr& aChannelPtr, const Log& aLog);

            /**
             * @brief Set the default output Log::Level of any new Channel
             */
            static void setDefaultLevel(Log::Level aLevel);

            /**
             * @brief Serialize the current Log::Level of Channel objects and return them as a Config instance
             */
            static Config::Ptr getChannelConfig(void);

            /**
             * @brief Set the Log::Level of Channel objects from the provided Config instance
             */
            static void setChannelConfig(const Config::Ptr& aConfigPtr);

            static bool IsValidTag(uint32_t tag);
            static bool IsFilter(uint32_t tag);
            static bool IsSeverityFilter(ylog::Log::Level severity, uint32_t tag);
            static void AddFilter(uint32_t tag);
            static bool IsOverrideFilter(uint32_t tag);
            static void AddOverrideFilter(uint32_t tag);
            static void RemoveFilter(uint32_t tag);
            static void AddOutput(Output::Ptr outputPtr);

            template<typename T, typename... Args>
            static void AddOutput(Args&&... args)
            {
                typename T::Ptr logOutput = std::make_shared<T>(std::forward<Args>(args)...);
                ylog::Manager::AddOutput(logOutput);
            }

            using OutputCreator = Output*(*)(const Config::Ptr& configPtr);

            template<typename T>
            static void RegisterOutputType(OutputCreator outputCreator)
            {
                RegisterOutputType(typeid(T).name(), outputCreator);
            }

            template<typename... Args>
            static void RegisterOutputTypes()
            {
                using LogOutputs = std::tuple<Args...>;
                meta::for_each_type<LogOutputs>([](const auto& logType)
                {
                    using LogType = std::decay_t<decltype(*logType)>;
                    RegisterOutputType<LogType>(&ylog::CreateOutputInstance<LogType>);
                });
            }

            static void RegisterOutputType(const char* name, OutputCreator outputCreator);
            static void ResetRuntimeData();
        };

    } // namespace ylog
} // namespace yaget
