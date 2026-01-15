#include "PlayerSystem.h"


//-------------------------------------------------------------------------------------------------
defensor::game::PlayerSystem::PlayerSystem(Messaging& messaging, Application& app, GameCoordinatorSet& coordinatorSet)
    : GameSystem("PlayerSystem", messaging, app, [this](auto&&... params) {OnUpdate(params...); }, coordinatorSet)
{
}


//-------------------------------------------------------------------------------------------------
void defensor::game::PlayerSystem::OnUpdate(yaget::comp::Id_t id, const yaget::time::GameClock& gameClock, yaget::metrics::Channel& channel, yaget::comp::LocationComponent3* locationComponent, const comp::InputComponent* inputComponent)
{
    id; gameClock; channel; locationComponent; inputComponent;

    // NOTE: do we need this part in script?
    // Move have some kind of speed, ? based on entity type, modifiers, etc
    // How does this number gets set?
    // based on "set" speed, calculate distance based on delta timre
    // float speed 0.2f; // units per second
    // float moveBy = gameClock.GetDeltaTimeSecond() * speed;
    
    const bool isActionLeft = inputComponent->IsAction("Move Left");
    const bool isActionRight = inputComponent->IsAction("Move Right");

    if (isActionLeft || isActionRight)
    {
        float speed = 0.2f; // units per second. This should come from unit/player speed component
        auto position = locationComponent->GetValue<comp::db_location::Position>();
        float moveBy = gameClock.GetDeltaTimeSecond() * speed;

        if (isActionLeft)
        {
            position.x -= moveBy;
        }
        if (isActionRight)
        {
            position.x += moveBy;
        }

        position.x = std::clamp(position.x, 0.0f, 1.0f);
        locationComponent->SetValue<comp::db_location::Position>(position);
    }

    YLOG_DEBUG("GSYS", "============ Player Position: '%f'", locationComponent->GetValue<comp::db_location::Position>().x);
}
