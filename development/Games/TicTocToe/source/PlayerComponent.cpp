#include "PlayerComponent.h"

ttt::PlayerComponent::PlayerComponent(yaget::comp::Id_t id, PieceType sideControl)
    : BaseComponent(id)
    , mSideControl(sideControl)
{

}
