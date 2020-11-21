#include "Components/LineComponent.h"
#include "Device.h"
#include "RenderTarget.h"
#include "Debugging/Assert.h"
#include "Exception/Exception.h"
#include "RenderHelpers.h"
#include "Fmt/format.h"
#include "VertexTypes.h"
#include "CommonStates.h"
#include "Effects.h"
#include "App/Application.h"

using namespace yaget;
using namespace DirectX;


render::LineComponent::LineComponent(comp::Id_t id, render::Device& device, bool bScreenSpace)
    : render::RenderComponent(id, device, Init::AutoReset, {})
    , mScreenSpace(bScreenSpace)
{
}

void render::LineComponent::OnReset()
{
    Device::ID3D11Device_t* d3dDevice = mDevice.GetDevice();
    Device::ID3D11DeviceContext_t* d3dDeviceContext = mDevice.GetDeviceContext();

    YAGET_ASSERT(d3dDevice, "LineComponent did not get valid d3dDevice pointer.");
    YAGET_ASSERT(d3dDeviceContext, "LineComponent did not have d3dContext attached.");

    try
    {
        // NOTE: mEffect and mStates should come from Device, since it can manage duplicates
        mEffect = std::make_unique<BasicEffect>(d3dDevice);
        mBatch = std::make_unique<PrimitiveBatch<VertexPositionColor>>(d3dDeviceContext);
        mStates = std::make_unique<CommonStates>(d3dDevice);
    }
    catch (const std::exception& e)
    {
        const auto& textError = fmt::format("Did not initialize LineComponent '{}'. Error: {}", Id(), e.what());
        YAGET_UTIL_THROW("REND", textError);
    }

    mEffect->SetVertexColorEnabled(true);
    void const* shaderByteCode = nullptr;
    size_t byteCodeLength = 0;

    mEffect->GetVertexShaderBytecode(&shaderByteCode, &byteCodeLength);

    HRESULT hr = d3dDevice->CreateInputLayout(VertexPositionColor::InputElements, VertexPositionColor::InputElementCount,
        shaderByteCode, byteCodeLength,
        mInputLayout.ReleaseAndGetAddressOf());
    YAGET_UTIL_THROW_ON_RROR(hr, "Could not create input layout for line component");
}

render::LineComponent::~LineComponent()
{ }

void render::LineComponent::Draw(const LinesPayload& lines)
{
    mLineStager.SetPayload(lines);
}

void render::LineComponent::onRender(const RenderTarget* renderTarget, const math3d::Matrix& /*matrix*/)
{
    if (auto payload = mLineStager.GetPayload())
    {
        YAGET_ASSERT(renderTarget, "Render Target parameter is nullptr.");

        Device::ID3D11DeviceContext_t* deviceContext = mDevice.GetDeviceContext();

        if (mScreenSpace)
        {
            // use RenderTarget facility to query size 
            deviceContext->OMSetDepthStencilState(mStates->DepthNone(), 1);
            //SimpleMath::Vector2 screenSize = mDevice.GetWindowSize();
            auto dimensions = renderTarget->Size<float>();
            //SimpleMath::Matrix orthoMatrix = math3d::Matrix::Identity;// math3d::Matrix::CreateOrthographicOffCenter(0.0f, static_cast<float>(dimensions.first), static_cast<float>(dimensions.second), 0.0f, 0.0f, 1.0f);

            SimpleMath::Matrix orthoMatrix = math3d::Matrix::CreateOrthographic(4.0f, 4.0f, 0.0f, 1.0f);
            //XMMatrixOrthographicOffCenterLH(0.0f, (float)m_pRenderManager->GetWidth(), (float)m_pRenderManager->GetHeight(), 0.0f, 0.0f, 100.0f);

            //SimpleMath::Matrix orthoMatrix = SimpleMath::Matrix::CreateOrthographicOffCenter(0, screenSize.x, screenSize.y, 0, 0, 10000);

            mEffect->SetMatrices(math3d::Matrix::Identity, math3d::Matrix::Identity, orthoMatrix);
        }
        else
        {
            deviceContext->OMSetDepthStencilState(mStates->DepthDefault(), 1);
        }

        mEffect->Apply(deviceContext);
        deviceContext->IASetInputLayout(mInputLayout.Get());

        mBatch->Begin();

        for (const auto& line : *payload)
        {
            VertexPositionColor v1(line.p0, line.c0);
            VertexPositionColor v2(line.p1, line.c1);
            mBatch->DrawLine(v1, v2);
        }

        mBatch->End();
    }
}

void render::LineComponent::OnRender(const RenderBuffer& renderBuffer, const DirectX::SimpleMath::Matrix& viewMatrix, const DirectX::SimpleMath::Matrix& projectionMatrix)
{
    LinesPtr_t lines = std::atomic_load(&mLinesToRender);
    if (lines)
    {
        Device::ID3D11DeviceContext_t* d3dDeviceContext = mDevice.GetDeviceContext();

        if (mScreenSpace)
        {
            d3dDeviceContext->OMSetDepthStencilState(mStates->DepthNone(), 1);
            SimpleMath::Vector2 screenSize = mDevice.App().GetSurface().Size();
            SimpleMath::Matrix matrix = SimpleMath::Matrix::CreateOrthographicOffCenter(0, screenSize.x, screenSize.y, 0, 0, 10000);
            mEffect->SetMatrices(SimpleMath::Matrix::Identity, SimpleMath::Matrix::Identity, matrix);
        }
        else
        {
            d3dDeviceContext->OMSetDepthStencilState(mStates->DepthDefault(), 1);
            mEffect->SetMatrices(renderBuffer.Matrix, viewMatrix, projectionMatrix);
        }

        mEffect->Apply(d3dDeviceContext);
        d3dDeviceContext->IASetInputLayout(mInputLayout.Get());

        mBatch->Begin();

        size_t num = lines->size();
        for (size_t i = 0; i < num; ++i)
        {
            const Line& line = (*lines)[i];

            VertexPositionColor v1(line.p0, line.c0);
            VertexPositionColor v2(line.p1, line.c1);
            mBatch->DrawLine(v1, v2);
        }

        mBatch->End();
    }
}

void render::LineComponent::OnGuiRender(const RenderBuffer& /*renderBuffer*/, const DirectX::SimpleMath::Matrix& /*viewMatrix*/, const DirectX::SimpleMath::Matrix& /*projectionMatrix*/)
{
}
