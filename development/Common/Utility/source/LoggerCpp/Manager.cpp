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

#include <string>
#include <algorithm>
#include <fstream>
#include <set>

namespace
{
    // only to ensure proper initialization of static variables
    struct ManagerData
    {
        ManagerData()
        {
            using namespace yaget;

            yaget::Strings tags = yaget::ylog::GetRegisteredTags();
            for (const auto& tag : tags)
            {
                mTags.insert(LOG_TAG(tag.c_str()));
            }

#ifndef YAGET_SHIPPING
            ylog::Config::Vector configList;
            ylog::Config::addOutput(configList, "OutputDebug");
            ylog::Config::setOption(configList, "split_lines", "true");

            ylog::Output::Ptr outputPtr(new ylog::OutputDebug(*configList.begin()));
            mOutputList.push_back(outputPtr);
#endif // YAGET_SHIPPING
        }

        // return true if tag is filtered (do not show)
        bool IsFilter(uint32_t tag) const
        {
            const auto it = mTagFilters.find(tag);
            return it != mTagFilters.end();
        }

        void AddFilter(uint32_t tag)
        {
            mTagFilters.insert(tag);
        }

        void RemoveFilter(uint32_t tag)
        {
            mTagFilters.erase(tag);
        }

        bool IsOverrideFilter(uint32_t tag) const
        {
            const auto it = mOverriteTagFilters.find(tag);
            return it != mOverriteTagFilters.end();
        }

        void AddOverrideFilter(uint32_t tag)
        {
            mOverriteTagFilters.insert(tag);
        }

        yaget::ylog::Channel::Map mChannelMap;                       ///< Map of shared pointer of Channel objects
        yaget::ylog::Output::Vector mOutputList;                     ///< List of Output objects
        yaget::ylog::Log::Level mDefaultLevel = yaget::ylog::Log::Log::Level::eDebug;    ///< Default Log::Level of any new Channel
        
        using TagFilters_t = std::set<uint32_t>;
        TagFilters_t mTags;

        using OutputTypes = std::map<std::string, yaget::ylog::Manager::OutputCreator>;
        OutputTypes mRegisteredOutputTypes;

    private:
        TagFilters_t mTagFilters;
        TagFilters_t mOverriteTagFilters;
    };

    ManagerData& md()
    {
        static ManagerData managerData;
        return managerData;
    }

} // namespace


/*static*/ void yaget::ylog::Manager::AddOutput(Output::Ptr outputPtr)
{
    auto& d = md();
    d.mOutputList.push_back(outputPtr);
}

/*static*/ void yaget::ylog::Manager::RegisterOutputType(const char* name, Manager::OutputCreator outputCreator)
{
    auto& d = md();
    d.mRegisteredOutputTypes.insert(std::make_pair(name, outputCreator));
}

/*static*/ void yaget::ylog::Manager::ResetRuntimeData()
{
    auto& d = md();
    auto outputTypes = std::move(d.mRegisteredOutputTypes);
    auto tags = std::move(d.mTags);

    d = {};

    d.mRegisteredOutputTypes = std::move(outputTypes);
    d.mTags = std::move(tags);
}

// Create and configure the Output objects.
void yaget::ylog::Manager::configure(const Config::Vector& aConfigList)
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
yaget::ylog::Channel::Ptr yaget::ylog::Manager::get(const char* apChannelName)
{
    auto& d = md();
    ylog::Channel::Ptr ChannelPtr;

    if (const auto iChannelPtr = d.mChannelMap.find(apChannelName); d.mChannelMap.end() != iChannelPtr)
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
void yaget::ylog::Manager::output(const ylog::Channel::Ptr& aChannelPtr, const ylog::Log& aLog)
{
    auto& d = md();

    for (auto iOutputPtr = d.mOutputList.begin(); iOutputPtr != d.mOutputList.end(); ++iOutputPtr)
    {
        (*iOutputPtr)->output(aChannelPtr, aLog);
    }
}

// Serialize the current Log::Level of Channel objects and return them as a Config instance
yaget::ylog::Config::Ptr yaget::ylog::Manager::getChannelConfig(void)
{
    auto& d = md();
    Config::Ptr ConfigPtr(new Config("ChannelConfig"));

    for (auto iChannel = d.mChannelMap.begin(); iChannel != d.mChannelMap.end(); ++iChannel)
    {
        ConfigPtr->setValue(iChannel->first.c_str(), Log::toString(iChannel->second->getLevel()));
    }

    return ConfigPtr;
}

// Set the Log::Level of Channel objects from the provided Config instance
void yaget::ylog::Manager::setChannelConfig(const ylog::Config::Ptr& aConfigPtr)
{
    const Config::Values& ConfigValues = aConfigPtr->getValues();

    for (auto iValue = ConfigValues.begin(); iValue != ConfigValues.end(); ++iValue)
    {
        Manager::get(iValue->first.c_str())->setLevel(Log::toLevel(iValue->second.c_str()));
    }
}
            
void yaget::ylog::Manager::setDefaultLevel(Log::Level aLevel)
{
    auto& d = md();
    d.mDefaultLevel = aLevel;
}

bool yaget::ylog::Manager::IsValidTag(uint32_t tag)
{
    const auto& d = md();
    return d.mTags.find(tag) != std::end(d.mTags);
}

bool yaget::ylog::Manager::IsFilter(uint32_t tag)
{
    const auto& d = md();
    return d.IsFilter(tag);
}

bool yaget::ylog::Manager::IsSeverityFilter(ylog::Log::Level severity, uint32_t /*tag*/)
{
    return severity < ylog::Log::Level::eError;
}

void yaget::ylog::Manager::AddFilter(uint32_t tag)
{
    auto& d = md();
    d.AddFilter(tag);
}

bool yaget::ylog::Manager::IsOverrideFilter(uint32_t tag)
{
    const auto& d = md();
    return d.IsOverrideFilter(tag);
}

void yaget::ylog::Manager::AddOverrideFilter(uint32_t tag)
{
    auto& d = md();
    d.AddOverrideFilter(tag);
}

void yaget::ylog::Manager::RemoveFilter(uint32_t tag)
{
    auto& d = md();
    d.RemoveFilter(tag);
}
