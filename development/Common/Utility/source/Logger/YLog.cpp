#include "Logger/YLog.h"
#include "App/AppUtilities.h"
#include "StringHelpers.h"
#include "Platform/Support.h"
#include "Debugging/DevConfiguration.h"
#include "Time/GameClock.h"
#include <fstream>

#include "YagetVersion.h"

#if YAGET_LOG_ENABLED == 1
    YAGET_COMPILE_GLOBAL_SETTINGS("Log Included")
#else
    YAGET_COMPILE_GLOBAL_SETTINGS("Log NOT Included")
#endif // YAGET_LOG_ENABLED

namespace
{
    std::set<std::string> GetTagSet()
    {
        const auto tags = yaget::ylog::GetRegisteredTags();
        std::set<std::string> result(tags.begin(), tags.end());;
        return result;
    }

}


using namespace yaget;

void ylog::Initialize(const args::Options& options)
{
    ylog::Config::Vector configList;
    for (auto&& it : dev::CurrentConfiguration().mDebug.mLogging.Outputs)
    {
        ylog::Config::addOutput(configList, it.first.c_str());
        for (auto&& o : it.second)
        {
            ylog::Config::setOption(configList, o.first.c_str(), o.second.c_str());
        }
    }

    ylog::Logger& logObject = ylog::Get();

    //DBUG, INFO, NOTE, WARN, EROR
    Log::Level level = ylog::Log::toLevel(dev::CurrentConfiguration().mDebug.mLogging.Level.c_str());
    logObject = ylog::Logger(logObject.getName().c_str());
    logObject.setLevel(level);

    // setup filters, which tags will be suppressed from log output
    Strings filters = dev::CurrentConfiguration().mDebug.mLogging.Filters;
    for (auto&& it: filters)
    {
        if (*it.begin() == '!')
        {
            std::string tag(it.begin() + 1, it.end());
            ylog::Manager::AddOverrideFilter(Tagger(tag.c_str()));
        }
        else
        {
            ylog::Manager::AddFilter(Tagger(it.c_str()));
        }
    }

    // Configure the Log Manager (create Output objects)
    ylog::Manager::configure(configList);

    // dump file with all registered tags using name and hash values
    if (options.find<bool>("log_write_tags", false))
    {
        const std::string fileName = util::ExpendEnv("$(LogFolder)/LogTags.txt", nullptr);
        std::ofstream logTagsFile(fileName.c_str());

        logTagsFile << "; Currently registered log tags\n";
        const auto& tags = GetTagSet();
        for (const auto& tag : tags)
        {
            logTagsFile << std::setw(4) << std::left << tag << " = " << std::setw(10) << std::right << LOG_TAG(tag.c_str()) << "\n";
        }
    }
}

ylog::Logger& ylog::Get()
{
    static ylog::Logger ApplicationLogger("yaget");
    return ApplicationLogger;
}
