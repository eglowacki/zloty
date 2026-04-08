#include "Core/ErrorHandlers.h"
#include "Render/Helpers/ResourceDescriptions.h"
#include "Render/Platform/D3D12MemAlloc.h"
#include "Render/Platform/DeviceDebugger.h"
#include "StringHelpers.h"

#include "magic_enum/magic_enum.hpp"

namespace
{
    D3D12MA::Allocation* CreateHeapTexture(const yaget::io::Tag& tag,
                                           D3D12_HEAP_TYPE heapType,
                                           D3D12_RESOURCE_DIMENSION dimension,
                                           D3D12_RESOURCE_STATES resourceState,
                                           D3D12_TEXTURE_LAYOUT layout,
                                           uint64_t sizeX,
                                           uint32_t sizeY,
                                           DXGI_FORMAT format,
                                           D3D12MA::Allocator* allocator)
    {
        using namespace yaget;

        D3D12MA::ALLOCATION_DESC heapDesc = {};
        heapDesc.HeapType = heapType;
        heapDesc.ExtraHeapFlags = D3D12_HEAP_FLAG_NONE;

        D3D12_RESOURCE_DESC textureDesc = {};
        textureDesc.Dimension = dimension;
        textureDesc.Alignment = 0;
        textureDesc.Width = sizeX;
        textureDesc.Height = sizeY;
        textureDesc.DepthOrArraySize = 1;
        textureDesc.MipLevels = 1;
        textureDesc.Format = format;
        textureDesc.SampleDesc.Count = 1;
        textureDesc.SampleDesc.Quality = 0;
        textureDesc.Layout = layout;
        textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

        D3D12MA::Allocation* allocation = nullptr;

        HRESULT hr = allocator->CreateResource(
            &heapDesc,
            &textureDesc,
            resourceState,
            nullptr,
            &allocation,
            IID_NULL, nullptr);

        error_handlers::ThrowOnError(hr, std::format("Could not allocate texture resource heap: '{}', dimension: '{}', resourceState: '{}' for tag: {}",
                                                     magic_enum::enum_name(heapType),
                                                     magic_enum::enum_name(dimension),
                                                     magic_enum::enum_name(resourceState),
                                                     conv::ToString(tag)));

        return allocation;
    }
}


//-------------------------------------------------------------------------------------------------
D3D12MA::Allocation* yaget::render::helpers::CreateGpuHeap(const io::Tag& tag, int sizeX, int sizeY, D3D12_RESOURCE_DIMENSION dimension, D3D12_RESOURCE_STATES resourceState, D3D12_TEXTURE_LAYOUT layout,
                                                           DXGI_FORMAT format, D3D12MA::Allocator* allocator)
{
    auto allocation = CreateHeapTexture(tag,
                                        D3D12_HEAP_TYPE_DEFAULT,
                                        dimension,
                                        resourceState,
                                        layout,
                                        sizeX,
                                        sizeY,
                                        format,
                                        allocator);

    ID3D12Resource* res = allocation->GetResource();
    platform::SetDebugName(res, allocation, "GpuHeap", conv::ToString(tag));

    return allocation;
}


//-------------------------------------------------------------------------------------------------
D3D12MA::Allocation* yaget::render::helpers::CreateUploadHeap(const io::Tag& tag, uint64_t bufferSize, D3D12MA::Allocator* allocator)
{
    auto allocation = CreateHeapTexture(tag,
                                        D3D12_HEAP_TYPE_UPLOAD,
                                        D3D12_RESOURCE_DIMENSION_BUFFER,
                                        D3D12_RESOURCE_STATE_GENERIC_READ,
                                        D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
                                        bufferSize,
                                        1,
                                        DXGI_FORMAT_UNKNOWN,
                                        allocator);

    ID3D12Resource* res = allocation->GetResource();
    platform::SetDebugName(res, allocation, "UploadHeap", conv::ToString(tag));

    return allocation;
}


//-------------------------------------------------------------------------------------------------
void yaget::render::helpers::UploadData(ID3D12GraphicsCommandList* commandList, ID3D12Resource* destination, ID3D12Resource* intermediate, const uint8_t* data, int stride, size_t sliceSize)
{
    D3D12_SUBRESOURCE_DATA textureData = {};
    textureData.pData = data;
    textureData.RowPitch = stride;
    textureData.SlicePitch = static_cast<LONG_PTR>(sliceSize);

    auto destinationDesc = destination->GetDesc();
    auto intermediateDesc = intermediate->GetDesc();

    if (destinationDesc.Dimension == intermediateDesc.Dimension &&
        destinationDesc.Height == intermediateDesc.Height && destinationDesc.Width == intermediateDesc.Width &&
        destinationDesc.Format == intermediateDesc.Format)
    {
        void* bufferData = nullptr;
        HRESULT hr = intermediate->Map(0, nullptr, &bufferData);
        error_handlers::ThrowOnError(hr, "Could not map upload buffer for write.");

        memcpy(bufferData, data, sliceSize);
        intermediate->Unmap(0, nullptr);

        commandList->CopyResource(destination, intermediate);
    }
    else
    {
        UpdateSubresources(commandList, destination, intermediate, 0, 0, 1, &textureData);
    }
}


//-------------------------------------------------------------------------------------------------
yaget::render::helpers::GpuResourceResult yaget::render::helpers::CreateGpuResource(const io::Tag& tag, const SourceGpuParameters& parameters, ID3D12GraphicsCommandList* commandList, D3D12MA::Allocator* allocator)
{
    //--------------------------------------------------
    // Describe and create a Texture2D which will be used by shader (GPU).
    auto allocation = CreateGpuHeap(tag, parameters.mSizeX, parameters.mSizeY, parameters.mDimension, parameters.mResourceState, parameters.mLayout, parameters.mFormat, allocator);
    ID3D12Resource* texture = allocation->GetResource();

    //--------------------------------------------------
    // Create the GPU upload buffer, this will receive the actual pixel data and will copy pixel data to the texture.
    const auto uploadBufferSize = GetRequiredIntermediateSize(texture, 0, 1);

    auto uploadAllocation = CreateUploadHeap(tag, uploadBufferSize, allocator);

    //--------------------------------------------------
    // Copy data to the intermediate upload heap and then schedule a copy 
    // from the upload heap to the Texture2D.
    UploadData(commandList, texture, uploadAllocation->GetResource(), parameters.mData, parameters.mStride, parameters.mSliceSize);

    return { .mGpuAllocation = allocation, .mUploadAllocation = uploadAllocation };
}


//-------------------------------------------------------------------------------------------------
yaget::render::ComPtr<ID3D12DescriptorHeap> yaget::render::helpers::CreateDescriptorHeap(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors)
{
    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.NumDescriptors = numDescriptors;
    desc.Type = type;
    switch (type)
    {
        case D3D12_DESCRIPTOR_HEAP_TYPE_RTV:
        case D3D12_DESCRIPTOR_HEAP_TYPE_DSV:
            desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
            break;
        case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
            desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
            break;
        case D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER:
            desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
            break;
        default:
            YAGET_ASSERT(false, std::format("Descriptor Type: '{}' not handled!!!", magic_enum::enum_name(type)).c_str());
    }

    ComPtr<ID3D12DescriptorHeap> descriptorHeap;
    const HRESULT hr = device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&descriptorHeap));
    error_handlers::ThrowOnError(hr, "Could not create DX12 DescriptorHeap");

    YAGET_RENDER_SET_DEBUG_NAME(descriptorHeap.Get(), std::format("DescriptorHeap-{}", magic_enum::enum_name(type)).c_str());

    return descriptorHeap;
}


//-------------------------------------------------------------------------------------------------
yaget::render::ComPtr<ID3D12Resource> yaget::render::helpers::CreateDepthStencilBuffer(ID3D12Device* device, ID3D12DescriptorHeap* dsDescriptorHeap, size_t width, size_t height, DXGI_FORMAT depthStencilFormat)
{
    if (depthStencilFormat == DXGI_FORMAT_UNKNOWN)
    {
        return {};
    }

    YAGET_ASSERT(depthStencilFormat == DXGI_FORMAT_D32_FLOAT || depthStencilFormat == DXGI_FORMAT_D24_UNORM_S8_UINT,
        std::format("Can not create Depth-Stencil Buffer with invalid format: '%s'.",
            magic_enum::enum_name(depthStencilFormat)).c_str());

    D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
    depthOptimizedClearValue.Format = depthStencilFormat;
    depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
    depthOptimizedClearValue.DepthStencil.Stencil = 0;

    D3D12_RESOURCE_DESC depthStencilDesc = {};
    depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    depthStencilDesc.Alignment = 0;
    depthStencilDesc.Width = width;
    depthStencilDesc.Height = static_cast<uint32_t>(height);
    depthStencilDesc.DepthOrArraySize = 1;
    depthStencilDesc.MipLevels = 0;
    depthStencilDesc.Format = depthStencilFormat;
    depthStencilDesc.SampleDesc.Count = 1;
    depthStencilDesc.SampleDesc.Quality = 0;
    depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_HEAP_PROPERTIES heapProperties{};
    heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

    yaget::render::ComPtr<ID3D12Resource> depthStencilBuffer;
    HRESULT hr = device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &depthStencilDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &depthOptimizedClearValue, IID_PPV_ARGS(&depthStencilBuffer));
    yaget::error_handlers::ThrowOnError(hr, "Could not create DX12 Depth Stencil buffer");

    YAGET_RENDER_SET_DEBUG_NAME(depthStencilBuffer.Get(), "Depth Stencil Buffer");

    D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc = {};
    depthStencilViewDesc.Format = depthStencilFormat;
    depthStencilViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    depthStencilViewDesc.Flags = D3D12_DSV_FLAG_NONE;

    device->CreateDepthStencilView(depthStencilBuffer.Get(), &depthStencilViewDesc, dsDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

    return depthStencilBuffer;
}
