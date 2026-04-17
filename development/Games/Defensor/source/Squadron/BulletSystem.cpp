#include "Squadron/BulletSystem.h"


//-------------------------------------------------------------------------------------------------
defensor::game::BulletSystem::BulletSystem(Messaging& messaging, Application& app, GameCoordinatorSet& coordinatorSet)
    : GameSystem("BulletSystem", messaging, app, [this](auto&&... params) {OnUpdate(params...); }, coordinatorSet, true)
{
}


//-------------------------------------------------------------------------------------------------
void defensor::game::BulletSystem::OnUpdate(yaget::comp::Id_t id, const yaget::time::GameClock& gameClock, yaget::metrics::Channel& /*channel*/, comp::LocationComponent3* locationComponent, comp::MaterialComponent* /*materialComponent*/, comp::BulletComponent* /*bulletComponent*/)
{
    //id; gameClock; channel;
    if (id == comp::END_ID_MARKER)
    {
        int z = 0;
        z;
    }
    else
    {
        float speed = 0.8f; // units per second. This should come from unit/player speed component
        auto bulletPosition = locationComponent->GetValue<comp::db_location::Position>();
        auto position = locationComponent->GetValue<comp::db_location::Position>();
        float moveBy = gameClock.GetDeltaTimeSecond() * speed;

        bulletPosition.y += moveBy;
        bulletPosition.y = std::clamp(bulletPosition.y, 0.0f, 0.8f);
        locationComponent->SetValue<comp::db_location::Position>(bulletPosition);
    }
}

