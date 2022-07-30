#include "RenderSystem.h"
#include "Render/DesktopApplication.h"

yaget::editor::RenderSystem::RenderSystem(Messaging& messaging, render::DesktopApplication& app)
    : GameSystem("EditorSystem", messaging, app, [this](auto&&... params) {OnUpdate(params...); })
    , mDevice(app.GetDevice())
{
}

void yaget::editor::RenderSystem::OnUpdate([[maybe_unused]] comp::Id_t id, const time::GameClock& gameClock, metrics::Channel& channel, [[maybe_unused]] RenderComponent* renderComponent)
{
    if (id == comp::END_ID_MARKER)
    {
        mDevice.RenderFrame(gameClock, channel);
    }
}
