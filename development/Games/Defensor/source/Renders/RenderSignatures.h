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
#include "Render/Cache/CacheWatcher.h"

struct ID3D12RootSignature;
struct ID3D12Device;


namespace defensor::render
{
    class RenderSignatures : public yaget::render::CacheWatcher
    {
    public:
        RenderSignatures(ID3D12Device* device, yaget::io::VirtualTransportSystem& vts);
        ~RenderSignatures();

        ID3D12RootSignature* GetSignature(const yaget::io::Tag& tag);

    private:
        void CachedAssetChanged(const yaget::io::Tag& tag);

        ID3D12Device* mDevice = {};

        std::map<yaget::io::Tag, yaget::render::ComPtr<ID3D12RootSignature>> mSignatures;
    };
}
