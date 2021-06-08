#include "EditorGameCoordinator.h"

yaget::editor::EditorSystemsCoordinator::EditorSystemsCoordinator(Messaging& m, Application& app)
    : internal::SystemsCoordinatorE(m, app)
{
    using IdType = IdGameCache::IdType;

    auto& coordinator = GetCoordinator<Entity>();
    auto& globalCoordinator = GetCoordinator<GlobalEntity>();
    auto& idCache = app.IdCache;

    coordinator.AddComponent<EmptyComponent>(idCache.GetId(IdType::Burnable));
    globalCoordinator.AddComponent<EditorComponent>(idCache.GetId(IdType::Burnable));
}

