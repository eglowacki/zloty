#include "Systems/InputSystem.h"


//-------------------------------------------------------------------------------------------------
defensor::game::ProcessInputSystem::ProcessInputSystem(Messaging& messaging, Application& app, GameCoordinatorSet& coordinatorSet)
    : GameSystem("ProcessInputSystem", messaging, app, [this](auto&&... params) {OnUpdate(params...); }, coordinatorSet, true)
{
    app.Input().RegisterActionCallback("*", [this](const std::string& contextName, const std::string& actionName, uint64_t timeStamp, int32_t mouseX, int32_t mouseY, uint32_t flags)
    {
        if (!mActiveContextName.empty() && contextName == mActiveContextName)
        {
            SaveAction(actionName, timeStamp, mouseX, mouseY, flags);
        }
    });
}


//-------------------------------------------------------------------------------------------------
void defensor::game::ProcessInputSystem::SetContext(const std::string& contextName)
{
    mActiveContextName = contextName;
}


//-------------------------------------------------------------------------------------------------
void defensor::game::ProcessInputSystem::OnUpdate(comp::Id_t id, const time::GameClock& /*gameClock*/, metrics::Channel& /*channel*/, comp::InputComponent* inputComponent)
{
    if (id == comp::END_ID_MARKER)
    {
        mActionInputs.clear();
        return;
    }

    const auto& requestedActions = inputComponent->GetValue<comp::db_input::ActionNames>();
    for (auto element : requestedActions)
    {
        if (auto it = mActionInputs.find(element); it != mActionInputs.end())
        {
            // element (requested action) has a record in mActionInputs.
            // in this case we need store that info on inputComponent,
            // which allows other components/systems to query for "player"
            // actions
            inputComponent->mTriggeredAction[element] = it->second;

            //YLOG_INFO("REND", "============ Input Component action '%s', flags: '%d' added.", element.c_str(), it->second.mFlags);
        }
    }
}


//-------------------------------------------------------------------------------------------------
void defensor::game::ProcessInputSystem::SaveAction(const std::string& actionName, uint64_t timeStamp, int32_t mouseX, int32_t mouseY, uint32_t flags)
{
    mActionInputs[actionName] = {actionName, timeStamp, mouseX, mouseY, flags};
    //YLOG_INFO("REND", "============ Added Action Name: '%s'.", actionName.c_str());
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
defensor::game::ClearInputSystem::ClearInputSystem(Messaging& messaging, Application& app, GameCoordinatorSet& coordinatorSet)
    : GameSystem("ClearInputSystem", messaging, app, [this](auto&&... params) {OnUpdate(params...); }, coordinatorSet, true)
{
}


//-------------------------------------------------------------------------------------------------
void defensor::game::ClearInputSystem::OnUpdate(yaget::comp::Id_t id, const yaget::time::GameClock& gameClock, yaget::metrics::Channel& channel, yaget::comp::InputComponent* inputComponent)
{
    id; gameClock; channel;

    inputComponent->mTriggeredAction.clear();
    //YLOG_INFO("REND", "============ Cleared Actions.");
}
