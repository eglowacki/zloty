#include "SessionComponent.h"

//---------------------------------------------------------------------------------------------------------------------
yaget::server::SessionComponent::SessionComponent(comp::Id_t id, boost::asio::io_context& ioContext)
    : comp::BaseComponent<>(id)
    , mIoContext(&ioContext)
{
}
