#include "Components/MenuComponent.h"


void yaget::comp::MenuComponent::OnInput(const std::string& eventName)
{
    mInputQueue[eventName] = true;
}
