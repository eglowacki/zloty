#include "Polygon.h"

#include "App/AppUtilities.h"

#include "Render/Platform/D3D12MemAlloc.h"

yaget::render::Polygon::Polygon(D3D12MA::Allocator* allocator)
{
    struct Vertex
    {
        float x;
        float y;
    };

    const Vertex vertices[] = {
        { 0.0f, 0.5f },
        { 0.5f, -0.5f },
        { -0.5f, -0.5f }
    };

    const uint64_t verticesBufferSize = sizeof(vertices);


    D3D12_RESOURCE_DESC resourceDesc = {};
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resourceDesc.Alignment = 0;
    resourceDesc.Width = verticesBufferSize;
    resourceDesc.Height = 1;
    resourceDesc.DepthOrArraySize = 1;
    resourceDesc.MipLevels = 1;
    resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.SampleDesc.Quality = 0;
    resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    D3D12MA::ALLOCATION_DESC allocationDesc = {};
    allocationDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;
     
    //D3D12Resource* resource;
    //D3D12MA::Allocation* allocation;
    HRESULT hr = allocator->CreateResource(
        &allocationDesc,
        &resourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        &mAllocation,
        IID_PPV_ARGS(&mTriangleData));

    YAGET_UTIL_THROW_ON_RROR(hr, "Could not CreateResource from allocator.");

    void* bufferData = nullptr;
    hr = mTriangleData->Map(0, nullptr, &bufferData);
    YAGET_UTIL_THROW_ON_RROR(hr, "Could not map Polygon buffer for write.");

    memcpy(bufferData, vertices, verticesBufferSize);
    mTriangleData->Unmap(0, nullptr);
}


yaget::render::Polygon::~Polygon()
{
    mAllocation->Release();
}
