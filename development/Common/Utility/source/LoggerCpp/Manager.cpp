/**
 * @file    Manager.cpp
 * @ingroup LoggerCpp
 * @brief   The static class that manage the registered channels and outputs
 *
 * Copyright (c) 2013 Sebastien Rombauts (sebastien.rombauts@gmail.com)
 *
 * Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
 * or copy at http://opensource.org/licenses/MIT)
 */

#include "LoggerCpp/Manager.h"
#include "LoggerCpp/Exception.h"
#include "LoggerCpp/OutputDebug.h"

#include "Logger/YLog.h"
#include "App/AppUtilities.h"
#include "StringHelpers.h"

#include <stdexcept>
#include <string>
#include <algorithm>
#include <fstream>

//#define YAGET_WRITE_LOG_TAGS

using namespace yaget;
using namespace yaget::ylog;

namespace yaget::ylog
{
    extern yaget::Strings GetRegisteredTags();
}

namespace
{
    // only to ensure proper initialization of static variables
    struct ManagerData
    {
        ManagerData()
        {
            // how do we process which tags are valid, if we read some kind of file (at compile)
            // how do we allow to extend tags from other libraries and executables 
            // without recompiling core libraries?
#ifdef YAGET_WRITE_LOG_TAGS
            std::string fileName = util::ExpendEnv("$(LogFolder)/LogTags.txt", nullptr);
            std::ofstream logTagsFile(fileName.c_str());
#endif // YAGET_WRITE_LOG_TAGS

            yaget::Strings tags = yaget::ylog::GetRegisteredTags();
            for (const auto& tag : tags)
            {
                mTags.insert(LOG_TAG(tag.c_str()));

#ifdef YAGET_WRITE_LOG_TAGS
                logTagsFile << std::setw(4) << std::left << tag << " = " << std::setw(10) << std::right << LOG_TAG(tag.c_str()) << std::endl;
#endif // YAGET_WRITE_LOG_TAGS
            }

#ifndef YAGET_SHIPPING
            ylog::Config::Vector configList;
            ylog::Config::addOutput(configList, "OutputDebug");
            ylog::Config::setOption(configList, "split_log", "true");

            Output::Ptr outputPtr(new OutputDebug(*configList.begin()));
            mOutputList.push_back(outputPtr);
#endif // YAGET_SHIPPING

        }

        Channel::Map mChannelMap;                       ///< Map of shared pointer of Channel objects
        Output::Vector mOutputList;                     ///< List of Output objects
        Log::Level mDefaultLevel = Log::Log::Level::eDebug;    ///< Default Log::Level of any new Channel

        
        using TagFilters_t = std::set<uint32_t>;
        TagFilters_t mTagFilters;
        TagFilters_t mOverriteTagFilters;
        TagFilters_t mTags;

        using OutputTypes = std::map<std::string, Manager::OutputCreator>;
        OutputTypes mRegisteredOutputTypes;
    };

    ManagerData& md()
    {
        static ManagerData managerData;
        return managerData;
    }


    bool DumpRegisteredTags(const ManagerData& tagData)
    {
        using namespace yaget;

        std::string settingsPath = util::ExpendEnv("$(LogFolder)/RegisteredTags.txt", nullptr);

        std::ofstream file(settingsPath.c_str());

        file << "; All registered log tags." << std::endl;
        for (const auto& it : tagData.mTags)
        {
            file << it << std::endl;
        }

        return true;
    }

} // namespace

/*static*/ void Manager::AddOutput(Output::Ptr outputPtr)
{
    auto& d = md();
    d.mOutputList.push_back(outputPtr);
}

/*static*/ void Manager::RegisterOutputType(const char* name, Manager::OutputCreator outputCreator)
{
    auto& d = md();
    d.mRegisteredOutputTypes.insert(std::make_pair(name, outputCreator));
}

/*static*/ void Manager::ResetRuntimeData()
{
    auto& d = md();
    auto outputTypes = std::move(d.mRegisteredOutputTypes);
    auto tags = std::move(d.mTags);

    d = {};

    d.mRegisteredOutputTypes = std::move(outputTypes);
    d.mTags = std::move(tags);
}

// Create and configure the Output objects.
void Manager::configure(const Config::Vector& aConfigList)
{
    auto& d = md();
    d.mOutputList.clear();

    const ManagerData::OutputTypes& registeredOutputs = d.mRegisteredOutputTypes;
    for (auto&& it : aConfigList)
    {
        const std::string& configName = it->getName();

        for (auto&& f : registeredOutputs)
        {
            if (f.first.find(configName) != std::string::npos)
            {
                Output::Ptr outputPtr(f.second(it));
                d.mOutputList.push_back(outputPtr);
                break;
            }
        }
    }
}

// Return the Channel corresponding to the provided name
Channel::Ptr Manager::get(const char* apChannelName)
{
    auto& d = md();
    ylog::Channel::Ptr ChannelPtr;
    ylog::Channel::Map::iterator iChannelPtr = d.mChannelMap.find(apChannelName);

    if (d.mChannelMap.end() != iChannelPtr)
    {
        ChannelPtr = iChannelPtr->second;
    }
    else
    {
        /// @todo Add a basic thread-safety security (throw if multiple threads create Loggers)
        ChannelPtr.reset(new ylog::Channel(apChannelName, d.mDefaultLevel));
        d.mChannelMap[apChannelName] = ChannelPtr;
    }

    return ChannelPtr;
}

// Output the Log to all the active Output objects.
void Manager::output(const ylog::Channel::Ptr& aChannelPtr, const ylog::Log& aLog)
{
    auto& d = md();
    Output::Vector::iterator iOutputPtr;

    for (iOutputPtr = d.mOutputList.begin(); iOutputPtr != d.mOutputList.end(); ++iOutputPtr)
    {
        (*iOutputPtr)->output(aChannelPtr, aLog);
    }
}

// Serialize the current Log::Level of Channel objects and return them as a Config instance
Config::Ptr Manager::getChannelConfig(void)
{
    auto& d = md();
    Config::Ptr ConfigPtr(new Config("ChannelConfig"));

    Channel::Map::const_iterator iChannel;
    for (iChannel = d.mChannelMap.begin(); iChannel != d.mChannelMap.end(); ++iChannel)
    {
        ConfigPtr->setValue(iChannel->first.c_str(), Log::toString(iChannel->second->getLevel()));
    }

    return ConfigPtr;
}

// Set the Log::Level of Channel objects from the provided Config instance
void Manager::setChannelConfig(const ylog::Config::Ptr& aConfigPtr)
{
    const Config::Values& ConfigValues = aConfigPtr->getValues();

    Config::Values::const_iterator iValue;
    for (iValue = ConfigValues.begin(); iValue != ConfigValues.end(); ++iValue)
    {
        Manager::get(iValue->first.c_str())->setLevel(Log::toLevel(iValue->second.c_str()));
    }
}
            
void Manager::setDefaultLevel(Log::Level aLevel)
{
    auto& d = md();
    d.mDefaultLevel = aLevel;
}

bool Manager::IsValidTag(uint32_t tag)
{
    const auto& d = md();
    return d.mTags.find(tag) != std::end(d.mTags);
}

bool Manager::IsFilter(uint32_t tag)
{
    auto& d = md();
    ManagerData::TagFilters_t::const_iterator it = d.mTagFilters.find(tag);
    return it != d.mTagFilters.end();
}

bool Manager::IsSeverityFilter(ylog::Log::Level severity, uint32_t /*tag*/)
{
    return severity < ylog::Log::Level::eError;
}

void Manager::AddFilter(uint32_t tag)
{
    auto& d = md();
    d.mTagFilters.insert(tag);
}

bool Manager::IsOverrideFilter(uint32_t tag)
{
    auto& d = md();
    ManagerData::TagFilters_t::const_iterator it = d.mOverriteTagFilters.find(tag);
    return it != d.mOverriteTagFilters.end();
}

void Manager::AddOverrideFilter(uint32_t tag)
{
    auto& d = md();
    d.mOverriteTagFilters.insert(tag);
}

void Manager::RemoveFilter(uint32_t tag)
{
    auto& d = md();
    d.mTagFilters.erase(tag);
}
