#include "Renders/RenderSystem.h"
#include "Render/Device.h"
#include "Render/DesktopApplication.h"
#include "Render/Platform/Adapter.h"


//-------------------------------------------------------------------------------------------------
defensor::render::RenderSystem::RenderSystem(Messaging& messaging, Application& app, RenderCoordinatorSet& coordinatorSet)
    : RenderSystemApp("RenderSystem", messaging, app, [this](auto&&... params) {OnUpdate(params...); }, coordinatorSet)
    , mColorInterpolator({ 0.4f, 0.6f, 0.9f, 1.0f }, { 0.6f, 0.9f, 0.4f, 1.0f })
    , mRenderSignatures(GetDevice().GetAdapter().GetDevice())
    , mRenderPipeline(GetDevice().GetAdapter().GetDevice())
{
}


//-------------------------------------------------------------------------------------------------
void defensor::render::RenderSystem::OnUpdate(comp::Id_t id, const time::GameClock& gameClock, metrics::Channel& channel, RenderComponent* /*renderComponent*/, SceneComponent* sceneComponent)
{
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
        auto pipe = mRenderPipeline.GetPipeline(sigFlag, rootSig);

        commandList->SetGraphicsRootSignature(rootSig);
        commandList->SetPipelineState(pipe);
        pipe;

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
