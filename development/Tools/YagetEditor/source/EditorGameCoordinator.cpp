#include "EditorGameCoordinator.h"

yaget::editor::EditorSystemsCoordinator::EditorSystemsCoordinator(Messaging& m, Application& app)
    : internal::SystemsCoordinatorE(m, app)
{
    using IdType = IdGameCache::IdType;

    auto& coordinator = GetCoordinator<Entity>();
    auto& globalCoordinator = GetCoordinator<GlobalEntity>();
    auto& idCache = app.IdCache;

    const auto id = idCache.GetId(IdType::Burnable);
    coordinator.AddComponent<EmptyComponent>(id);
    globalCoordinator.AddComponent<EditorComponent>(id);
}

