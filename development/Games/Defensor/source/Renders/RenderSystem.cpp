
#include "MemoryManager/NewAllocator.h"
#include "Render/DesktopApplication.h"
#include "Render/Device.h"
#include "Render/Pipeline/ShaderBuffers.h"
#include "Render/Platform/Adapter.h"
#include "Renders/RenderSystem.h"
#include "Render/UI/FontRender.h"


extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = D3D12_SDK_VERSION;}
extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\"; }


//-------------------------------------------------------------------------------------------------
defensor::render::RenderSystem::RenderSystem(Messaging& messaging, Application& app, RenderCoordinatorSet& coordinatorSet)
    : RenderSystemApp("RenderSystem", messaging, app, [this](auto&&... params) { OnUpdate(params...); }, coordinatorSet, false)
    , mColorInterpolator({ 0.4f, 0.6f, 0.9f, 1.0f }, { 0.6f, 0.9f, 0.4f, 1.0f })
    , mMatrixInterpolator(0.0f, 1.0f)
{
    mAssetPreloader.AddTask([this]()
    {
        PreloadAssets();
    });
}


////-------------------------------------------------------------------------------------------------
//colors::Color lerp(const colors::Color& a, const colors::Color& b, float t)
//{
//    return colors::Color::Lerp(a, b, t);
//}


//-------------------------------------------------------------------------------------------------
void defensor::render::RenderSystem::OnUpdate(comp::Id_t id, const time::GameClock& gameClock, metrics::Channel& channel, const SceneComponent* sceneComponent)
{
    YAGET_ASSERT(mPipelineContext, "Pipeline context is not initialized");

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
            auto [resX, resY] = mApp.GetSurface().GetSize<int>();

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

            mPipelineContext->UpdateText(fontTag, textPrinters, commands::Type::Direct);

            mCurrentCalcTime -= 1.0f;
            mFramesThisSecond = 0;
        }

        mPipelineContext->Render(GetCS(), gameClock, channel);

        memory::StopRecordAllocations();
    }
    else
    {
        const auto& newFrameRenderIds = sceneComponent->GetIds();

        mPipelineContext->RenderPasses().BindAsset(sceneComponent->mRenderPassTag);

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

    mPipelineContext = nullptr;
    mPipelineContext = std::make_unique<yaget::render::PipelineContext>(GetDevice(), mApp.VTS(), "Boot", mApp.Director(), [this](const auto event){ mMessaging.Dispatch(event, Messaging::DispatcherType::Logic); } );


    mPipelineContext->PreloadAssets("");
    //mMessaging.Dispatch(items::StageEvent{ "Main Menu", items::db_stage::BlendOp::Replace }, Messaging::DispatcherType::Logic);

    SetTickEnabled(true);
}
