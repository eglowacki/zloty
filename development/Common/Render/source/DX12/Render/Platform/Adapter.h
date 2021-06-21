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
#include "DeviceDebugger.h"

struct IDXGIFactory4;
struct IDXGIAdapter1;
struct ID3D12Device;
struct ID3D12CommandQueue;
struct ID3D12Fence;
struct ID3D12DebugDevice;

namespace yaget::render::platform
{
    class Adapter
    {
    public:
        template <typename T>
        using ComPtr = Microsoft::WRL::ComPtr<T>;

        Adapter();
        ~Adapter();

        const ComPtr<ID3D12Device>& GetDevice() const;
        const ComPtr<IDXGIFactory4>& GetFactory() const;

    private:
#if YAGET_DEBUG_RENDER == 1
        ComPtr<ID3D12DebugDevice> mDebugDevice;
        DeviceDebugger mDeviceDebugger;
#endif
        ComPtr<IDXGIFactory4> mFactory;
        ComPtr<IDXGIAdapter1> mAdapter;
        ComPtr<ID3D12Device> mDevice;
    };


    class CommandQueue
    {
    public:
        template <typename T>
        using ComPtr = Microsoft::WRL::ComPtr<T>;

        CommandQueue(const ComPtr<ID3D12Device>& device);
        ~CommandQueue();

        const ComPtr<ID3D12CommandQueue>& Get() const { return mCommandQueue; }

    private:
        ComPtr<ID3D12CommandQueue> mCommandQueue;
    };

    class Fence
    {
    public:
        template <typename T>
        using ComPtr = Microsoft::WRL::ComPtr<T>;

        Fence(const ComPtr<ID3D12Device>& device);
        ~Fence();

        void Wait(CommandQueue& commandQueue);

    private:
        ComPtr<ID3D12Fence> mFence;
        HANDLE mFenceEvent;
        uint64_t mFenceValue;
        //m_fenceValue = 1;
        //m_fenceEvent
        //m_fence
    };



}
