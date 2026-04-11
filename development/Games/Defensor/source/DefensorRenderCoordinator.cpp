#include "DefensorRenderCoordinator.h"
#include "Render/AdapterInfo.h"
#include "Debugging/Assert.h"


//-------------------------------------------------------------------------------------------------
defensor::render::DefensorSystemsCoordinator::DefensorSystemsCoordinator(Messaging& m, yaget::Application& app)
    : SystemsCoordinator(m, app)
    , mApplication(static_cast<yaget::render::DesktopApplication&>(app))
{
    AddComponent<SceneComponent>(comp::GLOBAL_ID_MARKER);
}

defensor::render::DefensorSystemsCoordinator::~DefensorSystemsCoordinator()
{
    mApplication.Device().Shutdown();
}
