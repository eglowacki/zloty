#include "EditorSystem.h"


yaget::editor::EditorSystem::EditorSystem(Messaging& messaging, Application& app, EditorGameCoordinatorSet& coordinatorSet)
    : GameSystem("EditorSystem", messaging, app, [this](auto&&... params) {OnUpdate(params...); }, coordinatorSet)
{
}

void yaget::editor::EditorSystem::OnUpdate([[maybe_unused]] comp::Id_t id, [[maybe_unused]] const time::GameClock& gameClock, [[maybe_unused]] metrics::Channel& channel, [[maybe_unused]] EmptyComponent* emptyComponent, [[maybe_unused]] const BlankComponent* blankComponent/*, [[maybe_unused]] EditorComponent* editorComponent/**/)
{
    int z = 0;
    z;
}
