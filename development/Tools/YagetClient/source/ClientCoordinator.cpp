#include "ClientCoordinator.h"
#include <boost/asio.hpp>

//-------------------------------------------------------------------------------------------------
yaget::client::ClientSystemsCoordinator::ClientSystemsCoordinator(Messaging& m, Application& app)
    : internal::SystemsCoordinatorE(m, app)
{
}
