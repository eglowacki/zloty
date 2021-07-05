/////////////////////////////////////////////////////////////////////////
// DeviceDebugger.h
//
//  Copyright 06/12/2021 Edgar Glowacki.
//
// NOTES:
//      Deals with debug layer for platform device
//
// #include "DeviceDebugger.h"
//
/////////////////////////////////////////////////////////////////////////
//! \file

#pragma once

#include "Render/RenderCore.h"

struct ID3D12Object;

#if YAGET_DEBUG_RENDER == 1

struct ID3D12Device4;

namespace yaget::render::platform
{
    class DeviceDebugger
    {
    public:
        DeviceDebugger();
        ~DeviceDebugger();

        void ActivateMessageSeverity(const ComPtr<ID3D12Device4>& device);
    };

    void SetDebugName(ID3D12Object* object, const std::string& name, const char* file, unsigned line);
}

#else

namespace yaget::render::platform
{
    inline void SetDebugName(ID3D12Object*, const std::string&, const char*, unsigned) {}
}

#endif // YAGET_DEBUG_RENDER == 1

#define YAGET_RENDER_SET_DEBUG_NAME(object, name) yaget::render::platform::SetDebugName(object, name, __FILE__, __LINE__)