#include "DefensorRenderCoordinator.h"
#include "Render/AdapterInfo.h"
#include "Debugging/Assert.h"


//-------------------------------------------------------------------------------------------------
defensor::render::DefensorSystemsCoordinator::DefensorSystemsCoordinator(Messaging& m, yaget::Application& app)
    : SystemsCoordinator(m, app)
    , mApplication(static_cast<yaget::render::DesktopApplication&>(app))
{
}


//-------------------------------------------------------------------------------------------------
defensor::render::DefensorSystemsCoordinator::~DefensorSystemsCoordinator()
{
    mApplication.Device().Shutdown();
}


//-------------------------------------------------------------------------------------------------
void defensor::render::DefensorSystemsCoordinator::Tick(const time::GameClock& gameClock, metrics::Channel& channel)
{
    mMessaging.Process(Messaging::DispatcherType::Render);

    SystemsCoordinator::Tick(gameClock, channel);
}
