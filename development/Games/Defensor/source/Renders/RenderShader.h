///////////////////////////////////////////////////////////////////////
// RenderShader.h
//
//  Copyright 01/17/2026 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//
//  #include "Renders/RenderShader.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

//#include <d3dcommon.h>
#include "Render/RenderCore.h"
#include "Streams/Buffers.h"
#include "VTS/VirtualTransportSystem.h"
#include "Render/Cache/AssetCache.h"
#include "Streams/Buffers.h"


namespace defensor::render
{
    using namespace yaget;

    ////-------------------------------------------------------------------------------------------------
    //class ShaderCache
    //{
    //public:
    //    ShaderCache(io::VirtualTransportSystem& vts, io::VirtualTransportSystem::Section fileName);
    //    ~ShaderCache();

    //    io::Buffer GetShader(const io::Tag& tag) const;
    //    void SaveShader(const io::Tag& tag, io::Buffer buffer);

    //private:
    //    io::VirtualTransportSystem& mVTS;
    //    size_t mCacheHash{};

    //    // map of where the blob of dats is located (.second - offset)
    //    // wirhin mCache
    //    struct Location
    //    {
    //        size_t mOffset{};
    //        size_t mSize{};
    //    };
    //    std::map<Guid, Location> mCacheIndex;
    //    io::MessagingBuffer mCache;
    //    bool mCacheDirty = false;

    //    io::VirtualTransportSystem::Section mCacheSection;
    //};


    //-------------------------------------------------------------------------------------------------
    class RenderShader
    {
    public:
        RenderShader(io::VirtualTransportSystem& vts);
        ~RenderShader();

        enum class ShaderType
        {
            Vertex,
            Pixel,
            Geometry,
            Compute,
            Hull,
            Domain
        };

        io::Buffer GetShader(const io::Tag& tag, ShaderType shaderType);
        std::vector<io::Buffer> GetShaders(const io::Tags& tags, ShaderType shaderType);

    private:
        std::map<io::Tag, io::Buffer> mShaders;
        io::VirtualTransportSystem& mVTS;

        yaget::render::AssetCache mCache;
    };

}
