#include "Renders/RenderSystem.h"
#include "Render/Device.h"
#include "Render/DesktopApplication.h"
#include "Render/Platform/Adapter.h"


namespace
{
    //---------------------------------------------------------------------------------
    std::array<float, 16> GetMatrixAsFloats(const math3d::Matrix& matrix)
    {
        std::array<float, 16> floatArray;
        DirectX::XMStoreFloat4x4(reinterpret_cast<DirectX::XMFLOAT4X4*>(floatArray.data()), matrix);

        return floatArray;
    }

    yaget::io::Tag TypeToTag(yaget::render::AssetCacheType assetCacheType, yaget::io::VirtualTransportSystem& vts)
    {
        auto section = yaget::render::AssetCache::operator[](assetCacheType);
        auto tag = vts.GetTag(section);
        if (!tag.IsValid())
        {
            tag = vts.GenerateTag(section);
        }

        return tag;
    }

}


//-------------------------------------------------------------------------------------------------
defensor::render::RenderSystem::RenderSystem(Messaging& messaging, Application& app, RenderCoordinatorSet& coordinatorSet)
    : RenderSystemApp("RenderSystem", messaging, app, [this](auto&&... params) {OnUpdate(params...); }, coordinatorSet)
    , mAssetPoolThread("PreloadRenderAssets", 1)
    , mColorInterpolator({ 0.4f, 0.6f, 0.9f, 1.0f }, { 0.6f, 0.9f, 0.4f, 1.0f })
    , mDependencyGraph(app.VTS(), io::VirtualTransportSystem::Section("Manifest@RenderDependencies"))
    , mRenderSignatures(GetDevice().GetAdapter().GetDevice(), app.VTS())
    , mRenderPipeline(GetDevice().GetAdapter().GetDevice(), app.VTS())
    , mRenderShader(app.VTS())
{
    using namespace yaget::render;

    mAssetPoolThread.AddTask([this]()
    {
        PreloadAssets();
    });
}


//-------------------------------------------------------------------------------------------------
void defensor::render::RenderSystem::OnUpdate(comp::Id_t id, const time::GameClock& gameClock, metrics::Channel& channel, RenderComponent* /*renderComponent*/, SceneComponent* sceneComponent)
{
    if (!mAssetsPreloaded)
    {
        return;
    }

    using RenderEntity = comp::RowPolicy<RenderComponent*>;
    auto& coordinator = GetCS().GetCoordinator<RenderEntity>();

    if (id == comp::END_ID_MARKER)
    {
        auto& vts = mApplication.VTS();

        const colors::Color color = mColorInterpolator.GetColor(gameClock);
        auto& device = GetDevice();
        auto framerHandle = device.GetFramerHandle(gameClock, channel, &color);
        auto commandList = framerHandle.GetCommandList();

        auto signatureTag = TypeToTag(yaget::render::BasicSignature, vts);
        auto rootSig = mRenderSignatures.GetSignature(signatureTag);

        auto vertexShaderTag = TypeToTag(yaget::render::BasicVertex, vts);
        auto pixelShaderTag = TypeToTag(yaget::render::BasicPixel, vts);
        auto vertexShaderBuffer = mRenderShader.GetShader(vertexShaderTag, RenderShader::ShaderType::Vertex);
        auto pixelShaderBuffer = mRenderShader.GetShader(pixelShaderTag, RenderShader::ShaderType::Pixel);

        auto pipelineTag = TypeToTag(yaget::render::BasicPipeline, vts);
        //DependencyNode* psoNode = mDependencyGraph.Find(pipelineTag.mGuid);

        auto pipe = mRenderPipeline.GetPipeline(pipelineTag, rootSig, vertexShaderBuffer, pixelShaderBuffer);

        commandList->SetGraphicsRootSignature(rootSig);
        commandList->SetPipelineState(pipe);

        coordinator.ForEach<RenderEntity>([commandList](comp::Id_t /*id*/, const auto& row)
        {
            auto renderComponent = std::get<RenderComponent*>(row);
            const auto location = renderComponent->mMatrix;

            auto matrix = GetMatrixAsFloats(location);
            std::ranges::fill(matrix, 0.75f);

            commandList->SetGraphicsRoot32BitConstants(0, 16, matrix.data(), 0);
            renderComponent->Render(commandList);

            return true;
        });
    }
    else
    {
        const auto& newFrameRenderIds = sceneComponent->GetIds();

        coordinator.ForEach<RenderEntity>(newFrameRenderIds, [sceneComponent](comp::Id_t id, const auto& row)
        {
            if (auto data = sceneComponent->FindState(id))
            {
                auto renderComponent = std::get<RenderComponent*>(row);
                renderComponent->mMatrix = math3d::Matrix(data->mMatrix);
            }

            return true;
        });
    }
}


//-------------------------------------------------------------------------------------------------
void defensor::render::RenderSystem::PreloadAssets()
{
    // we need to have some kind of manifest file which will enumerate all the files that need to be post process and saved into a cache
    auto& vts = mApplication.VTS();
    const io::VirtualTransportSystem::Section vertexShaderSection("VeretexShaders");
    auto vertexShaderTags = vts.GetTags(vertexShaderSection);

    const io::VirtualTransportSystem::Section pixelShaderSection("PixelShaders");
    auto pixelShaderTags = vts.GetTags(pixelShaderSection);

    mRenderShader.GetShaders(vertexShaderTags, RenderShader::ShaderType::Vertex);
    mRenderShader.GetShaders(pixelShaderTags, RenderShader::ShaderType::Pixel);

    yaget::platform::Sleep(1, time::kSecondUnit);

    mAssetsPreloaded = true;
}
