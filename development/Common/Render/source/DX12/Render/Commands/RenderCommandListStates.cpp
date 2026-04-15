#include "Core/ErrorHandlers.h"
#include "Render/Commands/RenderCommandList.h"
#include "Render/Commands/RenderCommandListStates.h"


//-------------------------------------------------------------------------------------------------
D3D12_RESOURCE_STATES yaget::render::commands::TransitionToRenderTarget(const CommandList* commandList, D3D12_RESOURCE_STATES fromState, ID3D12Resource* renderTarget, ID3D12DescriptorHeap* rtDescriptorHeap, ID3D12DescriptorHeap* dsDescriptorHeap, uint32_t frameIndex)
{
    YAGET_ASSERT(renderTarget, "Render-Target parameter is null.");
    YAGET_ASSERT(commandList, "CommandList parameter is null.");
    YAGET_ASSERT(rtDescriptorHeap, "Render-Target descriptorHeap parameter is null.");

    TransitionFromTo(commandList, renderTarget, fromState/*D3D12_RESOURCE_STATE_PRESENT*/, D3D12_RESOURCE_STATE_RENDER_TARGET);

    auto deviceCommandList = commandList->GetDeviceCommandList();

    D3D12_RESOURCE_DESC desc = renderTarget->GetDesc();
    auto width = std::max(1.0f, static_cast<float>(desc.Width));
    auto height = std::max(1.0f, static_cast<float>(desc.Height));

    D3D12_VIEWPORT viewport = {};
    viewport.Width = width;
    viewport.Height = height;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    deviceCommandList->RSSetViewports(1, &viewport);

    D3D12_RECT rect = {};
    rect.right = static_cast<LONG>(desc.Width);
    rect.bottom = static_cast<LONG>(desc.Height);
    deviceCommandList->RSSetScissorRects(1, &rect);

    ComPtr<ID3D12Device4> device;
    const HRESULT hr = renderTarget->GetDevice(IID_PPV_ARGS(&device));
    error_handlers::ThrowOnError(hr, "Could not get device from render target");

    const auto descriptorHandleSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    const CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), frameIndex, descriptorHandleSize);

    if (dsDescriptorHeap)
    {
        const CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(dsDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
        deviceCommandList->OMSetRenderTargets(1, &rtvHandle, false, &dsvHandle);
    }
    else
    {
        deviceCommandList->OMSetRenderTargets(1, &rtvHandle, false, nullptr);
    }

    return D3D12_RESOURCE_STATE_RENDER_TARGET;
}


//-------------------------------------------------------------------------------------------------
void yaget::render::commands::ClearRenderTarget(const CommandList* commandList, const colors::Color& color, ID3D12Resource* renderTarget, ID3D12DescriptorHeap* descriptorHeap, uint32_t frameIndex)
{
    YAGET_ASSERT(renderTarget, "Render-Target parameter is null.");
    YAGET_ASSERT(descriptorHeap, "Render-Target DescriptorHeap parameter is null.");

    const float clearColor[] = { color.R(), color.B(), color.G(), color.A() };
    auto deviceCommandList = commandList->GetDeviceCommandList();

    ComPtr<ID3D12Device4> device;
    const HRESULT hr = renderTarget->GetDevice(IID_PPV_ARGS(&device));
    error_handlers::ThrowOnError(hr, "Could not get device from render target");

    const auto descriptorHandleSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    const CD3DX12_CPU_DESCRIPTOR_HANDLE rtv(descriptorHeap->GetCPUDescriptorHandleForHeapStart(), frameIndex, descriptorHandleSize);
    deviceCommandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
}


//-------------------------------------------------------------------------------------------------
void yaget::render::commands::ClearDepthStencil(const CommandList* commandList, float depth, uint8_t stencil, ID3D12DescriptorHeap* dsDescriptorHeap)
{
    YAGET_ASSERT(dsDescriptorHeap, "Depth-Stencil DescriptorHeap parameter is null.");

    auto deviceCommandList = commandList->GetDeviceCommandList();

    const CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(dsDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
    deviceCommandList->ClearDepthStencilView(dsDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, depth, stencil, 0, nullptr);
}


//-------------------------------------------------------------------------------------------------
D3D12_RESOURCE_STATES yaget::render::commands::TransitionFromTo(const CommandList* commandList, ID3D12Resource* renderTarget, D3D12_RESOURCE_STATES fromState, D3D12_RESOURCE_STATES toState)
{
    auto deviceCommandList = commandList->GetDeviceCommandList();

    const CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(renderTarget, fromState, toState);
    deviceCommandList->ResourceBarrier(1, &barrier);

    return toState;
}
