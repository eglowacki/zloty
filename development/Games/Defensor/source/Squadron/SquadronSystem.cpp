#include "SquadronSystem.h"


defensor::game::SquadronSystem::SquadronSystem(Messaging& messaging, Application& app, GameCoordinatorSet& coordinatorSet)
    : GameSystem("SquadronSystem", messaging, app, [this](auto&&... params) {OnUpdate(params...); }, coordinatorSet)
{
}

void defensor::game::SquadronSystem::OnUpdate(yaget::comp::Id_t id, const yaget::time::GameClock& gameClock, yaget::metrics::Channel& channel, yaget::comp::LocationComponent3* locationComponent)
{
    id; gameClock; channel; locationComponent;
}
