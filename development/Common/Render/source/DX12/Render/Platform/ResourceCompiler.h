// ResourceCompiler.h
//
//  Copyright 08/29/2022 Edgar Glowacki.
//
// NOTES:
//     Exposes compiler facility
//
// #include "Render/Platform/ResourceCompiler.h"
//
/////////////////////////////////////////////////////////////////////////
//! \file

#pragma once

#include <d3d12shader.h>
#include <d3dx12_root_signature.h>

#include "D3D12MemAlloc.h"
#include "Render/RenderCore.h"
#include "Streams/Buffers.h"
#include "Render/Pipeline/RenderShaders.h"

struct IDxcCompiler3;
struct IDxcUtils;
struct IDxcResult;
struct ID3D12LibraryReflection;
struct ID3D12ShaderReflection;


namespace yaget::render
{
    //-------------------------------------------------------------------------------------------------
    class ResourceReflector
    {
    public:
        using Ptr = std::shared_ptr<ResourceReflector>;

        ResourceReflector(IDxcUtils* util, io::BufferView buffer);

        using ShaderPins = RenderShaders::ShaderPins;
        ShaderPins mShaderInputs;
        ShaderPins mShaderOutputs;

        struct RootParameter
        {
            RootParameter() = default;
            RootParameter(const D3D12_ROOT_PARAMETER1& parameter, const std::string& variableName, const std::string& variableTypeName);
            RootParameter(const RootParameter& other);
            RootParameter& operator=(const RootParameter& other);

            D3D12_ROOT_PARAMETER1 mParameter;
            std::string mVariableName;
            std::string mVariableTypeName;
            std::vector<CD3DX12_DESCRIPTOR_RANGE1> mDescriptorRangesScratchPad;

        private:
            void FixScratchPad();
        };

        using RootParameters = std::vector<RootParameter>;

        using IndexMap = RenderShaders::IndexMap;
        using RootDescResult = RenderShaders::RootDescResult;

        using DescriptionCallback = RenderShaders::DescriptionCallback;
        void MakeRootSignature(ResourceReflector* additionalReflector, DescriptionCallback descriptionCallback);

        static void PopulateMappings(io::VirtualTransportSystem::Section fileName, io::VirtualTransportSystem& vts);
        static void SaveMappings(io::VirtualTransportSystem::Section fileName, io::VirtualTransportSystem& vts);

    private:
        D3D12_SHADER_VERSION_TYPE mShaderType = D3D12_SHVER_RESERVED0;
        uint32_t mMajorVersion = 0;
        uint32_t mMinorVersion = 0;
        D3D12_SHADER_VISIBILITY mShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

        static const RootDescResult MakeRootSignature(const RootParameters& rootParameters, const RootParameters& samplerParameters);

        void GenerateSignature(RootParameters& rootParameters);
        D3D12_SHADER_VISIBILITY GeneratePins(uint32_t shaderType, const D3D12_SHADER_DESC& shaderDesc);

        ComPtr<ID3D12ShaderReflection> mShaderReflection;
    };

    template <typename... Args>
    auto JoinParameters(Args& ... args)
    {
        ResourceReflector::RootParameters rootParameters;

        using Sources = std::tuple<Args...>;
        Sources sources(args...);
        static constexpr size_t NumSources = std::tuple_size_v<Sources>;

        yaget::meta::for_each(sources, [&rootParameters](const auto& source)
        {
            rootParameters.append_range(source);
        });

        return rootParameters;
    }


    //-------------------------------------------------------------------------------------------------
    class ResourceCompiler
    {
    public:
        ResourceCompiler();
        ~ResourceCompiler();

        using CompileResult = std::pair<io::Buffer, ResourceReflector::Ptr>;
        CompileResult Compile(io::BufferView data, const Strings& parameters) const;

        ResourceReflector::Ptr Decompile(io::BufferView data) const;

    private:
        ComPtr<IDxcUtils> mUtils;
        ComPtr<IDxcCompiler3> mCompiler;
    };

}
