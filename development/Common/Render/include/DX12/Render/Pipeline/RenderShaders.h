///////////////////////////////////////////////////////////////////////
// RenderShader.h
//
//  Copyright 01/17/2026 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//
//  #include "Render/Pipeline/RenderShaders.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once
#include "Render/RenderCore.h"
#include "Streams/Buffers.h"
#include "VTS/VirtualTransportSystem.h"
#include "Render/Cache/CacheWatcher.h"
#include <d3d12.h>


namespace yaget
{
    class DependencyGraph;
}

namespace yaget::render
{
    class ResourceReflector;
    class ResourceCompiler;

    //-------------------------------------------------------------------------------------------------
    class RenderShaders : public CacheWatcher<io::Buffer>
    {
    public:
        RenderShaders(io::VirtualTransportSystem& vts);
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

        void ClearCache(const io::Tag& tag);

        struct ShaderPin
        {
            std::string mName;
            uint32_t mIndex = 0;
        };
        using ShaderPins = std::vector<ShaderPin>;

        using IndexMap = std::map<std::string, uint32_t>;
        struct RootDescResult : public NoCopy
        {
            std::vector<D3D12_ROOT_PARAMETER1> mRootParameters;
            IndexMap mIndexMap;
            D3D12_VERSIONED_ROOT_SIGNATURE_DESC mRootSignatureDesc{};
        };

        using DescriptionCallback = std::function<void(const RootDescResult& descResult)>;
        void CreateSignatureDescription(const io::Tag& vertexTag, const io::Tag& pixelTag, DescriptionCallback callback);

        static void PopulateShaderMappings(io::VirtualTransportSystem::Section fileName, io::VirtualTransportSystem& vts);
        static void SaveShaderMappings(io::VirtualTransportSystem::Section fileName, io::VirtualTransportSystem& vts);

    private:
        io::Buffer AssureShaderNonMT(const yaget::io::Tag& tag, ShaderType shaderType);

        std::shared_ptr<ResourceCompiler> mResourceCompiler;
        std::map<io::Tag, std::shared_ptr<ResourceReflector>> mReflections;
    };

}
