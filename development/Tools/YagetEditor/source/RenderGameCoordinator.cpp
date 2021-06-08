#include "RenderGameCoordinator.h"
#include "Render/DesktopApplication.h"

yaget::editor::RenderSystemsCoordinator::RenderSystemsCoordinator(Messaging& m, render::DesktopApplication& app)
    : internal::SystemsCoordinatorR(m, app)
{
    using IdType = yaget::IdGameCache::IdType;

    auto& coordinator = GetCoordinator<RenderEntity>();
    auto& idCache = app.IdCache;

    coordinator.AddComponent<RenderComponent>(idCache.GetId(IdType::Burnable));
}

