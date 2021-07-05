/////////////////////////////////////////////////////////////////////////
// Adapter.h
//
//  Copyright 06/12/2021 Edgar Glowacki.
//
// NOTES:
//      Deals with adapter which will create Device
//
// #include "Adapter.h"
//
/////////////////////////////////////////////////////////////////////////
//! \file

#pragma once

#include "YagetCore.h"
#include "Render/RenderCore.h"
#include "App/WindowFrame.h"
#include "DeviceDebugger.h"

struct IDXGIFactory4;
struct ID3D12Device4;
struct ID3D12CommandQueue;
struct ID3D12Fence;
struct IDXGIAdapter4;

namespace D3D12MA { class Allocator; }

namespace yaget::render::platform
{
    class SwapChain2;

    class Adapter
    {
    public:
        Adapter(app::WindowFrame windowFrame);
        ~Adapter();

        const ComPtr<ID3D12Device4>& GetDevice() const;
        const ComPtr<IDXGIFactory4>& GetFactory() const;

    private:
#if YAGET_DEBUG_RENDER == 1
        DeviceDebugger mDeviceDebugger;
#endif // YAGET_DEBUG_RENDER == 1
        ComPtr<IDXGIFactory4> mFactory;
        ComPtr<IDXGIAdapter4> mAdapter;
        ComPtr<ID3D12Device4> mDevice;

        struct Deleter
        {
            void operator()(D3D12MA::Allocator* allocator) const;
        };

        using AllocHolder = std::unique_ptr<D3D12MA::Allocator, Deleter>;
        AllocHolder mAllocator;
    };

}
