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
#include <source_location>


struct ID3D12Device;
struct ID3D12Object;
namespace D3D12MA 
{
    class Allocation; 
}

#if YAGET_DEBUG_RENDER == 1

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

    void SetDebugName(ID3D12Object* object, std::string_view name, const char* file, unsigned line);
    std::string GetDebugName(ID3D12Object* object);
    void SetDebugName(ID3D12Object* object, D3D12MA::Allocation* allocation, std::string_view typeName, std::string_view objectName, const std::source_location& location = std::source_location::current());

}

#else // YAGET_DEBUG_RENDER == 1

namespace yaget::render::platform
{
    inline void SetDebugName(ID3D12Object*, const std::string&, const char*, unsigned) {}
    std::string GetDebugName(ID3D12Object*) { return {}; }
    void SetDebugName(ID3D12Object, D3D12MA::Allocation, std::string_view, std::string_view, const std::source_location& location = std::source_location::current()) { location; }
}

#endif // YAGET_DEBUG_RENDER == 1

#define YAGET_RENDER_SET_DEBUG_NAME(object, name) yaget::render::platform::SetDebugName(object, name, __FILE__, __LINE__)
#define YAGET_RENDER_GET_DEBUG_NAME(object) yaget::render::platform::GetDebugName(object)