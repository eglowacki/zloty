//////////////////////////////////////////////////////////////////////
// DevConfigurationParsers.h
//
//  Copyright 1/8/2020 Edgar Glowacki
//
//  Maintained by: Edgar
//
//  NOTES:
//      Used internally by DevConfiguration to parse
//      config file json sections.
//      Not intended for public use
//      In most cases you may need to include:
//          #include "Debugging/DevConfiguration.h"
//      before including this one.
//          
//
//
//  #include "Debugging/DevConfigurationParsers.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "YagetCore.h"
#include "App/AppUtilities.h"
#include "Json/JsonHelpers.h"


namespace std
{
    void from_json(const nlohmann::json& j, yaget::dev::Configuration::Init::VTSConfigList& vtsConfig);
    void from_json(const nlohmann::json& j, yaget::dev::Configuration::Init::EnvironmentList& environment);

} // namespace std


namespace DirectX::SimpleMath
{

    inline void to_json(nlohmann::json& j, const math3d::Color& color)
    {
        j = fmt::format("{}, {}, {}, {}", color.R(), color.G(), color.B(), color.A());
    }

    inline void from_json(const nlohmann::json& j, math3d::Color& color)
    {
        std::string source;
        j.get_to(source);

        const auto values = yaget::conv::Split(source, ",");
        if (values.size() == 4)
        {
            const float r = yaget::conv::AtoN<float>(values[0].c_str());
            const float g = yaget::conv::AtoN<float>(values[1].c_str());
            const float b = yaget::conv::AtoN<float>(values[2].c_str());
            const float a = yaget::conv::AtoN<float>(values[3].c_str());
            color = math3d::Color(r, g, b, a);
        }
    }

} // namespace DirectX::SimpleMath


namespace yaget::util
{
    inline void to_json(nlohmann::json& j, const util::EnvAlias& alias)
    {
        j["Path"] = alias.Value;
        j["ReadOnly"] = alias.ReadOnly;
    }

    inline void from_json(const nlohmann::json& j, util::EnvAlias& alias)
    {
        if (j.is_string())
        {
            alias = { j.get<std::string>() };
        }
        else if (j.is_object())
        {
            alias.Value = json::GetValue(j, "Path", alias.Value);
            alias.ReadOnly = json::GetValue(j, "ReadOnly", alias.ReadOnly);
        }
    }

    inline bool operator==(const util::EnvAlias& lhs, const util::EnvAlias& rhs)
    {
        return lhs.Value == rhs.Value && lhs.ReadOnly == rhs.ReadOnly;
    }

} // namespace yaget::util


namespace yaget::dev
{
    inline bool operator==(const Configuration::Debug::Flags& lhs, const Configuration::Debug::Flags& rhs)
    {
        return lhs.Physics == rhs.Physics &&
            lhs.Gui == rhs.Gui &&
            lhs.SuppressUI == rhs.SuppressUI &&
            lhs.BuildId == rhs.BuildId &&
            lhs.MetricGather == rhs.MetricGather &&
            lhs.DisregardDebugger == rhs.DisregardDebugger;
    }

    inline bool operator==(const Configuration::Debug::Logging& lhs, const Configuration::Debug::Logging& rhs)
    {
        return lhs.Level == rhs.Level &&
            lhs.Filters == rhs.Filters &&
            lhs.Outputs == rhs.Outputs;
    }

    inline bool operator==(const Configuration::Debug::Threads& lhs, const Configuration::Debug::Threads& rhs)
    {
        return lhs.VTSSections == rhs.VTSSections &&
            lhs.VTS == rhs.VTS &&
            lhs.Blob == rhs.Blob &&
            lhs.App == rhs.App;
    }

    inline bool operator==(const Configuration::Debug::Metrics& lhs, const Configuration::Debug::Metrics& rhs)
    {
        return lhs.AllowSocketConnection == rhs.AllowSocketConnection &&
               lhs.AllowFallbackToFile == rhs.AllowFallbackToFile &&
               lhs.SocketConnectionTimeout == rhs.SocketConnectionTimeout &&
               lhs.TraceFileName == rhs.TraceFileName &&
               lhs.TraceOn == rhs.TraceOn;
    }

    inline bool operator==(const Configuration::Debug& lhs, const Configuration::Debug& rhs)
    {
        return lhs.mFlags == rhs.mFlags &&
            lhs.mLogging == rhs.mLogging &&
            lhs.mThreads == rhs.mThreads &&
            lhs.mMetrics == rhs.mMetrics;
    }

    inline bool operator==(const Configuration::Init& lhs, const Configuration::Init& rhs)
    {
        return lhs.mVTSConfig == rhs.mVTSConfig &&
            lhs.mEnvironmentList == rhs.mEnvironmentList &&
            lhs.mWindowOptions == rhs.mWindowOptions && 
            lhs.mGameDirectorScript == rhs.mGameDirectorScript;
    }

    inline bool operator==(const Configuration::Runtime& lhs, const Configuration::Runtime& rhs)
    {
        return lhs.DpiScaleFactor == rhs.DpiScaleFactor &&
               lhs.mShowScriptHelp == rhs.mShowScriptHelp;
    }

    inline bool operator==(const Configuration::Graphics& lhs, const Configuration::Graphics& rhs)
    {
        return lhs.mDevice == rhs.mDevice &&
               lhs.mMemoryReport == rhs.mMemoryReport && 
               lhs.mGPUTraceback == rhs.mGPUTraceback;
    }

    inline bool operator==(const Configuration& lhs, const Configuration& rhs)
    {
        return lhs.mDebug == rhs.mDebug &&
               lhs.mInit == rhs.mInit &&
               lhs.mRuntime == rhs.mRuntime &&
               lhs.mGraphics == rhs.mGraphics &&
               lhs.mGuiColors == rhs.mGuiColors;
    }

    inline bool operator==(const Configuration::Init::VTS& lhs, const Configuration::Init::VTS& rhs)
    {
        return lhs.Name == rhs.Name &&
            lhs.Path == rhs.Path &&
            lhs.Filters == rhs.Filters &&
            lhs.Converters == rhs.Converters &&
            lhs.ReadOnly == rhs.ReadOnly &&
            lhs.Recursive == rhs.Recursive;
    }



    inline void to_json(nlohmann::json& j, const Configuration::Init::VTS& vts)
    {
        nlohmann::json& block = j[vts.Name];

        block["Path"] = vts.Path;
        block["ReadOnly"] = vts.ReadOnly;
        block["Recursive"] = vts.Recursive;
        block["Filters"] = vts.Filters;
        block["Converters"] = vts.Converters;
    }

    inline void from_json(const nlohmann::json& j, Configuration::Init::VTS& vts)
    {
        if (j.is_object() && !j.empty())
        {
            const auto it = j.begin();
            const nlohmann::json& block = *it;

            vts.Name = it.key();

            auto newPath = json::GetValue(block, "Path", Strings{});
            vts.Path.insert(std::end(vts.Path), std::begin(newPath), std::end(newPath));

            vts.ReadOnly = json::GetValue(block, "ReadOnly", vts.ReadOnly);
            vts.Recursive = json::GetValue(block, "Recursive", vts.Recursive);

            auto newFilters = json::GetValue(block, "Filters", Strings{});
            vts.Filters.insert(std::end(vts.Filters), std::begin(newFilters), std::end(newFilters));

            vts.Converters = json::GetValue(block, "Converters", vts.Converters);
        }
    }

    inline void to_json(nlohmann::json& j, const Configuration::Debug::Flags& flags)
    {
        j["Physics"] = flags.Physics;
        j["Gui"] = flags.Gui;
        j["SuppressUI"] = flags.SuppressUI;
        j["BuildId"] = flags.BuildId;
        j["MetricGather"] = flags.MetricGather;
        j["DisregardDebugger"] = flags.DisregardDebugger;
    }

    inline void from_json(const nlohmann::json& j, Configuration::Debug::Flags& flags)
    {
        flags.Physics = yaget::json::GetValue(j, "Physics", flags.Physics);
        flags.Gui = yaget::json::GetValue(j, "Gui", flags.Gui);
        flags.SuppressUI = yaget::json::GetValue(j, "SuppressUI", flags.SuppressUI);
        flags.BuildId = yaget::json::GetValue(j, "BuildId", flags.BuildId);
        flags.MetricGather = yaget::json::GetValue(j, "MetricGather", flags.MetricGather);
        flags.DisregardDebugger = yaget::json::GetValue(j, "DisregardDebugger", flags.DisregardDebugger);
    }

    //-------------------------------------------------------------------------------------------------------------------------------
    // parse more complex options into configuration struct
    inline void to_json(nlohmann::json& j, const dev::Configuration::Debug::Logging& logging)
    {
        j["Level"] = logging.Level;
        j["Filters"] = logging.Filters;
        j["Outputs"] = logging.Outputs;
        j["PrintThreadName"] = logging.PrintThreadName;
    }

    namespace parsers { Strings ParseLogFilterTags(const Strings& newFilterTags, const Strings& currentFilterTags); }

    //-------------------------------------------------------------------------------------------------------------------------------
    // parse more complex options into configuration struct
    inline void from_json(const nlohmann::json& j, dev::Configuration::Debug::Logging& logging)
    {
        using Sinks = dev::Configuration::Debug::Logging::Sinks;

        // TODO: setup json enum serialization for this (similar to asset resource json)
        logging.Level = yaget::json::GetValue(j, "Level", logging.Level);
        logging.Filters = yaget::json::GetValue<Strings>(j, "Filters", logging.Filters, parsers::ParseLogFilterTags);
        logging.Outputs = yaget::json::GetValue<Sinks>(j, "Outputs", logging.Outputs, [](const Sinks& newSinks, const Sinks& oldSinks)
        {
            if (newSinks.empty())
            { 
                return oldSinks;
            }
            else if (oldSinks.empty())
            {
                return newSinks;
            }

            Sinks results = oldSinks;
            for (const auto& sink : newSinks)
            {
                if (auto it = results.find(sink.first); it != std::end(results))
                {
                    auto newOptions = sink.second;
                    const auto& oldOptions = it->second;

                    newOptions.insert(std::begin(oldOptions), std::end(oldOptions));
                    results[sink.first] = newOptions;
                }
                else
                {
                    results[sink.first] = sink.second;
                }

            }
            return results;
        });

        logging.PrintThreadName = yaget::json::GetValue(j, "PrintThreadName", logging.PrintThreadName);
    }


    //-------------------------------------------------------------------------------------------------------------------------------
    inline void to_json(nlohmann::json& j, const dev::Configuration::Debug::Threads& threads)
    {
        j["VTSSections"] = threads.VTSSections;
        j["VTS"] = threads.VTS;
        j["Blob"] = threads.Blob;
        j["App"] = threads.App;
    }

    //-------------------------------------------------------------------------------------------------------------------------------
    inline void from_json(const nlohmann::json& j, dev::Configuration::Debug::Threads& threads)
    {
        threads.VTSSections = json::GetValue(j, "VTSSections", threads.VTSSections);
        threads.VTS = json::GetValue(j, "VTS", threads.VTS);
        threads.Blob = json::GetValue(j, "Blob", threads.Blob);
        threads.App = json::GetValue(j, "App", threads.App);
    }


    //------------------------------------------------------------------------------------------------------------------------------------------------------
    // Read data from json block and populate metrics structure
    inline void to_json(nlohmann::json& j, const dev::Configuration::Debug::Metrics& metrics)
    {
        j["AllowSocketConnection"] = metrics.AllowSocketConnection;
        j["AllowFallbackToFile"] = metrics.AllowFallbackToFile;
        j["SocketConnectionTimeout"] = metrics.SocketConnectionTimeout;
        j["TraceFileName"] = metrics.TraceFileName;
        j["TraceOn"] = metrics.TraceOn;
    }

    //-------------------------------------------------------------------------------------------------------------------------------
    inline void from_json(const nlohmann::json& j, dev::Configuration::Debug::Metrics& metrics)
    {
        metrics.AllowSocketConnection = json::GetValue(j, "AllowSocketConnection", metrics.AllowSocketConnection);
        metrics.AllowFallbackToFile = json::GetValue(j, "AllowFallbackToFile", metrics.AllowFallbackToFile);
        metrics.SocketConnectionTimeout = json::GetValue(j, "SocketConnectionTimeout", metrics.SocketConnectionTimeout);
        metrics.TraceFileName = json::GetValue(j, "TraceFileName", metrics.TraceFileName);
        metrics.TraceOn = json::GetValue(j, "TraceOn", metrics.TraceOn);
    }


    //------------------------------------------------------------------------------------------------------------------------------------------------------
    inline void to_json(nlohmann::json& j, const dev::Configuration::Debug& debug)
    {
        j["Flags"] = debug.mFlags;
        j["Logging"] = debug.mLogging;
        j["Threads"] = debug.mThreads;
        j["Metrics"] = debug.mMetrics;
    }

    //------------------------------------------------------------------------------------------------------------------------------------------------------
    inline void from_json(const nlohmann::json& j, dev::Configuration::Debug& debug)
    {
        if (yaget::json::IsSectionValid(j, "Flags", ""))
        {
            from_json(j["Flags"], debug.mFlags);
        }
        if (yaget::json::IsSectionValid(j, "Logging", ""))
        {
            from_json(j["Logging"], debug.mLogging);
        }
        if (yaget::json::IsSectionValid(j, "Threads", ""))
        {
            from_json(j["Threads"], debug.mThreads);
        }
        if (yaget::json::IsSectionValid(j, "Metrics", ""))
        {
            from_json(j["Metrics"], debug.mMetrics);
        }
    }

    //------------------------------------------------------------------------------------------------------------------------------------------------------
    inline void to_json(nlohmann::json& j, const dev::Configuration::Init& init)
    {
        //j["Flags"] = init.VSync;
        //j["FullScreen"] = init.FullScreen;
        //j["ResX"] = init.ResX;
        //j["ResY"] = init.ResY;
        //j["LogicTick"] = init.LogicTick;

        j["VTS"] = init.mVTSConfig;
        j["Aliases"] = init.mEnvironmentList;
        j["WindowOptions"] = init.mWindowOptions;
        j["GameDirectorScript"] = init.mGameDirectorScript;
    }

    //------------------------------------------------------------------------------------------------------------------------------------------------------
    inline void from_json(const nlohmann::json& j, dev::Configuration::Init& init)
    {
        //init.VSync = json::GetValue(j, "VSync", init.VSync);
        //init.FullScreen = json::GetValue(j, "FullScreen", init.FullScreen);
        //init.ResX = json::GetValue(j, "ResX", init.ResX);
        //init.ResY = json::GetValue(j, "ResY", init.ResY);
        //init.LogicTick = json::GetValue(j, "LogicTick", init.LogicTick);

        if (yaget::json::IsSectionValid(j, "VTS", ""))
        {
            from_json(j["VTS"], init.mVTSConfig);
        }
        if (yaget::json::IsSectionValid(j, "Aliases", ""))
        {
            from_json(j["Aliases"], init.mEnvironmentList);
        }

        init.mWindowOptions = json::GetValue(j, "WindowOptions", init.mWindowOptions);
        init.mGameDirectorScript = json::GetValue(j, "GameDirectorScript", init.mGameDirectorScript);
    }

    //------------------------------------------------------------------------------------------------------------------------------------------------------
    inline void to_json(nlohmann::json& j, const dev::Configuration::Runtime& runtime)
    {
        j["DpiScaleFactor"] = runtime.DpiScaleFactor;
        j["ShowScriptHelp"] = runtime.mShowScriptHelp;
    }

    //------------------------------------------------------------------------------------------------------------------------------------------------------
    inline void from_json(const nlohmann::json& j, dev::Configuration::Runtime& runtime)
    {
        runtime.DpiScaleFactor = json::GetValue(j, "DpiScaleFactor", runtime.DpiScaleFactor);
        runtime.mShowScriptHelp = json::GetValue(j, "ShowScriptHelp", runtime.mShowScriptHelp);
    }

    //------------------------------------------------------------------------------------------------------------------------------------------------------
    inline void to_json(nlohmann::json& j, const dev::Configuration::Graphics& graphics)
    {
        j["Device"] = graphics.mDevice;
        j["MemoryReport"] = graphics.mMemoryReport;
        j["GPUTraceback"] = graphics.mGPUTraceback;
    }

    //------------------------------------------------------------------------------------------------------------------------------------------------------
    inline void from_json(const nlohmann::json& j, dev::Configuration::Graphics& graphics)
    {
        graphics.mDevice = json::GetValue(j, "Device", graphics.mDevice);
        graphics.mMemoryReport = json::GetValue(j, "MemoryReport", graphics.mMemoryReport);
        graphics.mGPUTraceback = json::GetValue(j, "GPUTraceback", graphics.mGPUTraceback);
    }

    //------------------------------------------------------------------------------------------------------------------------------------------------------
    inline void to_json(nlohmann::json& j, const dev::Configuration& configuration)
    {
        j["Debug"] = configuration.mDebug;
        j["Init"] = configuration.mInit;
        j["Runtime"] = configuration.mRuntime;
        j["Graphics"] = configuration.mGraphics;
    }

    //------------------------------------------------------------------------------------------------------------------------------------------------------
    inline void from_json(const nlohmann::json& j, dev::Configuration& configuration)
    {
        if (yaget::json::IsSectionValid(j, "Debug", ""))
        {
            from_json(j["Debug"], configuration.mDebug);
        }
        if (yaget::json::IsSectionValid(j, "Init", ""))
        {
            from_json(j["Init"], configuration.mInit);
        }
        if (yaget::json::IsSectionValid(j, "Runtime", ""))
        {
            from_json(j["Runtime"], configuration.mRuntime);
        }
        if (yaget::json::IsSectionValid(j, "Graphics", ""))
        {
            from_json(j["Graphics"], configuration.mGraphics);
        }
    }

} // namespace yaget::dev


namespace yaget::dev::parsers
{
    //--------------------------------------------------------------------------------------------------
    //! Read log filters and combine currentFilterTags with newFilterTags without dups and removing tag with '-'
    inline yaget::Strings ParseLogFilterTags(const yaget::Strings& newFilterTags, const yaget::Strings& currentFilterTags)
    {
        // now let's combine new ones with current
        Strings combinedFilterTags = currentFilterTags;
        for (const auto& it : newFilterTags)
        {
            if (*it.begin() == '-')
            {
                std::string tag(it.begin() + 1, it.end());
                auto foundTag = std::ranges::find(combinedFilterTags.begin(), combinedFilterTags.end(), tag);
                if (foundTag != combinedFilterTags.end())
                {
                    combinedFilterTags.erase(foundTag);
                }
            }
            else if (std::ranges::find(combinedFilterTags.begin(), combinedFilterTags.end(), it) == combinedFilterTags.end())
            {
                combinedFilterTags.push_back(it);
            }
        }

        return combinedFilterTags;
    }

} // namespace yaget::dev::parsers


namespace std
{
    inline void from_json(const nlohmann::json& j, yaget::dev::Configuration::Debug::Logging::Sinks& sinks)
    {
        for (auto it = j.begin(); it != j.end(); ++it)
        {
            auto key = it.key();
            auto value = it.value();

            if (value.is_null())
            {
                sinks[key] = {};
            }
            else if (value.is_object())
            {
                auto text = yaget::json::PrettyPrint(value);
                sinks[key] = value.get<yaget::dev::Configuration::Debug::Logging::Options>();
            }
        }
    }

    inline void from_json(const nlohmann::json& j, yaget::dev::Configuration::Init::VTSConfigList& vtsConfig)
    {
        using namespace yaget::dev;

        if (j.is_array())
        {
            for (auto elem : j)
            {
                if (elem.is_object() && !elem.empty())
                {
                    const auto name = elem.begin().key();

                    Configuration::Init::VTS existingVTS{ name };
                    if (auto it = vtsConfig.find(existingVTS); it != std::end(vtsConfig))
                    {
                        existingVTS = *it;
                    }

                    from_json(elem, existingVTS);
                    const auto config = vtsConfig.insert(existingVTS);
                    if (!config.second)
                    {
                        // this is set and elements are immutable
                        vtsConfig.erase(existingVTS);
                        vtsConfig.insert(existingVTS);
                    }
                }
            }
        }
    }

    inline void from_json(const nlohmann::json& j, yaget::dev::Configuration::Init::EnvironmentList& environment)
    {
        for (auto it = j.begin(); it != j.end(); ++it)
        {
            auto key = it.key();
            yaget::conv::Trim(key);
            auto value = it.value();

            from_json(value, environment[key]);
        }
    }
} // namespace std
