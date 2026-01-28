///////////////////////////////////////////////////////////////////////
// RenderShader.h
//
//  Copyright 01/17/2026 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//
//  #include "Renders/RenderShaders.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once
#include "Render/RenderCore.h"
#include "Streams/Buffers.h"
#include "VTS/VirtualTransportSystem.h"
#include "Render/Cache/CacheWatcher.h"


namespace yaget
{
    class DependencyGraph;
}

namespace defensor::render
{
    using namespace yaget;

    //-------------------------------------------------------------------------------------------------
    class RenderShaders : public yaget::render::CacheWatcher<io::Buffer>
    {
    public:
        RenderShaders(io::VirtualTransportSystem& vts, yaget::DependencyGraph& dependencyGraph, io::Watcher& watcher);
        ~RenderShaders();

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
        io::Buffer AssureShaderNonMT(const yaget::io::Tag& tag, ShaderType shaderType);
    };

}
