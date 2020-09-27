#include "Components/TerrainComponent.h"
#if 0

#include "Device.h"
#include "App/Application.h"
#include "App/AppUtilities.h"
#include "Logger/YLog.h"
#include "Debugging/Assert.h"
#include "Scene.h"
#include "RenderHelpers.h"
#include "MathFacade.h"
#include "imgui.h"
#include <fstream>

using namespace yaget;
using namespace DirectX;


yaget::render::TerrainComponent::TerrainComponent(comp::Id_t id, Device& device, Scene& scene)
    : RenderComponent(id, device, Init::Default, {})
    , mTerrainBounds(0.0f)
    , mScene(scene)
{
    AddSupportedTrigger(render::TerrainComponent::SignalChunkLoaded);
}

void render::TerrainComponent::Tick(const time::GameClock& /*gameClock*/)
{
}

void render::TerrainComponent::OnRender(const RenderBuffer& /*renderBuffer*/, const DirectX::SimpleMath::Matrix& /*viewMatrix*/, const DirectX::SimpleMath::Matrix& /*projectionMatrix*/)
{
    //math::Box boundingBox = mTerrainBounds;
    //float nearPlane = boundingBox.mMin.y;
    //float farPlane = boundingBox.mMax.y;
    //math3d::Vector4 terrainExtends(nearPlane, farPlane, 0, 0);
    //mTerrainExtends->Data() = terrainExtends;
    //mTerrainExtends->Commit(0);
}

void render::TerrainComponent::OnGuiRender(const RenderBuffer& /*renderBuffer*/, const DirectX::SimpleMath::Matrix& /*viewMatrix*/, const DirectX::SimpleMath::Matrix& /*projectionMatrix*/)
{
    //if (ImGui::Begin("Yaget Debug Options"))
    //{
    //    ImGui::Separator();
    //    ImGui::SliderFloat("Terrain Depth Range Max", &mTerrainBounds.mMax.y, mOriginalTerrainBounds.mMin.y, mOriginalTerrainBounds.mMax.y);
    //    ImGui::SliderFloat("Terrain Depth Range Min", &mTerrainBounds.mMin.y, mOriginalTerrainBounds.mMin.y, mOriginalTerrainBounds.mMax.y);
    //}
    //ImGui::End();
}

void render::TerrainComponent::OnReset()
{
    //mTerrainExtends = std::make_unique<ConstantBuffer<math3d::Vector4, render::BindPixelConstant>>(mDevice);
}

void yaget::render::TerrainComponent::ImportTerrain(const Sections& sections)
{
    comp::ComponentPools& pools = mScene.Pools();
    std::vector<io::Tag> tags = mDevice.App().VTS().GetTags(sections);
    mChunkStreamTime = platform::GetRealTime();
    mChunkIds.clear();
    mTerrainBounds = math::Box(0);

    for (const auto& tag : tags)
    {
        comp::Id_t itemId = mScene.NewItem(math3d::Matrix::Identity);
        if (render::ModelComponent* modelComponent = pools.mModel.Find(itemId))
        {
            mChunkIds.push_back(itemId);
            using namespace std::placeholders;
            modelComponent->ConnectTrigger(render::ModelComponent::SignalViewCreated, [this](auto&& param1) { OnChunkUpdate(param1); });
            modelComponent->AttachAsset(tag);
        }
    }
}

void render::TerrainComponent::OnChunkUpdate(const comp::Component& /*from*/)
{
    double currTime = platform::GetRealTime();
    double diff = currTime - mChunkStreamTime;
    YLOG_DEBUG("TERR", "Chunk loaded in: '%f'.", diff);

    math::Box bounds;
    comp::ComponentPools& pools = mScene.Pools();
    for (auto&& id : mChunkIds)
    {
        if (render::ModelComponent* modelComponent = pools.mModel.Find(id))
        {
            bounds.GrowExtends(modelComponent->BoundingBox());
        }
    }

    if (mChunkIds.empty())
    {
        bounds = math::Box::Zero();
    }

    if (render::LineComponent* lineComponent = pools.mLine.Find(Id()))
    {
        render::draw::BoundingBox(bounds, lineComponent);
    }

    mTerrainBounds = bounds;

    TriggerSignal(TerrainComponent::SignalChunkLoaded);
}

math::Box render::TerrainComponent::BoundingBox() const
{
    return mTerrainBounds;
}

render::TerrainComponentPool::Ptr render::TerrainComponentPool::New(comp::Id_t id, Device& device, Scene& scene)
{
    Ptr c = NewComponent(id, device, scene);
    return c;
}
#endif // 0