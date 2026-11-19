#include "DefensorRenderCoordinator.h"
#include "Script/MathBinder.h"


//-------------------------------------------------------------------------------------------------
defensor::render::DefensorSystemsCoordinator::DefensorSystemsCoordinator(Messaging& m, yaget::Application& app)
    : SystemsCoordinator(m, app)
    , mApplication(static_cast<yaget::render::DesktopApplication&>(app))
{
    script::BindMath(mScriptState, "yaget");
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
