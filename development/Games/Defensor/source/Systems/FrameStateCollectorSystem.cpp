#include "Systems/FrameStateCollectorSystem.h"
#include "DefensorRenderTypes.h"


//-------------------------------------------------------------------------------------------------
defensor::game::FrameStateCollectorSystem::FrameStateCollectorSystem(Messaging& messaging, Application& app, GameCoordinatorSet& coordinatorSet)
    : GameSystem("FrameStateCollectorSystem", messaging, app, [this](auto&&... params) {OnUpdate(params...); }, coordinatorSet)
    , mCurrentFrameState(messaging.CreatePayload(/*sizeof(EntityState)*/))
{
}


//-------------------------------------------------------------------------------------------------
void defensor::game::FrameStateCollectorSystem::OnUpdate(yaget::comp::Id_t id, const yaget::time::GameClock& /*gameClock*/, yaget::metrics::Channel& /*channel*/, comp::LocationComponent3* locationComponent, comp::MaterialComponent* materialComponent)
{
    constexpr size_t entitySize = sizeof(render::EntityState);

    if (id == comp::END_ID_MARKER)
    {
        size_t numEntities = 1;
        if (mCurrentFrameState)
        {
            numEntities = mCurrentFrameState->mNumEntities;
            mMessaging.SetPayload(mCurrentFrameState);
        }

        mCurrentFrameState = mMessaging.CreatePayload(numEntities * entitySize);
    }
    else if (mCurrentFrameState)
    {
        mCurrentFrameState->mNumEntities++;
        mCurrentFrameState->AssureWriteSize(entitySize);
        const auto location = locationComponent->Matrix();

        YAGET_ASSERT(comp::IsIdPersistent(id), "We only support Persistent id's!!!");
        render::EntityState entityState{ comp::StripQualifiers(id) };
        math3d::GetMatrixAsFloats(location, entityState.mMatrix);

        if (!materialComponent->mAssetTag.IsValid())
        {
            materialComponent->mAssetTag = mApp.VTS().GetTag(materialComponent->GetValue<comp::db_material::Section>());
        }

        memcpy(entityState.mAssetGuid, materialComponent->mAssetTag.mGuid.bytes().data(), 16);
        mCurrentFrameState->WriteDataChunk(&entityState, sizeof(entityState));
    }
}
