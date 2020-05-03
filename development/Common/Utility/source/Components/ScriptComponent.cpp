#include "Components/ScriptComponent.h"


yaget::comp::ScriptComponent::ScriptComponent(Id_t id, ScriptFunction scriptFunction)
    : Component(id)
    , mScriptFunction(scriptFunction)
{
}

yaget::comp::ScriptComponent::~ScriptComponent()
{
}

size_t yaget::comp::ScriptComponent::CalculateStateHash() const
{
    return 1;
}

void yaget::comp::ScriptComponent::Update(const time::GameClock& gameClock, metrics::Channel& channel)
{
    mScriptFunction(Id(), gameClock, channel);
}
