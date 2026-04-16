#include "PlayerSystem.h"
#include "Components/BulletComponent.h"


//-------------------------------------------------------------------------------------------------
defensor::game::PlayerSystem::PlayerSystem(Messaging& messaging, Application& app, GameCoordinatorSet& coordinatorSet)
    : GameSystem("PlayerSystem", messaging, app, [this](auto&&... params) {OnUpdate(params...); }, coordinatorSet, true)
{
}


//-------------------------------------------------------------------------------------------------
void defensor::game::PlayerSystem::OnUpdate(yaget::comp::Id_t id, const yaget::time::GameClock& gameClock, yaget::metrics::Channel& channel, yaget::comp::LocationComponent3* locationComponent, const comp::InputComponent* inputComponent)
{
    id; channel;

    auto strippedId = comp::StripQualifiers(id);
    strippedId;

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
        float speed = 0.4f; // units per second. This should come from unit/player speed component
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

    const bool isActionShoot = inputComponent->IsAction("Shoot");
    if (isActionShoot)
    {
        const auto& inputData = inputComponent->mTriggeredAction.at("Shoot");
        if (!mBulletActive && (inputData.mFlags & input::kButtonDown))
        {
            mBulletActive = true;
            YLOG_INFO("REND", "============ Player Shoots");

            // create the bullet now...
            auto bulletId = idspace::get_persistent(mApp.IdCache);
            auto playerPosition = locationComponent->GetValue<comp::db_location::Position>();

            AddPlayerBullet(bulletId, "PlayerShipBullet", Section{ "SceneItems@PlayerShipBullet" }, playerPosition + math3d::Vector3{ 0, 0.1f, 0 });
        }
        else if (mBulletActive && (inputData.mFlags & input::kButtonUp))
        {
            mBulletActive = false;
        }
    }
}

//-------------------------------------------------------------------------------------------------
void defensor::game::PlayerSystem::AddPlayerBullet(comp::Id_t id, const std::string& itemName, const Section& materialSection, math3d::Vector3 bulletPosition)
{
    auto& coordinatorSet = GetCS();

    coordinatorSet.AddComponent<comp::NameComponent>(id, itemName);
    coordinatorSet.AddComponent<comp::BulletComponent>(id);
    coordinatorSet.AddComponent<comp::MaterialComponent>(id, materialSection);
    coordinatorSet.AddComponent<comp::LocationComponent3>(id, bulletPosition);
}
