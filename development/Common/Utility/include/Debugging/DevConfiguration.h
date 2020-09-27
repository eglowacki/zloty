///////////////////////////////////////////////////////////////////////
// DevConfiguration.h
//
//  Copyright 8/1/2016 Edgar Glowacki.

//  Maintained by: Edgar
//
//  NOTES:
//      Configurations during development , etc
//
//  #include "Debugging/DevConfiguration.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "YagetCore.h"
#include "StringHelpers.h"
#include "Time/GameClock.h"
#include "App/AppUtilities.h"
#include <map>
#include <set>
#include <atomic>

namespace yaget
{
    namespace dev
    {
        //! Program configuration values, debug, init, runtime, etc.
        //! We provide global function CurrentConfiguration() that returns const ref to current data.
        //! There are some Refresh(...) methods that allow to change some values at run time.
        //! NOTE: Is this a good idea to change values at runtime?
        struct Configuration
        {
            // Any debug config values
            struct Debug
            {
                struct Flags
                {
                    bool Physics = true;
                    bool Gui = true;
                    bool SuppressUI = false;        // if true, then do not show any dialog boxes
                    int BuildId = -1;               // during dev, you can set to whatever number and be printed in log for engine init
                    bool MetricGather = false;      // turn on metric gather system
                    bool DisregardDebugger = false; // this controls how IsDebuggerAttached function works.
                };
                Flags mFlags;

                struct Logging
                {
                    // each output option is key = value. Not all outputs have options
                    using Options = std::map<std::string, std::string>;

                    // output: name, options for this output
                    using Sinks = std::map<std::string, Options>;

                    std::string Level{ "EROR" };
                    Strings Filters;
                    Sinks Outputs;
                };
                Logging mLogging;

                //! This is used to set how many threads for their respective pools to create.
                //! For some of performance or debug work, dial this down to 1 thread per pool
                //! 0 means numPoolThreads = num_cores - 1
                struct Threads
                {
                    uint32_t VTSSections = 0;
                    uint32_t VTS = 0;
                    uint32_t Blob = 0;
                    uint32_t App = 0;
                };
                Threads mThreads;

                struct Metrics
                {
                    bool AllowSocketConnection = false;
                    bool AllowFallbackToFile = false;
                    int SocketConnectionTimeout = 100;
                };
                Metrics mMetrics;

                void Refresh(bool supressUI, int buildId) const;
                void Refresh(const Logging::Sinks& outputs) const;
                void RefreshGui(bool gui) const;
            };
            Debug mDebug;

            // Any initialization config values
            struct Init
            {
                bool VSync = false;
                bool FullScreen = false;
                int ResX = 1920;
                int ResY = 1080;
                uint32_t LogicTick = time::kFrames_60;  // specifies what is the logic game thread ticking at, defaults to 60

                //  "Levels": {
                //      "Path": [ "$(TerrainPak)" ],
                //      "Filters" : [ "*.pak" ],
                //      "Converters": "PAK",
                //      "ReadOnly" : true,
                //      "Recursive": false
                // }

                struct VTS
                {
                    // DO NOT CHANGE ORDER OF MEMBER VARIABLES IN THIS STRUCT
                    std::string Name;
                    Strings Path;
                    Strings Filters;
                    std::string Converters;
                    bool ReadOnly = false;
                    bool Recursive = true;

                    bool operator<(const VTS& rhs) const { return Name < rhs.Name; }
                };
                using VTSConfigList = std::set<VTS>;

                VTSConfigList mVTSConfig;

                //! setup aliases
                // look in C:\Development\yaget\branch\version_0_2\Common\Utility\include\App\AppUtilities.h
                // for comments about predefined aliases
                //struct EnvAlias { std::string Value; bool ReadOnly; };
                //using EnvironmentList = std::map<std::string, EnvAlias>;
                using EnvAlias = util::EnvAlias;
                using EnvironmentList = util::EnvironmentList;
                EnvironmentList mEnvironmentList;

                // user options to use
                std::string mWindowOptions;
                // which script to start running when GameDirector is initialized, aka boot script
                std::string mGameDirectorScript;

                // This represents certain command line options, specially video/window options
                struct CLO
                {
                    int mResolutionX = 0;
                    int mResolutionY = 0;
                    bool mFullscreen = false;

                    bool IsResValid() const { return mResolutionX > 0 && mResolutionY > 0; }
                };
                CLO mCLO;
            };
            Init mInit;

            // Any runtime config values
            struct Runtime
            {
                float DpiScaleFactor = 1.0f;
                bool mShowScriptHelp = false;

                void RefreshDpi(float factor) const;
            };
            Runtime mRuntime;

            // Graphics related settings and configuration
            struct Graphics
            {
                // asset file to initial device configuration (VTS.Section)
                std::string mDevice;
                bool mMemoryReport = false;
            };
            Graphics mGraphics;

            using GuiColors = std::map<std::string, math3d::Color>;
            GuiColors mGuiColors{ 
                { "SectionText", { math3d::Color{ 0.0f, 0.7f, 1.0f, 1.0f }} },
                { "InfoText", { math3d::Color{ 0.9f, 0.9f, 0.0f, 1.0f }} },
                { "ActiveText", { math3d::Color{ 0.0f, 1.0f, 0.0f, 1.0f }} },
                { "InactiveText", { math3d::Color{ 0.2f, 0.4f, 0.0f, 1.0f }} },
                { "KeyText", { math3d::Color{ 0.2f, 1.0f, 0.9f, 1.0f }} },
                { "ValueText", { math3d::Color{ 0.9f, 0.8f, 1.0f, 1.0f }} },
                { "NodeText", { math3d::Color{ 0.6f, 0.6f, 0.6f, 1.0f }} }
            };

            void Refresh(const GuiColors& colors) const;
        };

        // Used in debug to make sure calls are done on correct threads
        struct ThreadIds
        {
            uint32_t Main = 0;
            uint32_t Logic = 0;
            uint32_t Render = 0;

            void RefreshLogic(uint32_t threadId) const;
            void RefreshRender(uint32_t threadId) const;

            bool IsThreadMain() const;
            bool IsThreadLogic() const;
            bool IsThreadRender() const;
        };

        void Initialize(const args::Options& options, const char* configData, size_t configSize);

        const Configuration& CurrentConfiguration();
        const ThreadIds& CurrentThreadIds();

    } // namespace dev

    namespace conv
    {
        template<>
        struct Convertor<yaget::dev::Configuration::Init::VTS>
        {
            static std::string ToString(const yaget::dev::Configuration::Init::VTS& value)
            {
                return value.Name;
            }
        };
    }

} // namespace yaget
