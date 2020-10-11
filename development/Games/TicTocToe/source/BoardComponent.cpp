#include "BoardComponent.h"


ttt::BoardComponent::BoardComponent(yaget::comp::Id_t id, int rows, int columns)
    : BaseComponent(id)
    , mNumRows(rows)
    , mNumColumns(columns)
{
}