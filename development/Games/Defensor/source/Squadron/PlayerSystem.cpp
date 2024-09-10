#include "PlayerSystem.h"


defensor::game::PlayerSystem::PlayerSystem(Messaging& messaging, Application& app, GameCoordinatorSet& coordinatorSet)
    : GameSystem("PlayerSystem", messaging, app, [this](auto&&... params) {OnUpdate(params...); }, coordinatorSet)
{
}

void defensor::game::PlayerSystem::OnUpdate(yaget::comp::Id_t id, const yaget::time::GameClock& gameClock, yaget::metrics::Channel& channel, yaget::comp::LocationComponent3* locationComponent, comp::InputComponent* inputComponent)
{
    id; gameClock; channel; locationComponent; inputComponent;
}
