//////////////////////////////////////////////////////////////////////
// Aplication.h
//
//  Copyright 7/17/2018 Edgar Glowacki
//
//  Maintained by: Edgar
//
//  NOTES:
//      notes_missing
//
//
//  #include "Json/JsonHelpers.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "YagetCore.h"
#include "nlohmann/json.hpp"


namespace yaget::json
{
    //-------------------------------------------------------------------------------------------------------------------------------
    //! convenience function to find json [sectionA][sectionB]. If sectionA empty, return false. if sectionB empty, it will only check for sectionA then sectionB
    inline bool IsSectionValid(const nlohmann::json& jasonBlock, const std::string& sectionA, const std::string& sectionB)
    {
        if (sectionA.empty())
        {
            return false;
        }

        return jasonBlock.find(sectionA) != jasonBlock.end() && (!sectionB.empty() ? jasonBlock[sectionA].find(sectionB) != jasonBlock[sectionA].end() : true);
    }

    //-------------------------------------------------------------------------------------------------------------------------------
    //! Return reference to requested section block. Asserts on invalid sections.
    inline const nlohmann::json& GetSection(const nlohmann::json& jasonBlock, const std::string& sectionA, const std::string& sectionB = "")
    {
        YAGET_ASSERT(IsSectionValid(jasonBlock, sectionA, sectionB), "Invalid Section '%s/%s' requested.", sectionA, sectionB);

        const auto it = jasonBlock.find(sectionA);
        if (!sectionB.empty())
        {
            const auto itB = (*it).find(sectionB);
            return std::cref(*itB);
        }

        return std::cref(*it);
    }

    //-------------------------------------------------------------------------------------------------------------------------------
    //! convenience function to look for a key and return value (from json block), or default one if does not exist
    template<typename T>
    inline T GetValue(const nlohmann::json& block, const char* key, const T& defaultValue)
    {
        if (const auto it = block.find(key); it != block.end())
        {
            return (*it).get<T>();
        }

        return defaultValue;
    }

    //-------------------------------------------------------------------------------------------------------------------------------
    //! convenience function to look for a key and return value (from json block), or default one if does not exist, it uses combiner
    //! to return new value, like merging two lists, adding values, etc
    template<typename T>
    inline T GetValue(const nlohmann::json& block, const char* key, const T& defaultValue, std::function<T(const T & valueNew, const T & valueOld)> combiner)
    {
        if (const auto it = block.find(key); it != block.end())
        {
            return combiner((*it).get<T>(), defaultValue);
        }

        return defaultValue;
    }

    //-------------------------------------------------------------------------------------------------------------------------------
    template<typename T>
    inline T GetValue(const nlohmann::json& block)
    {
        return block.get<T>();
    }

    //-------------------------------------------------------------------------------------------------------------------------------
    inline std::string PrettyPrint(const nlohmann::json& jasonBlock, int indent = 4)
    {
        std::ostringstream s;
        s << jasonBlock.dump(indent) << std::endl;
        return s.str();
    }

    //-------------------------------------------------------------------------------------------------------------------------------
    // parse txt in the format:
    // Debug.Metrics.TraceOn=false
    // Debug.Metrics.TraceFile='Hello World'
    // Debug.Metrics.Timeout=2.4f
    // Debug.Metrics.Ticks=40
    nlohmann::json ParseConfig(const std::string& text);

} // namespace yaget::json

