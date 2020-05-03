//////////////////////////////////////////////////////////////////////
// HashUtilities.h
//
//  Copyright 10/9/2019 Edgar Glowacki
//
//  Maintained by: Edgar
//
//  NOTES:
//      Provides hash utilities in use with std algorithm and data structures
//      It also provides helpers for yaget libraries
//
//
//  #include "HashUtilities.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "YagetCore.h"


namespace yaget::conv
{

    template <typename... Args>
    std::size_t GenerateHash(Args& ... args)
    {
        using Sources = std::tuple<Args...>;
        static constexpr size_t NumSources = std::tuple_size_v<Sources>;

        Sources sources(args...);
        using WorkBuffer = std::array<char, sizeof(Sources)>;
        WorkBuffer workBuffer;
        std::size_t currentIndex = 0;

        yaget::meta::for_each(sources, [&workBuffer, &currentIndex](const auto& source)
        {
            std::memcpy(&workBuffer[currentIndex], &source, sizeof(source));
            currentIndex += sizeof(source);
        });

        std::hash<WorkBuffer> hash_fn;
        return hash_fn(workBuffer);
    }

} // namespace yaget::conv


namespace std
{
    // Hash function for T pointer
    template<class T>
    struct hash<const T*>
    {
        typedef const T* argument_type;
        typedef std::size_t   result_type;
        inline result_type operator()(argument_type value) const
        {
            // Implementation by Alberto Barbati and Dave Harris.
            std::size_t x = static_cast<std::size_t>(reinterpret_cast<std::ptrdiff_t>(value));
            return x + (x >> 3);
        }
    };

    // Hash function for std::array of chars
    template <std::size_t S>
    struct hash<std::array<char, S>>
    {
        size_t operator()(const std::array<char, S>& bufferData) const
        {
            size_t result = 0;
            const size_t prime = 31;
            for (auto it : bufferData)
            {
                result = it + (result * prime);
            }

            return result;
        }
    };

    // trying out hash on char * terminated by null
    template <>
    struct hash<char*>
    {
        size_t operator()(const char* s) const
        {
            // http://www.cse.yorku.ca/~oz/hash.html
            size_t h = 5381;
            int c;
            while ((c = *s++))
                h = ((h << 5) + h) + c;
            return h;
        }
    };

} // namespace std
