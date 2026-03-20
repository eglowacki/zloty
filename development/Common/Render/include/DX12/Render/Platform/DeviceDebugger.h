/////////////////////////////////////////////////////////////////////////
// DeviceDebugger.h
//
//  Copyright 06/12/2021 Edgar Glowacki.
//
// NOTES:
//      Deals with debug layer for platform device
//
// #include "Render/Platform/DeviceDebugger.h"
//
/////////////////////////////////////////////////////////////////////////
//! \file

#pragma once

#include "Render/RenderCore.h"

struct ID3D12Object;

#if YAGET_DEBUG_RENDER == 1

struct ID3D12Device;

namespace yaget::render::platform
{
    //-------------------------------------------------------------------------------------------------
    class DeviceDebugger
    {
    public:
        DeviceDebugger();
        ~DeviceDebugger();

        void ActivateMessageSeverity(const ComPtr<ID3D12Device>& device);
    };

    void SetDebugName(ID3D12Object* object, const std::string& name, const char* file, unsigned line);
    std::string GetDebugName(ID3D12Object* object);

}

#else // YAGET_DEBUG_RENDER == 1

namespace yaget::render::platform
{
    inline void SetDebugName(ID3D12Object*, const std::string&, const char*, unsigned) {}
    std::string GetDebugName(ID3D12Object*) { return {}; }
}

#endif // YAGET_DEBUG_RENDER == 1

#define YAGET_RENDER_SET_DEBUG_NAME(object, name) yaget::render::platform::SetDebugName(object, name, __FILE__, __LINE__)
#define YAGET_RENDER_GET_DEBUG_NAME(object) yaget::render::platform::GetDebugName(object)