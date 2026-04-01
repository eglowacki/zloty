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
        textureDesc.Layout = layout;//D3D12_TEXTURE_LAYOUT_UNKNOWN;
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
D3D12MA::Allocation* yaget::render::helpers::CreateGpuHeap(const io::Tag& tag, int sizeX, int sizeY, D3D12_RESOURCE_DIMENSION dimension, D3D12_RESOURCE_STATES resourceState, D3D12_TEXTURE_LAYOUT layout, DXGI_FORMAT format, D3D12MA::Allocator* allocator)
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
    platform::SetDebugName(res, allocation, "Texture2D", conv::ToString(tag));

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
    platform::SetDebugName(res, allocation, "UploadTexture", conv::ToString(tag));

    return allocation;
}


//-------------------------------------------------------------------------------------------------
void yaget::render::helpers::UploadData(ID3D12GraphicsCommandList* commandList, ID3D12Resource* destination, ID3D12Resource* intermediate, const uint8_t* data, int stride, size_t sliceSize)
{
    D3D12_SUBRESOURCE_DATA textureData = {};
    textureData.pData = data;
    textureData.RowPitch = stride;
    textureData.SlicePitch = static_cast<LONG_PTR>(sliceSize);

    UpdateSubresources(commandList, destination, intermediate, 0, 0, 1, &textureData);

    //Consider using the CopyResource method when copying an entire resource, and use this method for copying regions of a resource.
    //ID3D12GraphicsCommandList::CopyResource method
    //Copies the entire contents of the source resource to the destination resource.
}


//-------------------------------------------------------------------------------------------------
yaget::render::helpers::GpuResourceResult yaget::render::helpers::CreateGpuResource(const io::Tag& tag, const SourceGpuParameters& parameters, ID3D12GraphicsCommandList* commandList, D3D12MA::Allocator* allocator)
{
    //--------------------------------------------------
    // Describe and create a Texture2D which will be used by shader (GPU).
    auto allocation = helpers::CreateGpuHeap(tag, parameters.mSizeX, parameters.mSizeY, parameters.mDimension, parameters.mResourceState, parameters.mLayout, parameters.mFormat, allocator);
    ID3D12Resource* texture = allocation->GetResource();

    //--------------------------------------------------
    // Create the GPU upload buffer, this will receive the actual pixel data and will copy pixel data to the texture.
    const auto uploadBufferSize = GetRequiredIntermediateSize(texture, 0, 1);

    auto uploadAllocation = helpers::CreateUploadHeap(tag, uploadBufferSize, allocator);

    //--------------------------------------------------
    // Copy data to the intermediate upload heap and then schedule a copy 
    // from the upload heap to the Texture2D.
    helpers::UploadData(commandList, texture, uploadAllocation->GetResource(), parameters.mData, parameters.mStride, parameters.mSliceSize);

    auto resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(texture, D3D12_RESOURCE_STATE_COPY_DEST, parameters.mTransitionTo);
    commandList->ResourceBarrier(1, &resourceBarrier);

    return { .mGpuAllocation = allocation, .mUploadAllocation = uploadAllocation };
}
