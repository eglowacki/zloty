#include "Systems/FrameStateCollectorSystem.h"
#include "DefensorRenderTypes.h"


//-------------------------------------------------------------------------------------------------
defensor::game::FrameStateCollectorSystem::FrameStateCollectorSystem(Messaging& messaging, Application& app, GameCoordinatorSet& coordinatorSet)
    : GameSystem("FrameStateCollectorSystem", messaging, app, [this](auto&&... params) {OnUpdate(params...); }, coordinatorSet)
    , mCurrentFrameState(messaging.CreatePayload(/*sizeof(EntityState)*/))
{
}


//-------------------------------------------------------------------------------------------------
void defensor::game::FrameStateCollectorSystem::OnUpdate(yaget::comp::Id_t id, const yaget::time::GameClock& gameClock, yaget::metrics::Channel& channel, comp::LocationComponent3* locationComponent)
{
    gameClock; channel; locationComponent;
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

        render::EntityState entityState{ comp::StripQualifiers(id) };

        const auto kMatrixSize = 4;
        for (auto r = 0; r < kMatrixSize; ++r)
        {
            for (auto c = 0; c < kMatrixSize; ++c)
            {
                entityState.mMatrix[r * kMatrixSize + c] = location(r, c);
            }
        }

        mCurrentFrameState->WriteDataChunk(&entityState, sizeof(entityState));
    }
}

