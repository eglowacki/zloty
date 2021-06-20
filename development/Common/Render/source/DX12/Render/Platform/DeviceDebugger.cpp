#include "DeviceDebugger.h"
#include "App/AppUtilities.h"

#include <d3d12.h>
#include <dxgi1_3.h>


//-------------------------------------------------------------------------------------------------
yaget::render::platform::DeviceDebugger::DeviceDebugger()
{
    using namespace Microsoft::WRL;

    ComPtr<ID3D12Debug> debugController;

    HRESULT hr = D3D12GetDebugInterface(IID_PPV_ARGS(&debugController));
    YAGET_UTIL_THROW_ON_RROR(hr, "Could not get DX12 Debug interface");

    hr = debugController->QueryInterface(IID_PPV_ARGS(&mDebugController));
    YAGET_UTIL_THROW_ON_RROR(hr, "Could not get DX12 Debug1 interface");

    mDebugController->EnableDebugLayer();
    mDebugController->SetEnableGPUBasedValidation(true);


    //dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
}
