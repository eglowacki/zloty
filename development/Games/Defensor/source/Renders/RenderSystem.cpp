#include "MemoryManager/NewAllocator.h"
#include "Render/DesktopApplication.h"
#include "Render/Device.h"
#include "Render/Pipeline/ShaderBuffers.h"
#include "Render/Platform/Adapter.h"
#include "Renders/RenderSystem.h"
#include "Render/UI/FontRender.h"


extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = 619;}
extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\"; }


//-------------------------------------------------------------------------------------------------
defensor::render::RenderSystem::RenderSystem(Messaging& messaging, Application& app, RenderCoordinatorSet& coordinatorSet)
    : RenderSystemApp("RenderSystem", messaging, app, [this](auto&&... params) { OnUpdate(params...); }, coordinatorSet, false)
    , mColorInterpolator({ 0.4f, 0.6f, 0.9f, 1.0f }, { 0.6f, 0.9f, 0.4f, 1.0f })
    , mMatrixInterpolator(0.0f, 1.0f)
    , mPipelineContext{ GetDevice(), app.VTS(), [this](const auto event){ mMessaging.Dispatch(event, Messaging::DispatcherType::Logic); } }
    , mResizeCallbackId{ GetDevice().RegisterResizeCallback([this](auto&&... params) { OnResetDevice(params...); }) }
{
    if (mApp.Input().IsAction("Quit App"))
    {
        mApp.Input().RegisterSimpleActionCallback("Quit App", [this]() { mApplicationQuiting = true; });
    }

    mAssetPreloader.AddTask([this]()
    {
        PreloadAssets();
    });
}


//-------------------------------------------------------------------------------------------------
colors::Color lerp(const colors::Color& a, const colors::Color& b, float t)
{
    return colors::Color::Lerp(a, b, t);
}


//-------------------------------------------------------------------------------------------------
void defensor::render::RenderSystem::OnUpdate(comp::Id_t id, const time::GameClock& gameClock, metrics::Channel& channel, const SceneComponent* sceneComponent)
{
    using namespace yaget::render;
    using RenderEntity = comp::RowPolicy<RenderComponent*>;
    auto& coordinator = GetCS().GetCoordinator<RenderEntity>();

    if (id == comp::END_ID_MARKER)
    {
        memory::StartRecordAllocations();
        auto fontTag = mApp.VTS().GetTag(Section{ "SceneItems@MediumFont" });

        constexpr float FpsAlpha = 0.5f;
        // let's show frame rate here, using stb library for generating text
        mFramesThisSecond++;
        mCurrentCalcTime += gameClock.GetDeltaTimeSecond();
        if (mCurrentCalcTime > 1.0f)
        {
            auto surface = mApp.GetSurface().GetSize<int>();

            auto resX = std::get<0>(surface);
            auto resY = std::get<1>(surface);
            mAverageFps = FpsAlpha * mAverageFps + (1.0f - FpsAlpha) * mFramesThisSecond;
            auto framePerSecond = std::format("FPS: {} ", static_cast<uint32_t>(mAverageFps));
            auto milliPerFrame = std::format("Ms: {:.4f} ", 1000.0f / mAverageFps);
            auto resolution = std::format("{}x{} ", resX, resY);

            ui::TextPrinters textPrinters
            {
                { .mText = framePerSecond, .mX = 10, .mY = 10, .mSize = 2.0f, .mColor = math3d::Color{ colors::Red } },
                { .mText = milliPerFrame, .mColor = math3d::Color{ colors::Yellow } },
                { .mText = resolution, .mColor = math3d::Color{ colors::White } }
            };

            mPipelineContext.UpdateText(fontTag, textPrinters, commands::Type::Direct);

            mCurrentCalcTime -= 1.0f;
            mFramesThisSecond = 0;
        }

        const auto& renderPasses = mPipelineContext.RenderPasses().GetPasses();
        auto& device = GetDevice();

        for (const auto& renderPass: renderPasses)
        {
            mCurrentRenderPassState = {};

            struct ItemToRender
            {
                scene::SceneItem* mItem{};
                math3d::Matrix mWorldViewProj;
                float mTime{};
            };

            std::vector<ItemToRender> itemsToRender;
            auto renderTarget = mPipelineContext.FindRenderTarget(renderPass.mRenderTargetTag);
            auto colorClear = renderPass.GetColorClear(renderTarget);
            auto depthClearValue = renderPass.GetDepthStencilClear(renderTarget);

            auto frameCommands = device.GetFrameCommands(*renderTarget, gameClock, channel);
            auto currentFrameIndex = frameCommands.GetFrameIndex();
            auto commandList = frameCommands.BeginFrame(colorClear, depthClearValue);
            auto viewMatrix = renderPass.GetViewMatrix();
            auto orthoMatrix = renderPass.GetProjectionMatrix();
            auto commandType = commandList->GetType();

            if (renderPass.mSceneItemTags.empty())
            {
                coordinator.ForEach<RenderEntity>([&itemsToRender, &viewMatrix, &orthoMatrix, &renderPass, this](comp::Id_t /*id*/, const auto& row)
                {
                    auto renderComponent = std::get<RenderComponent*>(row);
                    if (mPipelineContext.RenderPasses().RenderThisPass(renderComponent->mSceneItemTag, renderPass))
                    {
                        auto sceneItem = mPipelineContext.GetSceneItem(renderComponent->mSceneItemTag);

                        auto worldViewProj = (renderComponent->mMatrix * viewMatrix * orthoMatrix).Transpose();
                        float timeData = 1.0f;

                        itemsToRender.push_back({ .mItem = sceneItem, .mWorldViewProj = worldViewProj, .mTime = timeData });
                    }

                    return true;
                });
            }
            else
            {
                for (const auto& sceneItemTag : renderPass.mSceneItemTags)
                {
                    auto sceneItem = mPipelineContext.GetSceneItem(sceneItemTag);

                    auto worldViewProj = (viewMatrix * orthoMatrix).Transpose();
                    float timeData = 1.0f;

                    itemsToRender.push_back({ .mItem = sceneItem, .mWorldViewProj = worldViewProj, .mTime = timeData });
                }
            }

            std::ranges::sort(itemsToRender, [](const ItemToRender& item1, const ItemToRender& item2)
            {
                return item1.mItem->GetRenderOrder() < item2.mItem->GetRenderOrder();
            });

            //scene::SceneItemsStorage::SortSceneItems(itemsToRender);
            std::ranges::for_each(itemsToRender, [commandList, currentFrameIndex, commandType, this](const ItemToRender& item)
            {
                item.mItem->UpdateData(currentFrameIndex, constant_shader_types::ConstantTypes::WorldViewProjection, item.mWorldViewProj, commandType);
                item.mItem->UpdateData(currentFrameIndex, constant_shader_types::ConstantTypes::Time, item.mTime, commandType);
                item.mItem->Render(currentFrameIndex, commandList, mCurrentRenderPassState);
            });

            frameCommands.EndFrame();
        }

        memory::StopRecordAllocations();
    }
    else
    {
        const auto& newFrameRenderIds = sceneComponent->GetIds();

        mPipelineContext.RenderPasses().BindAsset(sceneComponent->mRenderPassTag);

        coordinator.ForEach<RenderEntity>(newFrameRenderIds, [sceneComponent, &vts = mApp.VTS(), this](comp::Id_t id, const auto& row)
        {
            if (auto data = sceneComponent->FindState(id))
            {
                auto renderComponent = std::get<RenderComponent*>(row);
                renderComponent->UpdateMatrix(math3d::Matrix(data->mMatrix));

                if (renderComponent->mSceneItemTag.mGuid != Guid(data->mAssetGuid))
                {
                    renderComponent->mSceneItemTag = vts.FindTag(Guid(data->mAssetGuid));
                }
            }

            return true;
        });
    }
}


//-------------------------------------------------------------------------------------------------
void defensor::render::RenderSystem::PreloadAssets()
{
    //mPipelineContext.PreloadAssets("Boot");
    //mMessaging.Dispatch(items::StageEvent{ "Asset Load", items::db_stage::BlendOp::Replace }, Messaging::DispatcherType::Logic);

    mPipelineContext.PreloadAssets("");
    mMessaging.Dispatch(items::StageEvent{ "Main Menu", items::db_stage::BlendOp::Replace }, Messaging::DispatcherType::Logic);

    SetTickEnabled(true);
}


//-------------------------------------------------------------------------------------------------
void defensor::render::RenderSystem::OnResetDevice(const app::WindowFrame& windowFrame, yaget::render::DeviceB::ResizeState resizeState)
{
    mPipelineContext.OnResetDevice(windowFrame, resizeState);
}
