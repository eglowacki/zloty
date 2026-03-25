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
                           , uint32_t mIndex)
                : mAllocation(allocation)
                , mResource(resource)
                , mRootType(rootType)
                , mConstantType(constantType)
                , mConstantLayout(constantLayout)
                , mIndex(mIndex)
            {
            }

            D3D12MA::Allocation* mAllocation;
            ComPtr<ID3D12Resource> mResource;
            constant_shader_types::RootType mRootType;
            constant_shader_types::ConstantTypes mConstantType;
            constant_shader_types::ConstantLayout mConstantLayout;
            uint32_t mIndex;
        };

        using ShaderVariables = std::vector<ShaderVariable>;

        ConstantBuffer(const ShaderVariables& shaderVariables);
        ~ConstantBuffer();

        bool UpdateData(constant_shader_types::ConstantTypes constantTypes, const void* data, size_t dataSize);
        bool UpdateData(constant_shader_types::ConstantTypes constantTypes, const uint8_t* data, size_t dataSize);
        bool UpdateData(constant_shader_types::ConstantTypes constantTypes, ID3D12Resource* resource);
        void Bind(ID3D12GraphicsCommandList* commandList) const;

        template <typename T>
        bool UpdateData(constant_shader_types::ConstantTypes constantTypes, const T& data)
        {
            return UpdateData(constantTypes, reinterpret_cast<const uint8_t*>(&data), sizeof(T));
        }

    private:
        const ShaderVariable* FindVariable(constant_shader_types::ConstantTypes constantType) const;

        ShaderVariables mShaderVariables;

        using VariableUpdateData = std::map<constant_shader_types::ConstantTypes, io::Buffer>;
        VariableUpdateData mVariableUpdateData;
    };
}
