///////////////////////////////////////////////////////////////////////
// ConstantBuffer.h
//
//  Copyright 03/18/2026 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//
//  #include "Render/Pipeline/ConstantBuffer.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once


#include "RenderShaders.h"

struct ID3D12Resource;

namespace D3D12MA
{
    class Allocation;
}

namespace yaget::render
{
    class ShaderBuffers;

    //--------------------------------------------------------------------------------------------------
    class ConstantBuffer
    {
    public:
        struct ShaderVariable
        {
            ShaderVariable(D3D12MA::Allocation* allocation
                           , ComPtr<ID3D12Resource> resource
                           , constant_shader_types::RootType rootType
                           , constant_shader_types::ConstantTypes constantType
                           , constant_shader_types::ConstantLayout constantLayout
                           , uint32_t mIndex
                           , ShaderBuffers* shaderBuffers)
                : mAllocation(allocation)
                , mResource(resource)
                , mRootType(rootType)
                , mConstantType(constantType)
                , mConstantLayout(constantLayout)
                , mIndex(mIndex)
                , mShaderBuffers{ shaderBuffers }
            {
            }

            ComPtr<ID3D12Resource> GetResource(uint32_t bufferIndex, size_t dataSize) const;

            D3D12MA::Allocation* mAllocation;
            mutable ComPtr<ID3D12Resource> mResource;
            constant_shader_types::RootType mRootType;
            constant_shader_types::ConstantTypes mConstantType;
            constant_shader_types::ConstantLayout mConstantLayout;
            uint32_t mIndex;

            ShaderBuffers* mShaderBuffers{};
        };

        using ShaderVariables = std::vector<ShaderVariable>;

        ConstantBuffer(const ShaderVariables& shaderVariables);
        ~ConstantBuffer();

        void Bind(ID3D12GraphicsCommandList* commandList) const;
        bool UpdateData(uint32_t bufferIndex, constant_shader_types::ConstantTypes constantTypes, const uint8_t* data, size_t dataSize);

        template <typename T>
        bool UpdateData(uint32_t bufferIndex, constant_shader_types::ConstantTypes constantTypes, const T& data)
        {
            return UpdateData(bufferIndex, constantTypes, reinterpret_cast<const uint8_t*>(&data), sizeof(T));
        }

    private:
        const ShaderVariable* FindVariable(constant_shader_types::ConstantTypes constantType) const;

        ShaderVariables mShaderVariables;

        using VariableUpdateData = std::map<constant_shader_types::ConstantTypes, io::Buffer>;
        VariableUpdateData mVariableUpdateData;
    };
}
