///////////////////////////////////////////////////////////////////////
// AssetCache.h
//
//  Copyright 01/18/2026 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//
//  #include "Render/Cache/AssetCache.h"
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
        //Vertex...    = 1ULL << 5,
        //Vertex..     = 1ULL << 6,
        //Vertex..     = 1ULL << 7,
        //Vertex..     = 1ULL << 8,
        //Vertex..     = 1ULL << 9,

        // describe pixel shader
        PixelColor    = 1ULL << 10,
        PixelTexture0 = 1ULL << 11,
        PixelTexture1 = 1ULL << 12,
        //Pixel...    = 1ULL << 13,
        //Pixel...    = 1ULL << 14,
        //Pixel...    = 1ULL << 15,
        //Pixel...    = 1ULL << 16,
        //Pixel...    = 1ULL << 17,
        //Pixel...    = 1ULL << 18,
        //Pixel...    = 1ULL << 19,

        // describe root signature
        //SigConstMatrix  = 1ULL << 20,
        //Sig...        = 1ULL << 21,
        //Sig...        = 1ULL << 22,
        //Sig...        = 1ULL << 23,
        //Sig...        = 1ULL << 24,
        //Sig...        = 1ULL << 25,
        //Sig...        = 1ULL << 26,
        //Sig...        = 1ULL << 27,
        //Sig...        = 1ULL << 28,
        //Sig...        = 1ULL << 29,

        // describe pipeline state blend
        BlendModeOpaque           = 1ULL << 30,
        BlendModeAlpha            = 1ULL << 31,
        BlendModeAdditive         = 1ULL << 32,
        BlendModeNonPremultiplied = 1ULL << 33,
        //BlendMode...            = 1ULL << 34,

        // describe pipeline state rasterizer, uses MaskRasterizerState
        RasterizerStateClockwise        = 1ULL << 35,
        RasterizerStateCounterClockwise = 1ULL << 36,
        RasterizerStateWireframe        = 1ULL << 37,
        //RasterizerState...            = 1ULL << 38,
        //RasterizerState...            = 1ULL << 39,

        // describe pipeline state depth stencil, uses MaskDepthState
        DepthStateNone    = 1ULL << 40,
        DepthStateOn      = 1ULL << 41,
        DepthStateStencil = 1ULL << 42,
        //DepthState...   = 1ULL << 43,
        //DepthState...   = 1ULL << 44,

        // describe pipeline state primitive topology
        TopologyStatePoint    = 1ULL << 45,
        TopologyStateLine     = 1ULL << 46,
        TopologyStateTriangle = 1ULL << 47,
        //TopologyState...    = 1ULL << 48,
        //TopologyState...    = 1ULL << 49,

        // describe pipeline state render target formats
        NumRTVTargetsOne   = 1ULL << 50,
        NumRTVTargetsTwo   = 1ULL << 51,
        NumRTVTargetsThree = 1ULL << 52,
        NumRTVTargetsFour  = 1ULL << 53,
        //NumRTVTargets... = 1ULL << 54,

        // describe pipeline state render target formats
        RTVFormatRGBA8   = 1ULL << 55,
        RTVFormatRGBA16F = 1ULL << 56,
        RTVFormatRGBA32F = 1ULL << 57,
        DSVFormatD24S8   = 1ULL << 58,
        //DSVFormat...   = 1ULL << 59,
        //DXGI_FORMAT_B5G6R5_UNORM = 59,
        //DXGI_FORMAT_B5G5R5A1_UNORM = 60,


        Reserved60 = 1ULL << 60,
        Reserved61 = 1ULL << 61,
        Reserved62 = 1ULL << 62,
        Empty      = 1ULL << 63,
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
    static AssetCacheType BasicSignature = BasicVertex | BasicPixel;
    static AssetCacheType BasicPipeline = BasicSignature |
        AssetCacheType::RasterizerStateCounterClockwise |
        AssetCacheType::BlendModeOpaque | AssetCacheType::DepthStateNone |
        AssetCacheType::TopologyStateTriangle | AssetCacheType::RTVFormatRGBA8;

    //-------------------------------------------------------------------------------------------------
    class AssetCache
    {
    public:
        using Section = io::VirtualTransportSystem::Section;  

        AssetCache(io::VirtualTransportSystem& vts, Section fileName);
        ~AssetCache();
        io::Buffer GetCachedAsset(const io::Tag& tag) const;
        bool IsCachedAsset(const io::Tag& tag) const;
        void SaveCachedAsset(const io::Tag& tag, io::Buffer buffer);
        void ClearCachedAsset(const io::Tag& tag);
        static Section operator[](AssetCacheType typeFlag);
        static AssetCacheType operator[](const Section& section);
        static void PopulateTypeToSection(const Section& fileName, io::VirtualTransportSystem& vts);
        static void SaveTypeToSection(const Section& fileName, io::VirtualTransportSystem& vts);

    private:
        io::VirtualTransportSystem& mVTS;
        size_t mCacheHash{};

        // map of where the blob of dats is located (.second - offset)
        struct Location
        {
            size_t mOffset = 0;
            size_t mSize = 0;
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
        Section mCacheSection;

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

