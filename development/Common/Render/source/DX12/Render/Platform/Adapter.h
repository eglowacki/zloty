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
#include <wrl/client.h>

struct IDXGIFactory4;
struct IDXGIAdapter1;
struct ID3D12Device;
struct ID3D12DebugDevice;

namespace yaget::render::platform
{
    class Adapter
    {
    public:
        Adapter();
        ~Adapter();

        Microsoft::WRL::ComPtr<ID3D12Device> GetDevice() const;
        Microsoft::WRL::ComPtr<IDXGIFactory4> GetFactory() const;

    private:
        Microsoft::WRL::ComPtr<IDXGIFactory4> mFactory;
        Microsoft::WRL::ComPtr<IDXGIAdapter1> mAdapter;
        Microsoft::WRL::ComPtr<ID3D12Device> mDevice;
    };
}
