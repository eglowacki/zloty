#include "SessionSystem.h"


//---------------------------------------------------------------------------------------------------------------------
yaget::server::SessionSystem::SessionSystem(Messaging& messaging, Application& app, EntityCoordinatorSet& coordinatorSet)
    : GameSystem("SessionSystem", messaging, app, [this](auto&&... params) {OnUpdate(params...); }, coordinatorSet)
{
}


//---------------------------------------------------------------------------------------------------------------------
yaget::server::SessionSystem::~SessionSystem()
{
    auto& coordinator = GetCoordinator<Entity>();
    coordinator.RemoveItems(mItems);
}

//---------------------------------------------------------------------------------------------------------------------
void yaget::server::SessionSystem::OnUpdate(comp::Id_t id, [[maybe_unused]] const time::GameClock& gameClock, [[maybe_unused]] metrics::Channel& channel, [[maybe_unused]] const SessionComponent* sessionComponent)
{
    if (id == comp::END_ID_MARKER)
    {
    }
    else
    {
        // operate on serverComponent...
    }
}
