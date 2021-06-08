#include "EditorSystem.h"


yaget::editor::EditorSystem::EditorSystem(Messaging& messaging, Application& app)
    : GameSystem("EditorSystem", messaging, app, [this](auto&&... params) {OnUpdate(params...); })
{
}

void yaget::editor::EditorSystem::OnUpdate(comp::Id_t id, const time::GameClock& gameClock, metrics::Channel& channel, EditorComponent* editorComponent, EmptyComponent* emptyComponent)
{
    id;
    gameClock;
    channel;
    editorComponent;
    emptyComponent;
}
