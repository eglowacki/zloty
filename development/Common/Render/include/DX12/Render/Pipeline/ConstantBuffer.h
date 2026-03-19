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
        ConstantBuffer(D3D12MA::Allocation* allocation, ComPtr<ID3D12Resource> resource, constant_shader_types::RootType rootType, uint32_t index);
        ~ConstantBuffer();

        bool UpdateData(const void* data, size_t dataSize) const;
        void SetView(ID3D12GraphicsCommandList* commandList) const;

    private:
        unique_obj<D3D12MA::Allocation> mAllocation;
        ComPtr<ID3D12Resource> mResource;
        constant_shader_types::RootType mRootType;
        uint32_t mIndex;
    };
}
