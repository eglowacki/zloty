#include "EditorSystem.h"


yaget::editor::EditorSystem::EditorSystem(Messaging& messaging, Application& app)
    : GameSystem("EditorSystem", messaging, app, [this](auto&&... params) {OnUpdate(params...); })
{
}

void yaget::editor::EditorSystem::OnUpdate([[maybe_unused]] comp::Id_t id, [[maybe_unused]] const time::GameClock& gameClock, [[maybe_unused]] metrics::Channel& channel, [[maybe_unused]] EditorComponent* editorComponent, [[maybe_unused]] EmptyComponent* emptyComponent, [[maybe_unused]] const BlankComponent* blankComponent)
{
    int z = 0;
    z;
}
