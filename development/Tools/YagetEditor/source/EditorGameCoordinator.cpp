#include "EditorGameCoordinator.h"

//-------------------------------------------------------------------------------------------------
yaget::editor::EditorSystemsCoordinator::EditorSystemsCoordinator(Messaging& m, Application& app)
    : internal::SystemsCoordinatorE(m, app)
{
    using IdType = IdGameCache::IdType;

    auto& coordinator = GetCoordinator<Entity>();
    auto& globalCoordinator = GetCoordinator<GlobalEntity>();
    auto& idCache = app.IdCache;

    const auto id = idCache.GetId(IdType::Burnable);
    coordinator.AddComponent<EmptyComponent>(id);
    mItems.insert(id);

    globalCoordinator.AddComponent<EditorComponent>(id);
    mGlobalItems.insert(id);
}


//-------------------------------------------------------------------------------------------------
yaget::editor::EditorSystemsCoordinator::~EditorSystemsCoordinator()
{
    auto& coordinator = GetCoordinator<Entity>();
    coordinator.RemoveItems(mItems);

    auto& globalCoordinator = GetCoordinator<GlobalEntity>();
    globalCoordinator.RemoveItems(mGlobalItems);
}