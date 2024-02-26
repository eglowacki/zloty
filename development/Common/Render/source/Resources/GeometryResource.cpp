#include "Resources/GeometryResource.h"
#include "Core/ErrorHandlers.h"
#include "Device.h"
#include "imgui.h"
#include "VTS/RenderResolvedAssets.h"

#include <wrl/client.h>

using namespace Microsoft::WRL;

namespace
{
    using namespace yaget;

    struct QuadVertex    //Overloaded Vertex Structure
    {
        QuadVertex(float x, float y, float z, float u, float v)
            : mPos(x, y, z), mTexCoord(u, v)
        {
        }

        math3d::Vector3 mPos;
        math3d::Vector2 mTexCoord;
    };

    struct VERTEX
    {
        float X, Y, Z; // vertex position
    };

    template<typename T, typename V>
    struct BufferMaker
    {
        static ComPtr<T> Make(render::Device::ID3D11Device_t* device, const std::string& /*bufferFileName*/)
        {
            V OurVertices[] =
            {
                { 0.0f, 1.0f, 0.0f },
            { 0.45f, 0.0f, 0.0f },
            { -0.45f, 0.0f, 0.0f },
            };
            uint32_t numElementes = ARRAYSIZE(OurVertices);

            ComPtr<T> buffer;
            D3D11_BUFFER_DESC bd = { 0 };
            bd.ByteWidth = sizeof(V) * numElementes;
            bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
            D3D11_SUBRESOURCE_DATA srd = { OurVertices, 0, 0 };

            const HRESULT hr = device->CreateBuffer(&bd, &srd, &buffer);
            error_handlers::ThrowOnError(hr, "Could not create vertex buffer");

            return buffer;
        }

        typedef std::pair<ComPtr<T>, ComPtr<ID3D11Buffer>> Buffers_t;
        static Buffers_t Make(render::Device::ID3D11Device_t* device, const std::string& debugName, const float* elements, size_t numElements, const unsigned int* indicies, size_t numIndices)
        {
            Buffers_t buffers;

            D3D11_BUFFER_DESC bd = { 0 };
            bd.ByteWidth = static_cast<UINT>(sizeof(float) * numElements);
            bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
            D3D11_SUBRESOURCE_DATA srd = { elements, 0, 0 };

            HRESULT hr = device->CreateBuffer(&bd, &srd, &buffers.first);
            error_handlers::ThrowOnError(hr, "Could not create vertex buffer");

            YAGET_SET_DEBUG_NAME(buffers.first.Get(), debugName);

            if (indicies)
            {
                D3D11_BUFFER_DESC bufferIndexDesc = { 0 };
                bufferIndexDesc.Usage = D3D11_USAGE_DEFAULT;
                bufferIndexDesc.ByteWidth = static_cast<UINT>(sizeof(unsigned int) * numIndices);
                bufferIndexDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
                bufferIndexDesc.CPUAccessFlags = 0;
                bufferIndexDesc.MiscFlags = 0;
                D3D11_SUBRESOURCE_DATA initIndexData = { 0 };
                initIndexData.pSysMem = indicies;
                initIndexData.SysMemPitch = 0;
                initIndexData.SysMemSlicePitch = 0;

                hr = device->CreateBuffer(&bufferIndexDesc, &initIndexData, &buffers.second);
                error_handlers::ThrowOnError(hr, "Could not create index buffer");

                YAGET_SET_DEBUG_NAME(buffers.second.Get(), debugName);
            }

            return buffers;
        }
    };

} // namespace

yaget::render::GeometryResource::GeometryResource(Device& device, std::shared_ptr<io::render::PakAsset> asset)
    : ResourceView(device, asset->mTag, std::type_index(typeid(GeometryResource)))
{
    using Buffer = BufferMaker<ID3D11Buffer, VERTEX>;
    using DeviceBuffer = Buffer::Buffers_t;

    Device::ID3D11Device_t* hardwareDevice = device.GetDevice();

    const auto& meshes = asset->Mesh;
    for (const auto& it : meshes)
    {
        const io::GeometryData& verticies = it;
        mVertexData.push_back(VertexData());
        VertexData& meshVertexData = mVertexData.back();

        DeviceBuffer deviceBuffer = Buffer::Make(hardwareDevice,
            asset->mTag.mName,
            verticies.mVerticies.first,
            verticies.mVerticies.second,
            verticies.mIndices.first,
            verticies.mIndices.second);

        meshVertexData.mVertexbuffer = deviceBuffer.first;
        meshVertexData.mNumVerticies = verticies.mVerticies.second;

        meshVertexData.mIndexBuffer = deviceBuffer.second;
        meshVertexData.mNumIndicies = verticies.mIndices.second;

        if (verticies.mUVs.second)
        {
            meshVertexData.mUVbuffer = Buffer::Make(hardwareDevice, asset->mTag.mName, verticies.mUVs.first, verticies.mUVs.second, nullptr, 0).first;
        }

        meshVertexData.mBoundingBox = verticies.mBoundingBox;
        mBoundingBox.GrowExtends(meshVertexData.mBoundingBox);
    }

    D3D11_RASTERIZER_DESC rasterizerDesc = {};

    rasterizerDesc.AntialiasedLineEnable = FALSE;
    rasterizerDesc.CullMode = D3D11_CULL_NONE;
    rasterizerDesc.DepthBias = 0;
    rasterizerDesc.DepthBiasClamp = 0.0f;
    rasterizerDesc.DepthClipEnable = TRUE;
    rasterizerDesc.FillMode = D3D11_FILL_WIREFRAME;
    rasterizerDesc.FrontCounterClockwise = FALSE;
    rasterizerDesc.MultisampleEnable = FALSE;
    rasterizerDesc.ScissorEnable = FALSE;
    rasterizerDesc.SlopeScaledDepthBias = 0.0f;

    const HRESULT hr = hardwareDevice->CreateRasterizerState(&rasterizerDesc, &mWireRasterizerState);
    error_handlers::ThrowOnError(hr, "Could not create wire rasterizer state");
    YAGET_SET_DEBUG_NAME(mWireRasterizerState.Get(), asset->mTag.mName);

    std::size_t hashValue = asset->mTag.Hash();
    SetHashValue(hashValue);
}


//--------------------------------------------------------------------------------------------------
yaget::render::GeometryResource::GeometryResource(Device& device, std::shared_ptr<yaget::io::render::GeomAsset> asset)
    : ResourceView(device, asset->mTag, std::type_index(typeid(GeometryResource)))
{
    using Buffer = BufferMaker<ID3D11Buffer, VERTEX>;
    using DeviceBuffer = Buffer::Buffers_t;

    render::GeometryConvertor::Ptr meshes = asset->Mesh;

    Device::ID3D11Device_t* hardwareDevice = device.GetDevice();

    for (size_t i = 0; i < meshes->NumMeshes(); ++i)
    {
        const GeometryConvertor::GeometryData& verticies = meshes->GetVertexData(i);
        mVertexData.push_back(VertexData());
        VertexData& meshVertexData = mVertexData.back();

        DeviceBuffer deviceBuffer = Buffer::Make(hardwareDevice,
            asset->mTag.mName,
            verticies.mVerticies.first,
            verticies.mVerticies.second,
            verticies.mIndices.data(),
            verticies.mIndices.size());

        meshVertexData.mVertexbuffer = deviceBuffer.first;
        meshVertexData.mNumVerticies = verticies.mVerticies.second;

        meshVertexData.mIndexBuffer = deviceBuffer.second;
        meshVertexData.mNumIndicies = verticies.mIndices.size();

        if (verticies.mUVs.second)
        {
            meshVertexData.mUVbuffer = Buffer::Make(hardwareDevice, asset->mTag.mName, verticies.mUVs.first, verticies.mUVs.second, nullptr, 0).first;
        }

        meshVertexData.mBoundingBox = verticies.mBoundingBox;
        mBoundingBox.GrowExtends(meshVertexData.mBoundingBox);
    }

    D3D11_RASTERIZER_DESC rasterizerDesc = {};

    rasterizerDesc.AntialiasedLineEnable = FALSE;
    rasterizerDesc.CullMode = D3D11_CULL_NONE;
    rasterizerDesc.DepthBias = 0;
    rasterizerDesc.DepthBiasClamp = 0.0f;
    rasterizerDesc.DepthClipEnable = TRUE;
    rasterizerDesc.FillMode = D3D11_FILL_WIREFRAME;
    rasterizerDesc.FrontCounterClockwise = FALSE;
    rasterizerDesc.MultisampleEnable = FALSE;
    rasterizerDesc.ScissorEnable = FALSE;
    rasterizerDesc.SlopeScaledDepthBias = 0.0f;

    const HRESULT hr = hardwareDevice->CreateRasterizerState(&rasterizerDesc, &mWireRasterizerState);
    error_handlers::ThrowOnError(hr, "Could not create wire rasterizer state");
    YAGET_SET_DEBUG_NAME(mWireRasterizerState.Get(), asset->mTag.mName);

    std::size_t hashValue = asset->mTag.Hash();
    SetHashValue(hashValue);
}

//--------------------------------------------------------------------------------------------------
bool yaget::render::GeometryResource::Activate()
{
    if (!mVertexData.empty())
    {
        mDevice.ActivatedResource(this, fmt::format("This: {}, Hash: {}", static_cast<void*>(this), GetStateHash()).c_str());

        Device::ID3D11DeviceContext_t* deviceContext = mDevice.GetDeviceContext();

        // set the vertex buffer
        deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        for (auto&& it : mVertexData)
        {
            UINT stride = sizeof(VERTEX);
            UINT offset = 0;
            deviceContext->IASetVertexBuffers(0, 1, it.mVertexbuffer.GetAddressOf(), &stride, &offset);

            if (it.mUVbuffer)
            {
                stride = sizeof(float) * 3;
                offset = 0;
                deviceContext->IASetVertexBuffers(1, 1, it.mUVbuffer.GetAddressOf(), &stride, &offset);
            }
            if (it.mIndexBuffer)
            {
                deviceContext->IASetIndexBuffer(it.mIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
                deviceContext->DrawIndexed(static_cast<UINT>(it.mNumIndicies), 0, 0);
            }
            else
            {
                deviceContext->Draw(static_cast<UINT>(it.mNumVerticies), 0);
            }
        }

        return true;
    }

    return false;
}

void yaget::render::GeometryResource::UpdateGui(comp::Component::UpdateGuiType updateGuiType)
{
    using Section = io::VirtualTransportSystem::Section;

    if (updateGuiType == comp::Component::UpdateGuiType::Default)
    {
        for (const auto& vertexGeom : mVertexData)
        {
            ImGui::Text(fmt::format("Num Verticies: : '{}', Num Indicies: '{}', Radius: '{}'", 
                vertexGeom.mNumVerticies, 
                vertexGeom.mNumIndicies, 
                conv::Convertor<float>::ToString(vertexGeom.mBoundingBox.GetSize().Length() / 2.0f)).c_str());
        }
    }
}
