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


namespace yaget::render
{
    //-------------------------------------------------------------------------------------------------
    // can you fix enum AssetCacheType?
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
        TopologyStateTriangle = 1ULL << 24,
        TopologyStateLine     = 1ULL << 24,
        TopologyStatePoint    = 1ULL << 24,
        //TopologyStateNone = 1ULL << 25,

        // describe pipeline state render target formats
        NumRTVTargetsOne = 1ULL << 26,
        NumRTVTargetsTwo = 1ULL << 27,
        //NumRTVTargetsThree = 1ULL << 28,
        //NumRTVTargetsFour = 1ULL << 29,

        // describe pipeline state render target formats
        RTVFormatRGBA8   = 1ULL << 30,
        RTVFormatRGBA16F = 1ULL << 31,
        RTVFormatRGBA32F = 1ULL << 32,
        DSVFormatD24S8   = 1ULL << 33,
        DSVFormatBlah1   = 1ULL << 34,
        DSVFormatBlah2   = 1ULL << 35,
        DSVFormatBlah3   = 1ULL << 36,
        DSVFormatBlah4   = 1ULL << 37,
        DSVFormatBlah5   = 1ULL << 38,
        DSVFormatBlah6   = 1ULL << 39,
        DSVFormatBlah7   = 1ULL << 40,
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
        static yaget::io::VirtualTransportSystem::Section operator[](AssetCacheType typeFlag);

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
        static std::map<AssetCacheType, yaget::io::VirtualTransportSystem::Section> TypeToSection;
    };
}
