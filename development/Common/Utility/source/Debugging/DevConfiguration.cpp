// DevConfiguration.cpp
#include "Debugging/DevConfiguration.h"
#include "Debugging/DevConfigurationParsers.h"
#include "App/Args.h"
#include "App/AppUtilities.h"
#include "App/FileUtilities.h"
#include "Logger/YLog.h"
#include "LoggerCpp/OutputDebug.h"
#include "Platform/Support.h"
#include "Exception/Exception.h"
#include "Fmt/format.h"
#include "Metrics/Gather.h"
#include "StringHelpers.h"

#include <filesystem>
#include <fstream>

//using json = nlohmann::json;
namespace fs = std::filesystem;


namespace
{
    yaget::dev::Configuration& GetCurrentConfiguration()
    {
        static yaget::dev::Configuration currentConfiguration;
        return currentConfiguration;
    }

    yaget::dev::ThreadIds& GetCurrentThreadIds()
    {
        static yaget::dev::ThreadIds currentThreadIds;
        return currentThreadIds;
    }

    using IncludeTracker = std::set<std::string>;
    yaget::Strings GetInclude(const nlohmann::json& jsonBlock, const fs::path& configPath, IncludeTracker& includeTracker)
    {
        using namespace yaget;

        Strings includeNames;

        if (json::IsSectionValid(jsonBlock, "Include", ""))
        {
            auto& incBlock = json::GetSection(jsonBlock, "Include");

            Strings foundIncludeNames;
            if (incBlock.is_string())
            {
                foundIncludeNames = { incBlock.get<std::string>() };
            }
            else if (incBlock.is_array())
            {
                foundIncludeNames = incBlock.get<Strings>();
            }

            for (const auto& includeFile : foundIncludeNames)
            {
                std::string includePathName = io::file::FindConfigFile(includeFile, false, nullptr);
                if (!includePathName.empty())
                {
                    if (!includeTracker.insert(includePathName).second)
                    {
                        platform::DebuggerOutput(fmt::format("[WARN:CONF] Include path: '{}' in configuration file: '{}' already previously included, resulting in circular inclusion, skipping.", includePathName, configPath.generic_string()));
                        continue;
                    }
                    else
                    {
                        includeNames.push_back(includePathName);
                    }
                }
            }
        }

        return includeNames;
    }

    //-------------------------------------------------------------------------------------------------------------------------------
    void ParseConfiguration(std::istream& configBindingsFile, const fs::path& configPath, IncludeTracker& includeTracker, yaget::dev::Configuration& configuration)
    {
        using namespace yaget;

        try
        {
            nlohmann::json root;
            configBindingsFile >> root;

            Strings includeNames = GetInclude(root, configPath, includeTracker);
            for (const auto& includePathName : includeNames)
            {
                std::ifstream configStream(includePathName.c_str());
                ParseConfiguration(configStream, includePathName, includeTracker, configuration);
            }

            if (json::IsSectionValid(root, "Configuration", ""))
            {
                const nlohmann::json& jsonBlock = json::GetSection(root, "Configuration", "");
                from_json(jsonBlock, configuration);
            }
        }
        catch (const std::exception& e)
        {
            std::string textError = fmt::format("Did not finished init configuration bindings from:\n{}.\nError: {}.\nIncludes: '{}'.", configPath.generic_string(), e.what(), "*"/*conv::Combine(currentIncludes, ", ")*/);
            throw yaget::ex::bad_init(textError);
        }
    }

} // namespace


//-------------------------------------------------------------------------------------------------------------------------------
void yaget::dev::Configuration::Refresh(const GuiColors& colors) const
{
    GetCurrentConfiguration().mGuiColors = colors;
}


//-------------------------------------------------------------------------------------------------------------------------------
void yaget::dev::Configuration::Runtime::RefreshDpi(float factor) const
{
    GetCurrentConfiguration().mRuntime.DpiScaleFactor = factor;
}


void yaget::dev::Configuration::Debug::Refresh(bool supressUI, int buildId) const
{
    GetCurrentConfiguration().mDebug.mFlags.SuppressUI = supressUI;
    GetCurrentConfiguration().mDebug.mFlags.BuildId = buildId;
}


//-------------------------------------------------------------------------------------------------------------------------------
void yaget::dev::Configuration::Debug::Refresh(const Logging::Sinks& outputs) const
{
    GetCurrentConfiguration().mDebug.mLogging.Outputs = outputs;
}


//-------------------------------------------------------------------------------------------------------------------------------
void yaget::dev::Configuration::Debug::RefreshGui(bool gui) const
{
    GetCurrentConfiguration().mDebug.mFlags.Gui = gui;
}


//-------------------------------------------------------------------------------------------------------------------------------
const yaget::dev::Configuration& yaget::dev::CurrentConfiguration()
{
    return GetCurrentConfiguration();
}


const yaget::dev::ThreadIds& yaget::dev::CurrentThreadIds()
{
    return GetCurrentThreadIds();
}


void yaget::dev::ThreadIds::RefreshLogic(uint32_t threadId) const
{
    GetCurrentThreadIds().Logic = threadId;
}


void yaget::dev::ThreadIds::RefreshRender(uint32_t threadId) const
{
    GetCurrentThreadIds().Render = threadId;
}


bool yaget::dev::ThreadIds::IsThreadLogic() const
{
    return Logic ? platform::CurrentThreadId() == Logic : true;
}


bool yaget::dev::ThreadIds::IsThreadRender() const
{
    return Render ? platform::CurrentThreadId() == Render : true;
}


//-------------------------------------------------------------------------------------------------------------------------------
/// do not use any YLOG since the log system has not been initialized yet
void yaget::dev::Initialize(const args::Options& options, const char* configData, size_t configSize)
{
    std::string optionsPathName = io::file::FindConfigFile("Configuration", true, &options);

    std::unique_ptr<std::istream> configStream;
    auto configuration = GetCurrentConfiguration();
    configuration = {};

    if (configData && configSize)
    {
        configStream.reset(new io::file::imemstream(configData, configSize));
    }
    else if (optionsPathName.empty())
    {
        configStream.reset(new io::file::imemstream("{}", sizeof("{}")));
    }
    else
    {
        configStream.reset(new std::ifstream(optionsPathName.c_str()));
    }

    IncludeTracker includeTracker;
    ParseConfiguration(*configStream, optionsPathName, includeTracker, configuration);

    // update configuration values and turn any specific knobs
    util::AddToEnvironment(configuration.mInit.mEnvironmentList);
    YM_GATHER_ACTIVATE(configuration.mDebug.mFlags.MetricGather);

    configuration.mDebug.mLogging.Level = options.find<std::string>("log_level", configuration.mDebug.mLogging.Level);

    // allow command line parameter override to show all logs
    if (options.find<bool>("log_filter_clear", false))
    {
        configuration.mDebug.mLogging.Filters.clear();
    }
    else
    {
        Strings newFilterTags = options.find<std::vector<std::string>>("log_filter", Strings{});
        configuration.mDebug.mLogging.Filters = parsers::ParseLogFilterTags(newFilterTags, configuration.mDebug.mLogging.Filters);
    }

    std::vector<std::string> logOutputs = options.find<std::vector<std::string>>("log_output", std::vector<std::string>());
    if (!logOutputs.empty())
    {
        configuration.mDebug.mLogging.Outputs.clear();
        for (auto&& it : logOutputs)
        {
            configuration.mDebug.mLogging.Outputs.insert(std::make_pair(it, Configuration::Debug::Logging::Options{}));
        }
    }

#ifndef YAGET_SHIPPING
    // if there is no log output setup, let's create default one
    if (configuration.mDebug.mLogging.Outputs.empty())
    {
        dev::Configuration::Debug::Logging::Sinks defualtOutputs;

        defualtOutputs["ylog::OutputDebug"]["split_log"] = "true";
        defualtOutputs["ylog::OutputConsole"] = {};

        configuration.mDebug.mLogging.Outputs = defualtOutputs;
        configuration.mDebug.mLogging.Level = "DBUG";
    }
#endif // YAGET_SHIPPING

    configuration.mInit.VSync = !options.find<bool>("vsync_off", !configuration.mInit.VSync);
    configuration.mInit.FullScreen = options.find<bool>("full_screen", configuration.mInit.FullScreen);
    configuration.mInit.ResX = options.find<int>("res_x", configuration.mInit.ResX);
    configuration.mInit.ResY = options.find<int>("res_y", configuration.mInit.ResY);
    configuration.mInit.LogicTick = options.find<int>("logic_tick", configuration.mInit.LogicTick);

    GetCurrentConfiguration() = configuration;
}