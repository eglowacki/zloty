#include "MenuSystem.h"


defensor::game::MenuSystem::MenuSystem(Messaging& messaging, Application& app, GameCoordinatorSet& coordinatorSet)
    : GameSystem("MenuSystem", messaging, app, [this](auto&&... params) {OnUpdate(params...); }, coordinatorSet)
{
}

void defensor::game::MenuSystem::OnUpdate(yaget::comp::Id_t id, const yaget::time::GameClock& gameClock, yaget::metrics::Channel& channel, yaget::comp::MenuComponent* menuComponent, comp::InputComponent* inputComponent, comp::ScriptComponent* scriptComponent)
{
    id; gameClock; channel;
    for (auto& [name, triggered] : menuComponent->mInputQueue)
    {
        if (triggered)
        {
            YLOG_DEBUG("DEF", "------------------------------------ Action: '%s' triggered.", name.c_str());

            scriptComponent->Execute();
            triggered = false;
        }
    }
}

