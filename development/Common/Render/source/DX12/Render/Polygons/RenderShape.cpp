#include "Render/Polygons/RenderShape.h"
#include "Core/ErrorHandlers.h"
#include "Render/Platform/DeviceDebugger.h"
#include "Render/Platform/D3D12MemAlloc.h"
#include "Streams/Buffers.h"
#include "MathFacade.h"
#include <d3dx12.h>
#include <VertexTypes.h>
#include "VTS/VirtualTransportSystem.h"

// we need following data for the geometry
// vertices - pointer to the buffer containing vertex data (pos, color, etc.)
// numTriangles
namespace
{
    using namespace yaget;

    //VertexPositionColorTexture
    using Vertex = DirectX::VertexPositionColorTexture;

    const float aspectRatio = 1.0f;

    const Vertex vertices[] = {
        { DirectX::XMFLOAT3{ 0.0f, 1.0f * aspectRatio, 0.0f }, DirectX::XMFLOAT4{ 1.0f, 0.0f, 0.0f, 0.99f }, DirectX::XMFLOAT2{ 0.5f, 0.0f } },
        { DirectX::XMFLOAT3{ 1.0f, -1.0f * aspectRatio, 0.0f }, DirectX::XMFLOAT4{ 0.0f, 1.0f, 0.0f, 0.f }, DirectX::XMFLOAT2{ 0.0f, 1.0f } },
        { DirectX::XMFLOAT3{ -1.0f, -1.0f * aspectRatio, 0.0f }, DirectX::XMFLOAT4{ 0.0f, 0.0f, 1.0f, 0.f }, DirectX::XMFLOAT2{ 1.0f, 1.0f } }
    };

    const int numTriangles = 1;

    io::Buffer GetVertexData()
    {
        const auto verticesBufferSize = sizeof(vertices) * numTriangles;
        auto buffer = io::CreateBuffer(verticesBufferSize);

        const float scale = 0.5f;
        const math3d::Vector3 offset{ -(1.0f - scale), (1.0f - scale), 0.0f };

        auto *vertex = io::cast_data<Vertex>(buffer);
        Vertex *targetVertex = vertex;
        const Vertex *sourceVertex = vertices;

        for (auto i = 0; i < numTriangles; ++i)
        {
            targetVertex->position = math3d::Vector3(sourceVertex->position) * scale + offset;
            targetVertex->color = sourceVertex->color;
            targetVertex->textureCoordinate = sourceVertex->textureCoordinate;
            targetVertex++;
            sourceVertex++;

            targetVertex->position = math3d::Vector3(sourceVertex->position) * scale + offset;
            targetVertex->color = sourceVertex->color;
            targetVertex->textureCoordinate = sourceVertex->textureCoordinate;
            targetVertex++;
            sourceVertex++;

            targetVertex->position = math3d::Vector3(sourceVertex->position) * scale + offset;
            targetVertex->color = sourceVertex->color;
            targetVertex->textureCoordinate = sourceVertex->textureCoordinate;
            targetVertex++;
            sourceVertex++;
        }

        return buffer;
    }

}


//-------------------------------------------------------------------------------------------------
yaget::render::RenderShape::RenderShape(D3D12MA::Allocator* allocator, const io::Tag& /*assetTag*/, io::VirtualTransportSystem& /*vts*/)
    : mVertexBufferSize(sizeof(vertices) * numTriangles)
{
    D3D12_RESOURCE_DESC resourceDesc = {};
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resourceDesc.Alignment = 0;
    resourceDesc.Width = mVertexBufferSize;
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
     
    D3D12MA::Allocation* allocation = nullptr;
    yaget::render::ComPtr<ID3D12Resource> triangleData;
    HRESULT hr = allocator->CreateResource(
        &allocationDesc,
        &resourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        &allocation,
        IID_PPV_ARGS(&triangleData));
    yaget::error_handlers::ThrowOnError(hr, "Could not CreateResource from allocator.");
    YAGET_RENDER_SET_DEBUG_NAME(triangleData.Get(), std::format("Yaget-Poly Triangle Data"));

    mAllocation.reset(allocation);
    mAllocation->SetName(L"RenderShape");

    const auto vertexData = GetVertexData();
    UploadData(vertexData);
    //vts.RequestBlob(assetTag, [this](std::shared_ptr<io::Asset> asset)
    //{
    //    UploadData(asset->mBuffer);
    //    
    //}, nullptr);
}


//-------------------------------------------------------------------------------------------------
yaget::render::RenderShape::~RenderShape() = default;


//-------------------------------------------------------------------------------------------------
void yaget::render::RenderShape::UploadData(const io::Buffer& dataBlock)
{
    YAGET_ASSERT(io::size_data(dataBlock) == mVertexBufferSize, "Data size of '%d' to upload to RenderShape does not match created buffer size of '%d'.", io::size_data(dataBlock), mVertexBufferSize);

    void* bufferData = nullptr;
    auto resource = mAllocation->GetResource();
    HRESULT hr = resource->Map(0, nullptr, &bufferData);
    error_handlers::ThrowOnError(hr, "Could not map RenderShape buffer for write.");

    memcpy(bufferData, io::cast_data<const char*>(dataBlock), io::size_data(dataBlock));
    resource->Unmap(0, nullptr);
}


//-------------------------------------------------------------------------------------------------
void yaget::render::RenderShape::Render(ID3D12GraphicsCommandList* commandList) const
{
    auto resource = mAllocation->GetResource();

    D3D12_VERTEX_BUFFER_VIEW vertexDataView;
    vertexDataView.BufferLocation = resource->GetGPUVirtualAddress();
    vertexDataView.SizeInBytes = mVertexBufferSize;
    vertexDataView.StrideInBytes = sizeof(Vertex);

    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->IASetVertexBuffers(0, 1, &vertexDataView);
    commandList->DrawInstanced(3 * numTriangles, 1, 0, 0);
}
