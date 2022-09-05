#include "Render/Platform/Commander.h"
#include "App/AppUtilities.h"
#include "MathFacade.h"

#include <d3dx12.h>
#include <dxgi1_6.h>


//-------------------------------------------------------------------------------------------------
yaget::render::platform::Commander::Commander(uint32_t rtvDescriptorHandleSize, ID3D12DescriptorHeap* descriptorHeap)
    : mRTVDescriptorHandleSize{ rtvDescriptorHandleSize }
    , mDescriptorHeap{ descriptorHeap }
{
}


//-------------------------------------------------------------------------------------------------
yaget::render::platform::Commander::~Commander() = default;


//-------------------------------------------------------------------------------------------------
void yaget::render::platform::Commander::SetRenderTarget(IDXGISwapChain2* swapChain, ID3D12GraphicsCommandList* commandList, uint32_t frameIndex)
{
    DXGI_SWAP_CHAIN_DESC1 chainDesc = {};
    HRESULT hr = swapChain->GetDesc1(&chainDesc);
    YAGET_UTIL_THROW_ON_RROR(hr, "Could not get DX12 swap chain description");

    D3D12_VIEWPORT viewport = {};
    viewport.Width = static_cast<float>(chainDesc.Width);
    viewport.Height = static_cast<float>(chainDesc.Height);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    commandList->RSSetViewports(1, &viewport);

    D3D12_RECT rect = {};
    rect.right = chainDesc.Width;
    rect.bottom = chainDesc.Height;
    commandList->RSSetScissorRects(1, &rect);

    const CD3DX12_CPU_DESCRIPTOR_HANDLE rtv(mDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), frameIndex, mRTVDescriptorHandleSize);
    commandList->OMSetRenderTargets(1, &rtv, false, nullptr);
}


//-------------------------------------------------------------------------------------------------
void yaget::render::platform::Commander::ClearRenderTarget(const colors::Color& color, ID3D12GraphicsCommandList* commandList, uint32_t frameIndex)
{
    const float clearColor[] = { color.R(), color.B(), color.G(), color.A() };

    const CD3DX12_CPU_DESCRIPTOR_HANDLE rtv(mDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), frameIndex, mRTVDescriptorHandleSize);
    commandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
}
