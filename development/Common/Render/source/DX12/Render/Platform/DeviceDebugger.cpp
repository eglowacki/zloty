#include "D3D12MemAlloc.h"

#include "Render/Platform/DeviceDebugger.h"
#include "HashUtilities.h"

#include "Core/ErrorHandlers.h"

#include "Debugging/DevConfiguration.h"

#if YAGET_DEBUG_RENDER == 1

YAGET_COMPILE_GLOBAL_SETTINGS("Debug Render Module Included")

#include "App/AppUtilities.h"
#include "StringHelpers.h"

#include <filesystem>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <dxgidebug.h>


//-------------------------------------------------------------------------------------------------
yaget::render::platform::DeviceDebugger::DeviceDebugger()
{
    if (yaget::dev::CurrentConfiguration().mInit.EnableRenderDebugLayer)
    {
        ComPtr<ID3D12Debug> debugController;
        HRESULT hr = D3D12GetDebugInterface(IID_PPV_ARGS(&debugController));
        error_handlers::ThrowOnError(hr, "Could not get DX12 Debug interface");

        ComPtr<ID3D12Debug1> debugController1;
        hr = debugController.As(&debugController1);
        //hr = debugController->QueryInterface(IID_PPV_ARGS(&debugController1));
        error_handlers::ThrowOnError(hr, "Could not get DX12 Debug1 interface");

        debugController1->EnableDebugLayer();
        debugController1->SetEnableGPUBasedValidation(true);

        YLOG_INFO("DEVI", "DeviceDebugger Activated.");
    }
    else
    {
        YLOG_INFO("DEVI", "DeviceDebugger NOT Activated.");
    }
}


//-------------------------------------------------------------------------------------------------
yaget::render::platform::DeviceDebugger::~DeviceDebugger()
{
    if (yaget::dev::CurrentConfiguration().mInit.EnableRenderDebugLayer)
    {
        ComPtr<IDXGIDebug1> dxgiDebug;
        if (SUCCEEDED(::DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug))))
        {
            dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_DETAIL | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
        }
    }
}


//-------------------------------------------------------------------------------------------------
void yaget::render::platform::DeviceDebugger::ActivateMessageSeverity(const ComPtr<ID3D12Device>& device)
{
    if (yaget::dev::CurrentConfiguration().mInit.EnableRenderDebugLayer)
    {
        const bool breakOnCorruption = true;
        const bool breakOnError = true;
        const bool breakOnWarning = true;

        ComPtr<ID3D12InfoQueue> infoQueue;
        HRESULT hr = device.As(&infoQueue);
        error_handlers::ThrowOnError(hr, "Could not get DX12 InfoQueue Device Interface");

        if (breakOnCorruption)
        {
            hr = infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
            error_handlers::ThrowOnError(hr, "Could not set DX12 Break On 'Corruption' Severity");
        }

        if (breakOnError)
        {
            hr = infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
            error_handlers::ThrowOnError(hr, "Could not set DX12 Break On 'Error' Severity");
        }

        if (breakOnWarning)
        {
            hr = infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);
            error_handlers::ThrowOnError(hr, "Could not set DX12 Break On 'Warning' Severity");
        }

#if 0
        // Suppress whole categories of messages
        //D3D12_MESSAGE_CATEGORY Categories[] = {};

        // Suppress messages based on their severity level
        D3D12_MESSAGE_SEVERITY Severities[] =
        {
            D3D12_MESSAGE_SEVERITY_INFO
        };

        // Suppress individual messages by their ID
        D3D12_MESSAGE_ID denyIds[] = {
            D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,   // I'm really not sure how to avoid this message.
            D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,                         // This warning occurs when using capture frame while graphics debugging.
            D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,                       // This warning occurs when using capture frame while graphics debugging.
        };

        D3D12_INFO_QUEUE_FILTER newFilter = {};
        //NewFilter.DenyList.NumCategories = _countof(Categories);
        //NewFilter.DenyList.pCategoryList = Categories;
        newFilter.DenyList.NumSeverities = _countof(Severities);
        newFilter.DenyList.pSeverityList = Severities;
        newFilter.DenyList.NumIDs = _countof(denyIds);
        newFilter.DenyList.pIDList = denyIds;

        infoQueue->PushStorageFilter(&newFilter);
#endif // 0
        YLOG_INFO("DEVI", "DeviceDebugger MessageSeverity Activated.");
    }
    else
    {
        YLOG_INFO("DEVI", "DeviceDebugger MessageSeverity NOT Activated.");
    }
}


//-------------------------------------------------------------------------------------------------
void yaget::render::platform::SetDebugName(ID3D12Object* object, std::string_view name, const char* file, unsigned line)
{
    const auto message = std::format("{}-{}({})", name, std::filesystem::path(file).filename().generic_string(), line);
    const auto text = conv::utf8_to_wide(message);
    HRESULT hr = object->SetName(text.c_str());
    hr;
    int z = 0;
    z;
}


//-------------------------------------------------------------------------------------------------
std::string yaget::render::platform::GetDebugName(ID3D12Object* object)
{
    std::string result;
    uint32_t dataSize = 0;
    HRESULT hr = object->GetPrivateData(WKPDID_D3DDebugObjectNameW, &dataSize, nullptr);
    if (SUCCEEDED(hr) && dataSize)
    {
        std::wstring name(dataSize, 0);
        hr = object->GetPrivateData(WKPDID_D3DDebugObjectNameW, &dataSize, name.data());
        if (SUCCEEDED(hr))
        {
            result = conv::wide_to_utf8(name.c_str());
        }
    }

    return result;
}


//-------------------------------------------------------------------------------------------------
void yaget::render::platform::SetDebugName(ID3D12Object* object, D3D12MA::Allocation* allocation, std::string_view typeName, std::string_view objectName, const std::source_location& location)
{
    std::string debugName = std::format("{}_{}", typeName, objectName);
    allocation->SetName(conv::utf8_to_wide(debugName).c_str());
    SetDebugName(object, debugName, location.file_name(), location.line());
}

#else // YAGET_DEBUG_RENDER == 1

YAGET_COMPILE_GLOBAL_SETTINGS("Debug Render Module NOT Included")

#endif // YAGET_DEBUG_RENDER == 1
