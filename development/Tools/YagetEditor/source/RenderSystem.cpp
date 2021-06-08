#include "RenderSystem.h"
#include "Render/DesktopApplication.h"

yaget::editor::RenderSystem::RenderSystem(Messaging& messaging, render::DesktopApplication& app)
    : GameSystem("EditorSystem", messaging, app, [this](auto&&... params) {OnUpdate(params...); })
    , mDevice(app.GetDevice())
{
}

void yaget::editor::RenderSystem::OnUpdate(comp::Id_t id, const time::GameClock& gameClock, metrics::Channel& channel, RenderComponent* renderComponent)
{
    mDevice.RenderFrame(gameClock, channel);
    id;
    renderComponent;
}
