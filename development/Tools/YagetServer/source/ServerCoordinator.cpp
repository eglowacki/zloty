#include "ServerCoordinator.h"
#include <boost/asio.hpp>

//-------------------------------------------------------------------------------------------------
yaget::server::ServerSystemsCoordinator::ServerSystemsCoordinator(Messaging& m, Application& app)
    : internal::SystemsCoordinatorE(m, app)
{
}
