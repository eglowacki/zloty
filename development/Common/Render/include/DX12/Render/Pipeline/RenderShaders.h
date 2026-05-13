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
#include "Components/GameSystem.h"
#include "Render/Cache/CacheWatcher.h"
#include "Render/RenderCore.h"
#include "Streams/Buffers.h"
#include "VTS/VirtualTransportSystem.h"

#include <d3dx12.h> // NOTE(eg) we need to figure out how to not include this dx12 header file



namespace yaget::render
{
    class ResourceReflector;
    class ResourceCompiler;

    namespace constant_shader_types
    {
        enum class ConstantTypes
        {
            WorldViewProjection,
            Time,
            Texture2d,
            Texture2dSecond,
            Texture2dThird,
            Texture2dFourth,
            Texture3d,
            Texture2dNormal,
            Sampler, 
            GeometryData
        };

        enum class ConstantLayout
        {
            Matrix4x4,
            Float,
            Float4,
            Int,
            Int4,
            Pixels,
            Struct
        };

        constexpr size_t UnnamedSize = std::numeric_limits<size_t>::max();
        size_t GetConstantLayoutSize(ConstantLayout constantLayout);

        enum class RootType
        {
            Table,
            Constant,
            ConstantBufferView,
            ShaderResourceView,
            UnorderedAccessView
        };

        struct VariableType
        {
            ConstantTypes mType;
            ConstantLayout mLayout;
            RootType mRootType;
            std::string mTypeName;
            std::string mVariableName;
            uint32_t mOffset;
        };
    }


    //-------------------------------------------------------------------------------------------------
    class RenderShaders : public CacheWatcher<io::Buffer>
    {
    public:
        RenderShaders(io::VirtualTransportSystem& vts, io::VirtualTransportSystem::Section fileName);
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
        std::vector<io::Buffer> GetShaders(const io::Tags& tags, ShaderType shaderType, comp::gs::mt::InitCounter* counter);

        void Preload(const io::Tags& tags, ShaderType shaderType, comp::gs::mt::InitCounter& counter);

        void ClearCache(const io::Tag& tag);

        struct ShaderPin
        {
            std::string mName;
            uint32_t mIndex = 0;
        };

        using ShaderPins = std::vector<ShaderPin>;

        using IndexMap = std::map<std::string, constant_shader_types::VariableType>;

        struct RootDescResult : NoCopy
        {
            std::vector<D3D12_ROOT_PARAMETER1> mRootParameters;
            std::vector<D3D12_STATIC_SAMPLER_DESC> mSamplerParameters;
            IndexMap mIndexMap;
            IndexMap mIndexSamplerMap;
            D3D12_VERSIONED_ROOT_SIGNATURE_DESC mRootSignatureDesc{};
        };

        struct ShaderInputOutputPins
        {
            ShaderPins mInputPins;
            ShaderPins mOutputPins;
        };

        ShaderInputOutputPins GetShaderPins(const io::Tag& tag);

        using DescriptionCallback = std::function<void(const RootDescResult& descResult)>;
        void CreateSignatureDescription(const io::Tag& vertexTag, const io::Tag& pixelTag, DescriptionCallback callback);

        static void PopulateMappings(io::VirtualTransportSystem::Section fileName, io::VirtualTransportSystem& vts);
        static void SaveMappings(io::VirtualTransportSystem::Section fileName, io::VirtualTransportSystem& vts);

        static void PopulateReflectorMappings(io::VirtualTransportSystem::Section fileName, io::VirtualTransportSystem& vts);
        static void SaveReflectorMappings(io::VirtualTransportSystem::Section fileName, io::VirtualTransportSystem& vts);

    private:
        io::Buffer AssureShaderNonMT(const io::Tag& tag, ShaderType shaderType);

        std::shared_ptr<ResourceCompiler> mResourceCompiler;
        std::map<io::Tag, std::shared_ptr<ResourceReflector>> mReflections;
    };

}
