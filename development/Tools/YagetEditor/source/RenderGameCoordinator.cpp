#include "RenderGameCoordinator.h"
#include "Render/DesktopApplication.h"

//-------------------------------------------------------------------------------------------------
yaget::editor::RenderSystemsCoordinator::RenderSystemsCoordinator(Messaging& m, render::DesktopApplication& app)
    : internal::SystemsCoordinatorR(m, app)
{
    using IdType = yaget::IdGameCache::IdType;

    auto& coordinator = GetCoordinator<RenderEntity>();
    auto& idCache = app.IdCache;

    const auto itemId = idCache.GetId(IdType::Burnable);
    coordinator.AddComponent<RenderComponent>(itemId);
    mRenderItems.insert(itemId);
}


//-------------------------------------------------------------------------------------------------
yaget::editor::RenderSystemsCoordinator::~RenderSystemsCoordinator()
{
    auto& coordinator = GetCoordinator<RenderEntity>();
    coordinator.RemoveItems(mRenderItems);
}

