#include "Components/ModelComponent.h"
#include "MathFacade.h"
#include "Device.h"
#include "Scene.h"
#include "TextureResource.h"
#include "Resources/ShaderResources.h"
#include "Debugging/Assert.h"
#include "RenderHelpers.h"
#include "StringHelpers.h"
#include "App/AppUtilities.h"
#include "VTS/ResolvedAssets.h"
#include "VTS/RenderResolvedAssets.h"
#include "Fmt/format.h"
#include "Exception/Exception.h"
#include "ImageLoaders/ImageProcessor.h"

#include <filesystem>
#include <d3dcompiler.h>
#include <VertexTypes.h>

using namespace yaget;
using namespace DirectX;
using namespace Microsoft::WRL;
namespace fs = std::filesystem;

namespace
{
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

            HRESULT hr = device->CreateBuffer(&bd, &srd, &buffer);
            YAGET_THROW_ON_RROR(hr, "Could not create vertex buffer");

            return buffer;
        }

        typedef std::pair<ComPtr<T>, ComPtr<ID3D11Buffer>> Buffers_t;
        static Buffers_t Make(render::Device::ID3D11Device_t* device, const std::string& /*bufferFileName*/, const float* elements, size_t numElements, const unsigned int* indicies, size_t numIndices)
        {
            Buffers_t buffers;

            D3D11_BUFFER_DESC bd = { 0 };
            bd.ByteWidth = static_cast<UINT>(sizeof(float) * numElements);
            bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
            D3D11_SUBRESOURCE_DATA srd = { elements, 0, 0 };

            HRESULT hr = device->CreateBuffer(&bd, &srd, &buffers.first);
            YAGET_THROW_ON_RROR(hr, "Could not create vertex buffer");

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
                YAGET_THROW_ON_RROR(hr, "Could not create index buffer");
            }

            return buffers;
        }
    };

} // namespace


//--------------------------------------------------------------------------------------------------
render::ModelComponent::ModelComponent(comp::Id_t id, render::Device& device)
    : render::RenderComponent(id, device, Init::Default, {})
{
    AddSupportedTrigger(render::RenderComponent::SignalReset);
    AddSupportedTrigger(SignalViewCreated);
}


namespace yaget
{
    namespace render
    {
        template <>
        struct Reloader<yaget::render::GeometryResource>
        {
            using R = yaget::render::GeometryResource;

            void operator()(std::shared_ptr<R> resource, mt::SmartVariable<R>& sink, std::function<void()> trigger) const
            {
                YAGET_ASSERT(sink.empty(), "While waiting for resource, sink is not null.");
                sink = resource;
                trigger();
            }
        };

    }
}


//--------------------------------------------------------------------------------------------------
void render::ModelComponent::OnReset()
{
    using Section = io::VirtualTransportSystem::Section;

    mGeometry  = mt::SmartVariable<GeometryResource>::SmartType();
    mSkinShader = mt::SmartVariable<SkinShaderResource>::SmartType();

    // Manage skin shader (aka material)
    const Section kEnglishHouse("ShaderMaterials@Shaders/EnglishHouse");
    io::Tag skinTag = mDevice.App().VTS().GetTag(kEnglishHouse);
    mDevice.RequestResourceView<SkinShaderResource>(skinTag, std::ref(mSkinShader), 0);

    // Manage geometry data
    std::vector<io::Tag> tags = mTags;
    YAGET_ASSERT(tags.size() == 1, "OnReset for model supports only one geom tag for now. There were: '%d' entries.", tags.size());

    auto modelReloader = std::bind(Reloader<GeometryResource>(), std::placeholders::_1, std::ref(mGeometry), [this]() { TriggerSignal(SignalViewCreated); });
    auto modelConvertor = std::bind(Convertor<io::render::GeomAsset, GeometryResource>(), std::placeholders::_1, std::ref(mDevice));
    mDevice.RequestResourceView<GeometryResource, io::render::GeomAsset>(tags, modelReloader, modelConvertor, false);
}


//--------------------------------------------------------------------------------------------------
math::Box render::ModelComponent::BoundingBox() const 
{
    mt::SmartVariable<GeometryResource>::SmartType dm = mGeometry;
    return dm ? dm->BoundingBox() : math::Box(0);
}


//--------------------------------------------------------------------------------------------------
void render::ModelComponent::OnRender(const RenderBuffer& /*renderBuffer*/, const math3d::Matrix& viewMatrix, const math3d::Matrix& projectionMatrix)
{
    mt::SmartVariable<GeometryResource>::SmartType geometry = mGeometry;
    mt::SmartVariable<SkinShaderResource>::SmartType skinShader = mSkinShader;

    if (geometry && skinShader && skinShader->Activate())
    {
        mDevice.UpdateConstant("PerFrame", "projectionMatrix", projectionMatrix);
        mDevice.UpdateConstant("PerFrame", "viewMatrix", viewMatrix);

        geometry->Activate();
    }
}


//--------------------------------------------------------------------------------------------------
void render::ModelComponent::OnGuiRender(const RenderBuffer& /*renderBuffer*/, const DirectX::SimpleMath::Matrix& /*viewMatrix*/, const DirectX::SimpleMath::Matrix& /*projectionMatrix*/)
{
}


//------------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------------------
render::QuadComponent::QuadComponent(comp::Id_t id, render::Device& device)
    : render::RenderComponent(id, device, Init::Default, {})
{
}

void render::QuadComponent::OnReset()
{
    QuadVertex quadVertices[] =
    {
        // Front Face
        QuadVertex(-1.0f, -1.0f, 1.0f, 0.0f, 1.0f),
        QuadVertex(-1.0f,  1.0f, 1.0f, 0.0f, 0.0f),
        QuadVertex( 1.0f,  1.0f, 1.0f, 1.0f, 0.0f),
        QuadVertex( 1.0f, -1.0f, 1.0f, 1.0f, 1.0f),
    };

    uint32_t numElementes = ARRAYSIZE(quadVertices);

    uint32_t indicies[] = {
        // Front Face
        0,  1,  2,
        0,  2,  3,
    };

    mVertexData.clear();
    mVertexData.resize(1);

    Device::ID3D11Device_t* d3dDevice = mDevice.GetDevice();

    D3D11_BUFFER_DESC vertexBufferDesc = {};
    vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    vertexBufferDesc.ByteWidth = sizeof(QuadVertex) * numElementes;
    vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA vertexBufferData = { quadVertices, 0, 0 };

    HRESULT hr = d3dDevice->CreateBuffer(&vertexBufferDesc, &vertexBufferData, &mVertexData[0].mVertexbuffer);
    YAGET_THROW_ON_RROR(hr, "Could not create vertex buffer");

    D3D11_BUFFER_DESC bufferIndexDesc = { 0 };
    bufferIndexDesc.Usage = D3D11_USAGE_DEFAULT;
    bufferIndexDesc.ByteWidth = static_cast<UINT>(sizeof(uint32_t) * 2 * 3);
    bufferIndexDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    bufferIndexDesc.CPUAccessFlags = 0;
    bufferIndexDesc.MiscFlags = 0;

    D3D11_SUBRESOURCE_DATA initIndexData = { 0 };
    initIndexData.pSysMem = indicies;

    hr = d3dDevice->CreateBuffer(&bufferIndexDesc, &initIndexData, &mVertexData[0].mIndexBuffer);
    YAGET_THROW_ON_RROR(hr, "Could not create index buffer");

    D3D11_RASTERIZER_DESC rasterizerDesc = {};
    rasterizerDesc.AntialiasedLineEnable = FALSE;
    rasterizerDesc.CullMode = D3D11_CULL_BACK;
    rasterizerDesc.DepthBias = 0;
    rasterizerDesc.DepthBiasClamp = 0.0f;
    rasterizerDesc.DepthClipEnable = TRUE;
    rasterizerDesc.FillMode = D3D11_FILL_SOLID;
    rasterizerDesc.FrontCounterClockwise = FALSE;
    rasterizerDesc.MultisampleEnable = FALSE;
    rasterizerDesc.ScissorEnable = FALSE;
    rasterizerDesc.SlopeScaledDepthBias = 0.0f;

    // Create the rasterizer wire state object.
    hr = d3dDevice->CreateRasterizerState(&rasterizerDesc, &mRasterizerState);
    YAGET_THROW_ON_RROR(hr, "Could not create rasterizer state");
}

void render::QuadComponent::OnRender(const RenderBuffer& /*renderBuffer*/, const math3d::Matrix& /*viewMatrix*/, const math3d::Matrix& /*projectionMatrix*/)
{
    Device::ID3D11DeviceContext_t* d3dDeviceContext = mDevice.GetDeviceContext();

    math3d::Matrix projectionOrtho;// = /*math3d::Matrix::*/CreateOrthographicLH(2, 2, 0.2, 1000.0f);

    //render::RenderTarget* renderTarget = mDevice.FindRenderTarget("MapTarget");

    // set the vertex buffer
    d3dDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    d3dDeviceContext->RSSetState(mRasterizerState.Get());

    //Set the d2d square's vertex buffer
    UINT stride = sizeof(QuadVertex);
    UINT offset = 0;
    d3dDeviceContext->IASetVertexBuffers(0, 1, mVertexData[0].mVertexbuffer.GetAddressOf(), &stride, &offset);

    //Set the d2d square's Index buffer
    d3dDeviceContext->IASetIndexBuffer(mVertexData[0].mIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

    d3dDeviceContext->DrawIndexed(6, 0, 0);

    ID3D11ShaderResourceView* nullSRV[1] = { nullptr };
    d3dDeviceContext->PSSetShaderResources(0, 1, nullSRV);
}

void render::QuadComponent::OnGuiRender(const RenderBuffer& /*renderBuffer*/, const DirectX::SimpleMath::Matrix& /*viewMatrix*/, const DirectX::SimpleMath::Matrix& /*projectionMatrix*/)
{
}
