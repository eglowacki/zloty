/////////////////////////////////////////////////////////////////////////
// Adapter.h
//
//  Copyright 06/12/2021 Edgar Glowacki.
//
// NOTES:
//      Deals with adapter which will create Device
//
// #include "Platform/Adapter.h"
//
/////////////////////////////////////////////////////////////////////////
//! \file

#pragma once

#include "Render/AdapterInfo.h"
#include "App/WindowFrame.h"
#include "Render/Platform/DeviceDebugger.h"

namespace D3D12MA { class Allocator; }

namespace yaget::render::platform
{
    class Adapter
    {
    public:
        Adapter(app::WindowFrame windowFrame, const yaget::render::info::Adapter& adapterInfo);
        ~Adapter();

        ID3D12Device* GetDevice() const;
        IDXGIFactory* GetFactory() const;
        D3D12MA::Allocator* GetAllocator() const;

    private:
#if YAGET_DEBUG_RENDER == 1
        DeviceDebugger mDeviceDebugger;
#endif // YAGET_DEBUG_RENDER == 1

        ComPtr<IDXGIFactory> mFactory{};
        ComPtr<IDXGIAdapter> mAdapter{};
        ComPtr<ID3D12Device> mDevice{};

        unique_obj<D3D12MA::Allocator> mAllocator;
    };

} // namespace yaget::render::platform

