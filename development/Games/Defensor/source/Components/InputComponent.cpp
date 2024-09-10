#include "Components/InputComponent.h"


void yaget::comp::InputComponent::AddInputEvent(const std::string& eventName, input::ActionNonParamCallback_t callback) const
{
    mInputDevice->RegisterSimpleActionCallback(eventName, std::move(callback));
}


void yaget::comp::InputComponent::AddInputEvent(const std::string& eventName, input::ActionCallback_t callback) const
{
    mInputDevice->RegisterActionCallback(eventName, std::move(callback));
}
