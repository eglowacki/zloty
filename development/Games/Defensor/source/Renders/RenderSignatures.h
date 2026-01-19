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
#include "Render/Cache/AssetCache.h"

struct ID3D12RootSignature;
struct ID3D12Device;


namespace defensor::render
{
    class RenderSignatures
    {
    public:
        RenderSignatures(ID3D12Device* device, yaget::io::VirtualTransportSystem& vts);
        ~RenderSignatures();

        ID3D12RootSignature* GetSignature(uint64_t sigType);

    private:
        ID3D12Device* mDevice = {};
        std::map<uint64_t, yaget::render::ComPtr<ID3D12RootSignature>> mSignatures;
        yaget::io::VirtualTransportSystem& mVTS;

        yaget::render::AssetCache mCache;
    };
}
