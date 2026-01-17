///////////////////////////////////////////////////////////////////////
// RenderSignatures.h
//
//  Copyright 01/16/2026 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//
//  #include "Renders/RenderSignatures.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "Render/RenderCore.h"

struct ID3D12RootSignature;
struct ID3D12Device;


namespace defensor::render
{
    class RenderSignatures
    {
    public:
        RenderSignatures(ID3D12Device* device);
        ~RenderSignatures();

        ID3D12RootSignature* GetSignature(uint64_t sigType);

    private:
        ID3D12Device* mDevice = {};
        std::map<uint64_t, yaget::render::ComPtr<ID3D12RootSignature>> mSignatures;
    };
}
