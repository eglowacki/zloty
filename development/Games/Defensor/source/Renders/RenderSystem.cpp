#include "Renders/RenderSystem.h"
#include "Render/Device.h"
#include "Render/DesktopApplication.h"
#include "Render/Platform/Adapter.h"


namespace
{

    //void PreloadAssets(defensor::render::RenderSignatures& renderSignatures, defensor::render::RenderPipeline& renderPipeline, defensor::render::RenderShader& renderShader, yaget::io::VirtualTransportSystem& vts)
    //{
    //    yaget::io::VirtualTransportSystem::Sections shaderSections;
    //    for (const auto& section : shaderSections)
    //    {
    //        
    //    }
    //}
    
}


//-------------------------------------------------------------------------------------------------
defensor::render::RenderSystem::RenderSystem(Messaging& messaging, Application& app, RenderCoordinatorSet& coordinatorSet)
    : RenderSystemApp("RenderSystem", messaging, app, [this](auto&&... params) {OnUpdate(params...); }, coordinatorSet)
    , mAssetPoolThread("PreloadRenderAssets", 1)
    , mColorInterpolator({ 0.4f, 0.6f, 0.9f, 1.0f }, { 0.6f, 0.9f, 0.4f, 1.0f })
    , mRenderSignatures(GetDevice().GetAdapter().GetDevice())
    , mRenderPipeline(GetDevice().GetAdapter().GetDevice())
    , mRenderShader(app.VTS())
{
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
        const colors::Color color = mColorInterpolator.GetColor(gameClock);
        auto& device = GetDevice();
        auto framerHandle = device.GetFramerHandle(gameClock, channel, &color);
        auto commandList = framerHandle.GetCommandList();

        uint64_t PositionSig = 1;
        //uint64_t TextureSig = 2;
        uint64_t ColorSig = 4;
        uint64_t sigFlag = PositionSig|ColorSig;
        auto rootSig = mRenderSignatures.GetSignature(sigFlag);

        auto& vts = mApplication.VTS();

        io::VirtualTransportSystem::Section vertexSection("VeretexShaders@Basic");
        io::VirtualTransportSystem::Section pixelSection("PixelShaders@Basic");
        auto vertexShaderTag = vts.GetTag(vertexSection);
        auto pixelShaderTag = vts.GetTag(pixelSection);

        //static auto vertexGuid = NewGuid();
        //static auto shaderGuid = NewGuid();
        //io::Tag vertexShaderTag{ "EmbeddedVertexShader", vertexGuid };
        //io::Tag pixelShaderTag{ "EmbeddedPixelShader", shaderGuid };
        auto vertexShaderBuffer = mRenderShader.GetShader(vertexShaderTag, RenderShader::ShaderType::Vertex);
        auto pixelShaderBuffer = mRenderShader.GetShader(pixelShaderTag, RenderShader::ShaderType::Pixel);

        uint64_t pipeFlag = sigFlag;
        conv::hash_combine(pipeFlag, vertexShaderTag.Hash(), pixelShaderTag.Hash());
        auto pipe = mRenderPipeline.GetPipeline(pipeFlag, rootSig, vertexShaderBuffer, pixelShaderBuffer);

        commandList->SetGraphicsRootSignature(rootSig);
        commandList->SetPipelineState(pipe);

        coordinator.ForEach<RenderEntity>([commandList](comp::Id_t /*id*/, const auto& row)
        {
            auto renderComponent = std::get<RenderComponent*>(row);
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

                //YLOG_DEBUG("GSYS", "============ Player Position: '%f'", renderComponent->mMatrix.Translation().x);
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

    //io::BLobLoader<io::Asset> shaderLoader(vts, vertexShaderTags);
    //auto shaderList = shaderLoader.Assets();

    yaget::platform::Sleep(1, time::kSecondUnit);

    mAssetsPreloaded = true;
}
