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

#include "Streams/Watcher.h"
#include "Render/RenderCore.h"
#include "Streams/Buffers.h"
#include "VTS/VirtualTransportSystem.h"
#include "Render/Cache/CacheWatcher.h"


namespace defensor::render
{
    using namespace yaget;

    //-------------------------------------------------------------------------------------------------
    class RenderShader : public yaget::render::CacheWatcher
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
        void CachedAssetChanged(const io::Tag& tag);

        std::map<io::Tag, io::Buffer> mShaders;
    };

}
