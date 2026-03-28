#include "Core/ErrorHandlers.h"
#include "Render/Commands/RenderCommandList.h"
#include "Render/Commands/RenderCommandListStates.h"

#include <d3dx12.h>


//-------------------------------------------------------------------------------------------------
void yaget::render::commands::TransitionToRenderTarget(CommandList* commandList, ID3D12Resource* renderTarget, ID3D12DescriptorHeap* descriptorHeap, int frameIndex)
{
    YAGET_ASSERT(renderTarget, "renderTarget parameter is null.");
    YAGET_ASSERT(commandList, "commandList parameter is null.");
    YAGET_ASSERT(descriptorHeap, "descriptorHeap parameter is null.");

    auto deviceCommandList = commandList->GetDeviceCommandList();

    const CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(renderTarget, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    deviceCommandList->ResourceBarrier(1, &barrier);

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

    const CD3DX12_CPU_DESCRIPTOR_HANDLE rtv(descriptorHeap->GetCPUDescriptorHandleForHeapStart(), frameIndex, descriptorHandleSize);
    deviceCommandList->OMSetRenderTargets(1, &rtv, false, nullptr);
}


//-------------------------------------------------------------------------------------------------
void yaget::render::commands::TransitionToPresent(const CommandList* commandList, ID3D12Resource* renderTarget)
{
    YAGET_ASSERT(commandList, "renderTarget is null.");
    YAGET_ASSERT(renderTarget, "renderTarget is null.");

    auto deviceCommandList = commandList->GetDeviceCommandList();

    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(renderTarget, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    deviceCommandList->ResourceBarrier(1, &barrier);
}


//-------------------------------------------------------------------------------------------------
void yaget::render::commands::ClearRenderTarget(CommandList* commandList, const colors::Color& color, ID3D12Resource* renderTarget, ID3D12DescriptorHeap* descriptorHeap, int frameIndex)
{
    YAGET_ASSERT(renderTarget, "renderTarget is null.");
    YAGET_ASSERT(descriptorHeap, "descriptorHeap is null.");

    const float clearColor[] = { color.R(), color.B(), color.G(), color.A() };
    auto deviceCommandList = commandList->GetDeviceCommandList();

    ComPtr<ID3D12Device4> device;
    const HRESULT hr = renderTarget->GetDevice(IID_PPV_ARGS(&device));
    error_handlers::ThrowOnError(hr, "Could not get device from render target");

    const auto descriptorHandleSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    const CD3DX12_CPU_DESCRIPTOR_HANDLE rtv(descriptorHeap->GetCPUDescriptorHandleForHeapStart(), frameIndex, descriptorHandleSize);
    deviceCommandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
}
