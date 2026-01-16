#include "Renders/FrameStateGatherSystem.h"
#include "DefensorRenderTypes.h"


//-------------------------------------------------------------------------------------------------
defensor::render::FrameStateGatherSystem::FrameStateGatherSystem(Messaging& messaging, Application& app, RenderCoordinatorSet& coordinatorSet)
    : yaget::render::RenderSystemApp<RenderCoordinatorSet, comp::gs::GenerateEndMarker, Messaging, SceneComponent*>("FrameStateGatherSystem", messaging, app, [this](auto&&... params) {OnUpdate(params...); }, coordinatorSet)
{
}


//-------------------------------------------------------------------------------------------------
void defensor::render::FrameStateGatherSystem::OnUpdate(yaget::comp::Id_t id, const yaget::time::GameClock& gameClock, yaget::metrics::Channel& channel, SceneComponent* sceneComponent)
{
    gameClock; channel; sceneComponent;

    if (id == comp::END_ID_MARKER)
    {
    }
    else if (id == comp::GLOBAL_ID_MARKER)
    {

        if (auto payload = mMessaging.ConsumePayload())
        {
            auto entityState = io::cast_data<render::EntityState>(payload->mBuffer);
            for (auto i = 0; i < payload->mNumEntities; ++i)
            {
                sceneComponent->mEntities.push_back(*entityState);
                entityState++;
            }

            using RenderEntity = comp::RowPolicy<RenderComponent*>;

            comp::ItemIds newFrameRenderIds = 
                sceneComponent->mEntities | 
                std::views::transform([](const auto& e)
                {
                    return e.mId;
                }) | 
                std::ranges::to<std::set>();

            auto& coordinator = GetCS().GetCoordinator<RenderEntity>();
            auto oldFrameRenderIds = coordinator.GetItemIds<RenderEntity>();

            comp::ItemIds newIds, deletedIds;
            std::ranges::set_difference(newFrameRenderIds, oldFrameRenderIds, std::inserter(newIds, newIds.end())); 
            std::ranges::set_difference(oldFrameRenderIds, newFrameRenderIds, std::inserter(deletedIds, deletedIds.end())); 

            auto& device = GetDevice();
            const auto& adapter = device.GetAdapter();
            std::ranges::for_each(newIds, [this, sceneComponent, &adapter](const auto& id)
            {
                auto data = sceneComponent->FindState(id);
                GetCS().AddComponent<RenderComponent>(id, math3d::Matrix(data->mMatrix), adapter);
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
    : GameSystem("FrameStateClearSystem", messaging, app, [this](auto&&... params) {OnUpdate(params...); }, coordinatorSet)
{
}


//-------------------------------------------------------------------------------------------------
void defensor::render::FrameStateClearSystem::OnUpdate(yaget::comp::Id_t id, const yaget::time::GameClock& gameClock, yaget::metrics::Channel& channel, SceneComponent* sceneComponent)
{
    gameClock; channel; sceneComponent;

    if (id == comp::END_ID_MARKER)
    {
    }
    else if (id == comp::GLOBAL_ID_MARKER)
    {
        sceneComponent->mEntities = {};
    }
}
