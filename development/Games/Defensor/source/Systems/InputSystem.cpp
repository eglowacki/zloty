#include "Systems/InputSystem.h"


//-------------------------------------------------------------------------------------------------
defensor::game::ProcessInputSystem::ProcessInputSystem(Messaging& messaging, Application& app, GameCoordinatorSet& coordinatorSet)
    : GameSystem("SquadronSystem", messaging, app, [this](auto&&... params) {OnUpdate(params...); }, coordinatorSet)
{
        //using ActionCallback_t = std::function<void(const std::string& /*actionName*/, uint64_t /*timeStamp*/, int32_t /*mouseX*/, int32_t /*mouseY*/, uint32_t /*flags*/)>;
    app.Input().RegisterActionCallback("*", [this](const std::string& actionName, uint64_t timeStamp, int32_t mouseX, int32_t mouseY, uint32_t flags)
    {
        SaveAction(actionName, timeStamp, mouseX, mouseY, flags);
    });
}


//-------------------------------------------------------------------------------------------------
void defensor::game::ProcessInputSystem::OnUpdate(yaget::comp::Id_t id, const yaget::time::GameClock& gameClock, yaget::metrics::Channel& channel, yaget::comp::InputComponent* inputComponent)
{
    id; gameClock; channel; inputComponent;
}


void defensor::game::ProcessInputSystem::SaveAction(const std::string& actionName, uint64_t timeStamp, int32_t mouseX, int32_t mouseY, uint32_t flags)
{
    actionName; timeStamp;mouseX;mouseY;flags;
}


//-------------------------------------------------------------------------------------------------
defensor::game::ClearInputSystem::ClearInputSystem(Messaging& messaging, Application& app, GameCoordinatorSet& coordinatorSet)
    : GameSystem("SquadronSystem", messaging, app, [this](auto&&... params) {OnUpdate(params...); }, coordinatorSet)
{
}


//-------------------------------------------------------------------------------------------------
void defensor::game::ClearInputSystem::OnUpdate(yaget::comp::Id_t id, const yaget::time::GameClock& gameClock, yaget::metrics::Channel& channel, yaget::comp::InputComponent* inputComponent)
{
    id; gameClock; channel; inputComponent;
}
