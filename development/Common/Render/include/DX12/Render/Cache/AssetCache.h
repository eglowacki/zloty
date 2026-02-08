///////////////////////////////////////////////////////////////////////
// AssetCache.h
//
//  Copyright 01/18/2026 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//
//  #include "Renders/Cache/AssetCache.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once
#include "Streams/Buffers.h"
#include "VTS/VirtualTransportSystem.h"
#include <magic_enum/magic_enum.hpp>


namespace yaget::render
{
    enum class AssetCacheType : uint64_t;
}


template <>
struct magic_enum::customize::enum_range<yaget::render::AssetCacheType> {
  static constexpr bool is_flags = true;
};


namespace yaget::render
{
    //-------------------------------------------------------------------------------------------------
    enum class AssetCacheType : uint64_t
    {
        // describe vertex shader
        VertexPosition = 1ULL << 0,
        VertexTexture0 = 1ULL << 1,
        VertexTexture1 = 1ULL << 2,
        VertexNormal   = 1ULL << 3,
        VertexColor    = 1ULL << 4,

        // describe pixel shader
        PixelColor    = 1ULL << 5,
        PixelTexture0 = 1ULL << 6,
        PixelTexture1 = 1ULL << 7,
        PixelBlah1    = 1ULL << 8,
        PixelBlah2    = 1ULL << 9,
        PixelBlah3    = 1ULL << 10,

        // describe root signature
        SigConstMatrix  = 1ULL << 11,
        SigPlaceholder1 = 1ULL << 12,
        SigPlaceholder2 = 1ULL << 13,
        SigPlaceholder3 = 1ULL << 14,
        SigPlaceholder4 = 1ULL << 15,

        // describe pipeline state rasterizer, uses MaskRasterizerState
        RasterizerStateNone             = 1ULL << 16,
        RasterizerStateClockwise        = 1ULL << 17,
        RasterizerStateCounterClockwise = 1ULL << 18,
        RasterizerStateWireframe        = 1ULL << 19,

        // describe pipeline state blend
        BlendModeOpaque           = 1ULL << 20,
        BlendModeAlpha            = 1ULL << 21,
        BlendModeAdditive         = 1ULL << 22,
        BlendModeNonPremultiplied = 1ULL << 23,

        // describe pipeline state depth stencil, uses MaskDepthState
        DepthStateNone    = 1ULL << 24,
        DepthStateDefault = 1ULL << 25,
        DepthStateRead    = 1ULL << 26,
        //DepthStateRead = 1ULL << 27,

        // describe pipeline state primitive topology
        TopologyStateTriangle = 1ULL << 28,
        TopologyStateLine     = 1ULL << 29,
        TopologyStatePoint    = 1ULL << 30,
        //TopologyStateNone = 1ULL << 31,

        // describe pipeline state render target formats
        NumRTVTargetsOne    = 1ULL << 33,
        NumRTVTargetsTwo    = 1ULL << 34,
        NumRTVTargetsThree  = 1ULL << 35,
        NumRTVTargetsFour   = 1ULL << 36,

        // describe pipeline state render target formats
        RTVFormatRGBA8   = 1ULL << 40,
        RTVFormatRGBA16F = 1ULL << 41,
        RTVFormatRGBA32F = 1ULL << 42,
        DSVFormatD24S8   = 1ULL << 43,
        DSVFormatBlah1   = 1ULL << 44,
        DSVFormatBlah2   = 1ULL << 45,
        DSVFormatBlah3   = 1ULL << 46,
        DSVFormatBlah4   = 1ULL << 47,
        DSVFormatBlah5   = 1ULL << 48,
        DSVFormatBlah6   = 1ULL << 49,
        DSVFormatBlah7   = 1ULL << 50,
    };


    //------------------------------------------------------------------------------------------------
    template <typename E>
    constexpr E ored(E lhs, E rhs)
    {
        return static_cast<E>(
            static_cast<uint64_t>(lhs) | static_cast<uint64_t>(rhs));
    }


    //------------------------------------------------------------------------------------------------
    template <typename E>
    constexpr E anded(E lhs, E rhs)
    {
        return static_cast<E>(
            static_cast<uint64_t>(lhs) & static_cast<uint64_t>(rhs));
    }


    //------------------------------------------------------------------------------------------------
    // Overload the bitwise OR operator for CarOptions
    constexpr AssetCacheType operator|(AssetCacheType lhs, AssetCacheType rhs)
    {
        return ored(lhs, rhs);
    }


    //------------------------------------------------------------------------------------------------
    // Overload bitwise AND operator (and others like &, ^, ~, etc.)
    constexpr AssetCacheType operator&(AssetCacheType lhs, AssetCacheType rhs)
    {
        return anded(lhs, rhs);
    }


    static AssetCacheType BasicVertex = AssetCacheType::VertexPosition | AssetCacheType::VertexColor;
    static AssetCacheType BasicPixel = AssetCacheType::PixelColor;
    static AssetCacheType BasicSignature = BasicVertex | BasicPixel | AssetCacheType::SigConstMatrix;
    static AssetCacheType BasicPipeline = BasicSignature |
        AssetCacheType::RasterizerStateCounterClockwise |
        AssetCacheType::BlendModeOpaque | AssetCacheType::DepthStateNone |
        AssetCacheType::TopologyStateTriangle | AssetCacheType::RTVFormatRGBA8;

    //-------------------------------------------------------------------------------------------------
    class AssetCache
    {
    public:
        AssetCache(io::VirtualTransportSystem& vts, io::VirtualTransportSystem::Section fileName);
        ~AssetCache();
        io::Buffer GetCachedAsset(const io::Tag& tag) const;
        void SaveCachedAsset(const io::Tag& tag, io::Buffer buffer);
        void ClearCachedAsset(const io::Tag& tag);
        static io::VirtualTransportSystem::Section operator[](AssetCacheType typeFlag);
        static void PopulateTypeToSection(io::VirtualTransportSystem::Section fileName, io::VirtualTransportSystem& vts);
        static void SaveTypeToSection(io::VirtualTransportSystem::Section fileName, io::VirtualTransportSystem& vts);

    private:
        io::VirtualTransportSystem& mVTS;
        size_t mCacheHash{};

        // map of where the blob of dats is located (.second - offset)
        struct Location
        {
            size_t mOffset{};
            size_t mSize{};
        };

        std::map<Guid, Location> mCacheIndex;
        io::MessagingBuffer mCache;

        enum class CacheStatus
        {
            Clean = 1 << 0,
            Dirty = 1 << 1,
            Holes = 1 << 2,
        };

        CacheStatus mCacheStatus = CacheStatus::Clean;
        io::VirtualTransportSystem::Section mCacheSection;

        using TypeToSectionMap = std::map<AssetCacheType, io::VirtualTransportSystem::Section>;
        static TypeToSectionMap TypeToSection;
    };

    namespace internal
    {
        inline std::string CacheTypeToString(yaget::render::AssetCacheType value)
        {
            std::string result;

            auto temp_value = static_cast<uint64_t>(value); // Work with a copy

            while (temp_value != 0)
            {
                // The expression (temp_value & -temp_value) isolates the least significant set bit
                uint64_t lsb = temp_value & (-static_cast<int64_t>(temp_value));

                auto singleValue = static_cast<yaget::render::AssetCacheType>(lsb);
                const auto enumName = magic_enum::enum_name(singleValue);

                result += result.empty() ? enumName : " | " + std::string(enumName);

                // Clear the least significant set bit
                temp_value ^= lsb;
                // Alternatively, a common trick to clear the LSB: temp_value &= (temp_value - 1);
            }

            return result;
        }

    }

    inline void to_json(nlohmann::json& j, const AssetCacheType cacheType)
    {
        const auto enumName = internal::CacheTypeToString(cacheType);
        j = enumName;
    }


    inline void from_json(const nlohmann::json& j, AssetCacheType& cacheType)
    {
        std::string source;
        j.get_to(source);

        AssetCacheType result{};
        auto bitValues = conv::Split(source, "|", true);
        for (auto bit : bitValues)
        {
            auto enumBit = magic_enum::enum_cast<AssetCacheType>(bit);
            if (enumBit.has_value())
            {
                result = result | enumBit.value();
            }
        }

        cacheType = result;
    }

}

