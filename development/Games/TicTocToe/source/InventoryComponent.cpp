#include "InventoryComponent.h"

ttt::InventoryComponent::InventoryComponent(yaget::comp::Id_t id, int numPieces)
    : BaseComponent(id)
    , mNumPieces(numPieces)
{
}
