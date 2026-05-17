#include "Systems/FrameStateCollectorSystem.h"
#include "DefensorRenderTypes.h"


//-------------------------------------------------------------------------------------------------
defensor::game::FrameStateCollectorSystem::FrameStateCollectorSystem(Messaging& messaging, Application& app, GameCoordinatorSet& coordinatorSet)
    : GameSystem("FrameStateCollectorSystem", messaging, app, [this](auto&&... params) { OnUpdate(params...); }, coordinatorSet, true)
{
}


//-------------------------------------------------------------------------------------------------
void defensor::game::FrameStateCollectorSystem::OnUpdate(comp::Id_t id, const time::GameClock& /*gameClock*/, metrics::Channel& /*channel*/)
{                                                         
    using RenderableEntity = comp::RowPolicy<comp::LocationComponent3*, comp::MaterialComponent*>;
    using MenuTextEntity = comp::RowPolicy<comp::LocationComponent3*, comp::TextComponent*>;
    auto& coordinator = GetCS().GetCoordinator<defensor::game::Entity>();

    if (id == comp::END_ID_MARKER)
    {
        constexpr size_t entitySize = sizeof(render::EntityState);

        comp::ItemIds renderableItemIds = coordinator.GetItemIds<RenderableEntity>();
        comp::ItemIds menuTextItemIds = coordinator.GetItemIds<MenuTextEntity>();
        auto currentFrameState = mMessaging.CreatePayload((renderableItemIds.size() + menuTextItemIds.size()) * entitySize);

        coordinator.ForEach<RenderableEntity>(renderableItemIds, [&currentFrameState, this](comp::Id_t itemId, const auto& row)
        {
            auto locationComponent = std::get<comp::LocationComponent3*>(row);
            auto materialComponent = std::get<comp::MaterialComponent*>(row);

            currentFrameState->mNumEntities++;
            currentFrameState->AssureWriteSize(entitySize);
            const auto location = locationComponent->Matrix();

            render::EntityState entityState{ .mId = comp::StripQualifiers(itemId) };
            math3d::GetMatrixAsFloats(location, entityState.mMatrix);

            if (!materialComponent->mAssetTag.IsValid())
            {
                materialComponent->mAssetTag = mApp.VTS().GetTag(materialComponent->template GetValue<comp::db_material::Section>());
            }

            constexpr auto guidSize = sizeof(Guid::DataBuffer);
            memcpy(entityState.mAssetGuid, materialComponent->mAssetTag.mGuid.bytes().data(), guidSize);
            currentFrameState->WriteDataChunk(&entityState, sizeof(entityState));

            return true;
        });

        coordinator.ForEach<MenuTextEntity>(menuTextItemIds, [&currentFrameState, this](comp::Id_t itemId, const auto& row)
        {
            auto locationComponent = std::get<comp::LocationComponent3*>(row);
            auto textComponent = std::get<comp::TextComponent*>(row);
            textComponent;

            currentFrameState->mNumEntities++;
            currentFrameState->AssureWriteSize(entitySize);

            const auto location = locationComponent->Matrix();

            render::EntityState entityState{ .mId = comp::StripQualifiers(itemId) };
            math3d::GetMatrixAsFloats(location, entityState.mMatrix);

            currentFrameState->WriteDataChunk(&entityState, sizeof(entityState));

            return true;
        });

        mMessaging.SetPayload(currentFrameState);
    }
}
