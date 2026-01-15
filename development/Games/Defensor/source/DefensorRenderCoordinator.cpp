#include "DefensorRenderCoordinator.h"


//-------------------------------------------------------------------------------------------------
defensor::render::DefensorSystemsCoordinator::DefensorSystemsCoordinator(Messaging& m, yaget::Application& app)
    : SystemsCoordinator(m, app)
    , mApplication(static_cast<yaget::render::DesktopApplication&>(app))
{
    constexpr auto sceneId = comp::GLOBAL_ID_MARKER;
    AddComponent<SceneComponent>(sceneId);
}

defensor::render::DefensorSystemsCoordinator::~DefensorSystemsCoordinator()
{
    int z = 0;
    z;
    mApplication.Device().Shutdown();
}
