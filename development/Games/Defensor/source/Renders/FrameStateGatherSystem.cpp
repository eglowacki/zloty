#include "Renders/FrameStateGatherSystem.h"
#include "DefensorRenderTypes.h"


//-------------------------------------------------------------------------------------------------
defensor::render::FrameStateGatherSystem::FrameStateGatherSystem(Messaging& messaging, Application& app, RenderCoordinatorSet& coordinatorSet)
    : RenderSystemApp("FrameStateGatherSystem", messaging, app, [this](auto&&... params) { OnUpdate(params...); }, coordinatorSet, true)
{
}


//-------------------------------------------------------------------------------------------------
void defensor::render::FrameStateGatherSystem::OnUpdate(comp::Id_t id, const time::GameClock& /*gameClock*/, metrics::Channel& /*channel*/,
                                                        SceneComponent* sceneComponent)
{
    if (id == comp::GLOBAL_ID_MARKER)
    {
        if (auto payload = mMessaging.ConsumePayload())
        {
            auto entityState = io::cast_data<EntityState>(payload->mBuffer);
            for (auto i = 0; i < payload->mNumEntities; ++i)
            {
                sceneComponent->mEntities.push_back(*entityState);
                entityState++;
            }

            using RenderEntity = comp::RowPolicy<RenderComponent*>;

            comp::ItemIds newFrameRenderIds = sceneComponent->mEntities | std::views::transform([](const auto& e)
            {
                return e.mId;
            }) | std::ranges::to<std::set>();

            auto& coordinator = GetCS().GetCoordinator<RenderEntity>();
            auto oldFrameRenderIds = coordinator.GetItemIds<RenderEntity>();

            comp::ItemIds newIds, deletedIds;
            std::ranges::set_difference(newFrameRenderIds, oldFrameRenderIds, std::inserter(newIds, newIds.end()));
            std::ranges::set_difference(oldFrameRenderIds, newFrameRenderIds, std::inserter(deletedIds, deletedIds.end()));

            auto& vts = mApp.VTS();
            auto& device = GetDevice();
            const auto& adapter = device.GetAdapter();
            std::ranges::for_each(newIds, [this, sceneComponent, &adapter, &vts](const auto& id)
            {
                auto data = sceneComponent->FindState(id);
                Guid guid(data->mAssetGuid);
                auto assetTag = mApp.VTS().FindTag(guid);
                YLOG_CERROR("REND", assetTag.IsValid(), "Render Asset '%s' does not exist.", conv::Convertor<Guid>::ToString(guid).c_str());

                auto textureTag = mApp.VTS().GetTag(io::VirtualTransportSystem::Section("Images@Checker"));
                auto geometryTag = mApp.VTS().GetTag(io::VirtualTransportSystem::Section("Geometry@Rectangle"));

                GetCS().AddComponent<RenderComponent>(id, math3d::Matrix(data->mMatrix), geometryTag, assetTag, io::Tags{ textureTag });
            });

            std::ranges::for_each(deletedIds, [this](const auto& id)
            {
                GetCS().RemoveComponent<RenderComponent>(id);
            });
        }
    }
}


//-------------------------------------------------------------------------------------------------
defensor::render::FrameStateClearSystem::FrameStateClearSystem(Messaging& messaging, Application& app, RenderCoordinatorSet& coordinatorSet)
    : GameSystem("FrameStateClearSystem", messaging, app, [this](auto&&... params) { OnUpdate(params...); }, coordinatorSet, true)
{
}


//-------------------------------------------------------------------------------------------------
void defensor::render::FrameStateClearSystem::OnUpdate(comp::Id_t id, const time::GameClock& /*gameClock*/, metrics::Channel& /*channel*/,
                                                       SceneComponent* sceneComponent)
{
    if (id == comp::GLOBAL_ID_MARKER)
    {
        sceneComponent->mEntities = {};
    }
}
