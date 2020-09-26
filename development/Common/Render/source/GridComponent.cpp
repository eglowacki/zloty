#include "Components/GridComponent.h"
#include "Device.h"
#include "Debugging/Assert.h"
#include "RenderHelpers.h"
#include "Fmt/format.h"
#include "MathFacade.h"
#include "Exception/Exception.h"
#include <memory>

using namespace yaget;
using namespace Microsoft::WRL;
using namespace DirectX;


render::GridComponent::GridComponent(comp::Id_t id, render::Device& device) 
    : render::RenderComponent(id, device, Init::AutoReset, {})
{
}

void render::GridComponent::OnReset()
{
    Device::ID3D11Device_t* d3dDevice = mDevice.GetDevice();
    Device::ID3D11DeviceContext_t* d3dDeviceContext = mDevice.GetDeviceContext();

    YAGET_ASSERT(d3dDevice, "ModelComponent did not get valid d3dDevice pointer.");
    YAGET_ASSERT(d3dDeviceContext, "ModelComponent did not have d3dContext attached.");

    try
    {
        mEffect = std::make_unique<BasicEffect>(d3dDevice);
        mBatch = std::make_unique<PrimitiveBatch<VertexPositionColor>>(d3dDeviceContext);
        mStates = std::make_unique<CommonStates>(d3dDevice);
    }
    catch (const std::exception& e)
    {
        const auto& textError = fmt::format("Did not initialize GridComponent '{}'. Error: {}", Id(), e.what());
        YAGET_UTIL_THROW("REND", textError);
    }

    mEffect->SetVertexColorEnabled(true);
    void const* shaderByteCode = nullptr;
    size_t byteCodeLength = 0;

    mEffect->GetVertexShaderBytecode(&shaderByteCode, &byteCodeLength);

    HRESULT hr = d3dDevice->CreateInputLayout(VertexPositionColor::InputElements, VertexPositionColor::InputElementCount,
        shaderByteCode, byteCodeLength,
        mInputLayout.ReleaseAndGetAddressOf());
    YAGET_THROW_ON_RROR(hr, "Could not create input layout for grid component");
}

render::GridComponent::~GridComponent()
{
    
}

void render::GridComponent::OnRender(const RenderBuffer& renderBuffer, const DirectX::SimpleMath::Matrix& viewMatrix, const DirectX::SimpleMath::Matrix& projectionMatrix)
{
    Device::ID3D11DeviceContext_t* d3dDeviceContext = mDevice.GetDeviceContext();

    d3dDeviceContext->OMSetDepthStencilState(mStates->DepthDefault(), 1);

    mEffect->SetMatrices(renderBuffer.Matrix, viewMatrix, projectionMatrix);

    mEffect->Apply(d3dDeviceContext);

    d3dDeviceContext->IASetInputLayout(mInputLayout.Get());

    mBatch->Begin();

    const float kExtents = 32;
    SimpleMath::Vector3 xaxis(kExtents, 0.f, 0.f);
    SimpleMath::Vector3 yaxis(0.f, 0.f, kExtents);
    SimpleMath::Vector3 origin = SimpleMath::Vector3::Zero;

    size_t divisions = static_cast<size_t>(kExtents) * 10;

    //const z 
    for (size_t i = 0; i <= divisions; ++i)
    {
        float fPercent = float(i) / float(divisions);
        fPercent = (fPercent * 2) - 1;

        SimpleMath::Vector3 scale = xaxis * fPercent + origin;
        SimpleMath::Color color1 = i % 5 == 0 ? SimpleMath::Color(Colors::White) : SimpleMath::Color(Colors::Gray);
        SimpleMath::Color color2 = color1;

        if (divisions / 2 == i)
        {
            color1 = Colors::Blue * 0.25;
            color2 = Colors::Blue;
        }

        if (fPercent < 0)
        {
            color1 *= 0.25;
            color2 *= 0.25;
        }

        VertexPositionColor v1(scale - yaxis, color1);
        VertexPositionColor v2(scale + yaxis, color2);
        mBatch->DrawLine(v1, v2);
    }

    for (size_t i = 0; i <= divisions; i++)
    {
        float fPercent = static_cast<float>(i) / static_cast<float>(divisions);
        fPercent = (fPercent * 2) - 1;

        SimpleMath::Vector3 scale = yaxis * fPercent + origin;
        SimpleMath::Color color1 = i % 5 == 0 ? SimpleMath::Color(Colors::White) : SimpleMath::Color(Colors::Gray);
        SimpleMath::Color color2 = color1;

        if (divisions / 2 == i)
        {
            color1 = Colors::Red * 0.25f;
            color2 = Colors::Red;
        }

        if (fPercent < 0)
        {
            color1 *= 0.25;
            color2 *= 0.25;
        }

        VertexPositionColor v1(scale - xaxis, color1);
        VertexPositionColor v2(scale + xaxis, color2);
        mBatch->DrawLine(v1, v2);
    }

    mBatch->End();
}

void render::GridComponent::OnGuiRender(const RenderBuffer& /*renderBuffer*/, const DirectX::SimpleMath::Matrix& /*viewMatrix*/, const DirectX::SimpleMath::Matrix& /*projectionMatrix*/)
{
}

render::GridComponentPool::Ptr render::GridComponentPool::New(comp::Id_t id, Device& device)
{
    Ptr c = NewComponent(id, device);
    return c;
}
