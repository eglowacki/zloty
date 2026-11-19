#include "Systems/FrameStateCollectorSystem.h"
#include "DefensorRenderTypes.h"


YAGET_COMPILE_SUPPRESS_START(4189, "local variable is initialized but not referenced")
YAGET_COMPILE_SUPPRESS_START(4100, "'variable': unreferenced parameter")

//-------------------------------------------------------------------------------------------------
namespace
{
    //-------------------------------------------------------------------------------------------------
    template <typename CS, typename... Comps>
    void ProcessComponents(auto& app, auto& currentFrameState, CS& coordinatorSet, auto callback)
    {
        using namespace yaget;

        using RowPolicy = comp::RowPolicy<Comps*...>;
        constexpr size_t coordinatorIndex = comp::metatypes::coordinator_index_for_components<typename RowPolicy::Row, CS>;
        constexpr size_t entitySize = sizeof(defensor::render::EntityState);

        YAGET_ASSERT(coordinatorIndex != comp::metatypes::InvalidCoordinatorIndex, "No Coordinator for requested Components: '%s'.", 
            conv::Combine(comp::db::GetPolicyRowNames<typename RowPolicy::Row>(), ", ").c_str());

        if (coordinatorIndex != comp::metatypes::InvalidCoordinatorIndex)
        {
            auto& coordinator = coordinatorSet.template GetCoordinator<coordinatorIndex>();
            comp::ItemIds itemIds = coordinator.template GetItemIds<RowPolicy>();

            coordinator.template ForEach<RowPolicy>(itemIds, [&callback, &currentFrameState, &app](comp::Id_t itemId, const auto& row)
            {
                if constexpr (meta::element_type_in_tuple_v<comp::MaterialComponent, typename RowPolicy::Row>)
                {
                    auto materialComponent = std::get<comp::MaterialComponent*>(row);
                    if (!materialComponent->mAssetTag.IsValid())
                    {
                        materialComponent->mAssetTag = app.VTS().GetTag(materialComponent->template GetValue<comp::db_material::Section>());
                    }
                }

                defensor::render::EntityState entityState = callback(itemId, row);

                ++(currentFrameState->mNumEntities);
                currentFrameState->AssureWriteSize(entitySize);
                currentFrameState->WriteDataChunk(&entityState, sizeof(entityState));

                return true;
            });
        }
    }

    //-------------------------------------------------------------------------------------------------
    void ExtractMatrix(const yaget::comp::LocationComponent3* locationComponent, defensor::render::EntityState& entityState)
    {
        const auto matrix = locationComponent->Matrix();
        math3d::GetMatrixAsFloats(matrix, entityState.mMatrix);
    }

    //-------------------------------------------------------------------------------------------------
    void ExtractGuid(const yaget::comp::MaterialComponent* materialComponent, defensor::render::EntityState& entityState)
    {
        constexpr auto guidSize = sizeof(yaget::Guid::DataBuffer);
        memcpy(entityState.mAssetGuid, materialComponent->mAssetTag.mGuid.bytes().data(), guidSize);
    }

}


//-------------------------------------------------------------------------------------------------
defensor::game::FrameStateCollectorSystem::FrameStateCollectorSystem(Messaging& messaging, Application& app, GameCoordinatorSet& coordinatorSet)
    : GameSystem("FrameStateCollectorSystem", messaging, app, [this](auto&&... params) { OnUpdate(params...); }, coordinatorSet, true)
{
}


//-------------------------------------------------------------------------------------------------
void defensor::game::FrameStateCollectorSystem::OnUpdate(comp::Id_t id, const time::GameClock& /*gameClock*/, metrics::Channel& /*channel*/)
{
    if (id == comp::END_ID_MARKER)
    {
        constexpr size_t entitySize = sizeof(render::EntityState);
        auto currentFrameState = mMessaging.CreatePayload(entitySize * mUsedEntitiesNumber);

        //----------------------------------------------------------------------------------------------------------------------------------
        ProcessComponents<CoordinatorSet, comp::LocationComponent3, comp::MaterialComponent>(mApp, currentFrameState, GetCS(), [&currentFrameState, &app = mApp](comp::Id_t itemId, const auto& row)
        {
            auto& [locationComponent, materialComponent] = row;

            render::EntityState entityState{ .mId = comp::StripQualifiers(itemId) };

            ExtractMatrix(locationComponent, entityState);
            ExtractGuid(materialComponent, entityState);

            return entityState;
        });

        //----------------------------------------------------------------------------------------------------------------------------------
        ProcessComponents<CoordinatorSet, comp::LocationComponent3, comp::TextComponent>(mApp, currentFrameState, GetCS(), [&currentFrameState, &app = mApp](comp::Id_t itemId, const auto& row)
        {
            auto& [locationComponent, textComponent] = row;
            
            render::EntityState entityState{ .mId = comp::StripQualifiers(itemId), .mAssetType = render::EntityState::AssetType::Text };
            ExtractMatrix(locationComponent, entityState);

            return entityState;
        });

        //----------------------------------------------------------------------------------------------------------------------------------
        ProcessComponents<CoordinatorSet, comp::RenderPassComponent, comp::MaterialComponent>(mApp, currentFrameState, GetCS(), [&currentFrameState, &app = mApp](comp::Id_t itemId, const auto& row)
        {
            auto& [renderPassComponent, materialComponent] = row;
            
            render::EntityState entityState{ .mId = comp::StripQualifiers(itemId), .mAssetType = render::EntityState::AssetType::RenderPass };
            ExtractGuid(materialComponent, entityState);

            return entityState;
        });

        mUsedEntitiesNumber = currentFrameState->mNumEntities;
        mMessaging.SetPayload(currentFrameState);
    }
}

YAGET_COMPILE_SUPPRESS_END
YAGET_COMPILE_SUPPRESS_END
