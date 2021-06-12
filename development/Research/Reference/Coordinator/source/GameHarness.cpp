#include "GameHarness.h"
#include "App/Application.h"
#include "Components/GameCoordinator.h"
#include "Device.h"
#include "RenderTarget.h"
#include "Gui/Layout.h"     // for gui::*
#include "Gui/Support.h"    // for Fonter
#include "imgui.h"          // for ImGui::*

#include <functional>


namespace
{
    void RenderLogPanel()
    {
        yaget::gui::SnapTo(yaget::gui::Border::Bottom, "Log Output");
        yaget::gui::DrawLogs("Log Output");
    }

    void RenderDemoPanel()
    {
        static bool demoWindow = false;
        static bool metricsWindow = false;
        static bool styleEditor = false;
        static bool styleSelector = false;
        static bool fontSelector = false;
        static bool userGuide = false;

        yaget::gui::Fonter fonter("Consola");
        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("Demos"))
            {
                ImGui::MenuItem("Demo Window", "", &demoWindow);
                ImGui::MenuItem("Metrics Window", "", &metricsWindow);
                ImGui::MenuItem("Style Editor", "", &styleEditor);
                ImGui::MenuItem("Style Selector", "", &styleSelector);
                ImGui::MenuItem("Font Selector", "", &fontSelector);
                ImGui::MenuItem("User Guide", "", &userGuide);

                ImGui::EndMenu();
            }

            ImGui::EndMainMenuBar();
        }

        if (demoWindow)
        {
            ImGui::ShowDemoWindow(&demoWindow);
        }
        if (metricsWindow)
        {
            ImGui::ShowMetricsWindow(&metricsWindow);
        }
        if (styleEditor)
        {
            ImGui::ShowStyleEditor();
        }
        if (styleSelector)
        {
            ImGui::ShowStyleSelector("Style Selector");
        }
        if (fontSelector)
        {
            ImGui::ShowFontSelector("Font Selector");
        }
        if (userGuide)
        {
            ImGui::ShowUserGuide();
        }
    }

} // namespace



yaget::GameHarness::GameHarness(IGameCoordinator& gameCoordinator, Application& app, render::Device& device)
    : mGameCoordinator(gameCoordinator)
    , mApplication(app)
    , mDevice(device)
    , mUpdateFrame([this](auto&&... params) { OnInitialize(params...); })
{
}

void yaget::GameHarness::OnInitialize(Application& /*app*/, const time::GameClock& gameClock, metrics::Channel& channel)
{
    mGameCoordinator.GameInitialize(gameClock, channel);
    mUpdateFrame = [this](auto&&... params) { OnUpdate(params...); };
}

void yaget::GameHarness::OnUpdate(Application& app, const time::GameClock& gameClock, metrics::Channel& channel)
{
    mGameCoordinator.GameUpdate(gameClock, channel);
    onGameLoop(app, gameClock, channel);
}

void yaget::GameHarness::OnShutdown(Application& /*app*/, const time::GameClock& gameClock, metrics::Channel& channel)
{
    mGameCoordinator.GameShutdown(gameClock, channel);
}

void yaget::GameHarness::RenderLoopTick(Application& app, const time::GameClock& gameClock, metrics::Channel& channel)
{
    onRenderLoop(app, gameClock, channel);

    if (render::RenderTarget* renderTarget = mDevice.FindRenderTarget("BackBuffer"))
    {
        if (dev::CurrentConfiguration().mDebug.mFlags.Gui)
        {
            renderTarget->AddProcessor(render::RenderTarget::ProcessorType::PostFrame, RenderDemoPanel);
            renderTarget->AddProcessor(render::RenderTarget::ProcessorType::PostFrame, RenderLogPanel);
        }
    }

    // we are ready to process stager from our logic/systems
    mDevice.RenderFrame(gameClock, channel, nullptr, [this](const time::GameClock& gameClock, metrics::Channel& channel)
    {
        mGameCoordinator.RenderUpdate(gameClock, channel);
    });
}

void yaget::GameHarness::IdleTick()
{
    onIdle();
}

void yaget::GameHarness::QuitTick()
{
    onQuit();
}

int yaget::GameHarness::Run()
{
    auto gameLoop = [this](auto&&... params) { mUpdateFrame(params...); };
    auto renderLoop = [this](auto&&... params) { RenderLoopTick(params...); };
    auto idleLoop = [this] { IdleTick(); };
    auto quitLoop = [this] { QuitTick(); };
    auto shutdownLogicLoop = [this](auto&&... params) { OnShutdown(params...); };

    return mApplication.Run(gameLoop, renderLoop, idleLoop, shutdownLogicLoop, quitLoop);
}
