#include "Gui/VideoOptions.h"
#include "Gui/Support.h"
#include "App/Application.h"
#include "Input/InputDevice.h"
#include "imgui.h"
#include "Logger/YLog.h"

namespace
{
    //--------------------------------------------------------------------------------------------------
    std::string ConvertMonitorString(const yaget::render::Device::Resolutions& monitor)
    {
        return fmt::format("{} - {}x{}", monitor.MonitorName, monitor.Width, monitor.Height);
    }

    //--------------------------------------------------------------------------------------------------
    yaget::Strings ConvertMonitorSelector(const std::vector<yaget::render::Device::Resolutions>& monitors)
    {
        using Resolutions = yaget::render::Device::Resolutions;

        yaget::Strings monitorEntries;
        std::transform(monitors.begin(), monitors.end(), std::back_inserter(monitorEntries), [](const Resolutions& entry)-> std::string
        {
            return ConvertMonitorString(entry);
        });

        return monitorEntries;
    }

    //--------------------------------------------------------------------------------------------------
    std::string ConvertResolutionString(const yaget::render::Device::Resolutions::Description& mode)
    {
        return fmt::format("{} by {} - {} Hz", mode.Width, mode.Height, mode.RefreshRate);
    }

    //--------------------------------------------------------------------------------------------------
    yaget::Strings ConvertMonitorResolution(const std::vector<yaget::render::Device::Resolutions::Description>& modes)
    {
        using Description = yaget::render::Device::Resolutions::Description;

        yaget::Strings resolutionEntries;
        std::transform(modes.begin(), modes.end(), std::back_inserter(resolutionEntries), [](const Description& entry)-> std::string
        {
            return ConvertResolutionString(entry);
        });

        return resolutionEntries;
    }

    const char* kModeFullScreen = "Full Screen";
    const char* kModeBorderlessWindow = "Borderless Window";
    const char* kModeWindow = "Window";

} // namespace


yaget::gui::VideoOptions::VideoOptions(render::Device& device)
    : mDevice(device)
    , mMonitorSelector([this](auto&& param1, auto&& param2) { onEntrySelection(param1, param2, "MonitorSelector"); }, "Monitor")
    , mResolutionSelector("Resolution")
    , mDisplayModeSelector("Display Mode")
    , mVSyncSelector("VSync")
    , mDialogButtons([this](auto&& param1) { onDialogButton(param1); }, "Apply", "Cancel")
{
    mDevice.App().Input().RegisterSimpleActionCallback("Options", [this]() { onOptions(); });
}

void yaget::gui::VideoOptions::onOptions()
{
    mFrameActive = !mFrameActive;
    if (mFrameActive)
    {
        RefreshSettings();
    }
}

void yaget::gui::VideoOptions::RefreshSettings()
{
    mMonitorSelector.mResolutions = mDevice.GetResolutions(&mStartingResolution);

    std::string currentMonitorEntry = ConvertMonitorString(mStartingResolution);
    Strings monitorEntries = ConvertMonitorSelector(mMonitorSelector.mResolutions);
    mMonitorSelector.mComboBox.SetEntries(monitorEntries, currentMonitorEntry);

    Strings displayModes = { kModeFullScreen, kModeBorderlessWindow, kModeWindow };
    mDisplayModeSelector.mComboBox.SetEntries(displayModes, kModeWindow);
}

void yaget::gui::VideoOptions::OnGuiPass()
{
    if (mFrameActive)
    {
        gui::layout::CenterFrame(800, 600);
        if (ImGui::Begin("", &mFrameActive, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse))// | ImGuiWindowFlags_NoTitleBar))
        {
            // Header Title for this option section
            {
                gui::Fonter fonter("Title");
                layout::CenterText("Graphics Options", 70 / 255.0f, 220 / 255.0f, 115 / 255.0f, 0.25f);
                ImGui::Separator();
                ImGui::Dummy(ImVec2(0, 5));
            }

            if (mMonitorSelector.mResolutions.size() > 1)
            {
                // show monitor selection combo box (monitor 1, Monitor 2)
                bool disableMonitor = mDisplayModeSelector.mLastSelection != kModeWindow;
                mMonitorSelector.mComboBox.OnGuiPass(disableMonitor);
                ImGui::Dummy(ImVec2(0, 10));
            }

            // show mode selection combo box (Full Screen, Borderless Window, Window)
            mDisplayModeSelector.mComboBox.OnGuiPass(true);
            ImGui::Dummy(ImVec2(0, 30));

            // show resolution selection, which is adjusted per monitor, but disable it when user selects borderless window
            // since then it defaults to desktop resolution
            bool disableResolutions = mDisplayModeSelector.mLastSelection != kModeBorderlessWindow;
            mResolutionSelector.mComboBox.OnGuiPass(disableResolutions);
            ImGui::Dummy(ImVec2(0, 10));

            // do we sync it or not
            mVSyncSelector.mCheckBox.OnGuiPass(true);
            ImGui::Dummy(ImVec2(0, 5));

            mDialogButtons.OnGuiPass(true);
        }

        ImGui::End();
    }
}

void yaget::gui::VideoOptions::onEntrySelection(const std::string& newEntry, const std::string& /*oldEntry*/, const std::string& controlName)
{
    if (controlName == "MonitorSelector")
    {
        using Resolutions = render::Device::Resolutions;

        auto it = std::find_if(mMonitorSelector.mResolutions.begin(), mMonitorSelector.mResolutions.end(), [&newEntry](const Resolutions& resolution)
        {
            std::size_t found = newEntry.find(resolution.MonitorName);
            return found != std::string::npos;
        });

        std::string currenModeEntry = ConvertResolutionString(*mStartingResolution.Modes.begin());
        Strings resolutionEntries = ConvertMonitorResolution(it->Modes);
        Resolutions currentResolution = mDevice.GetCurrentResolution();
        mResolutionSelector.mComboBox.SetEntries(resolutionEntries, currenModeEntry);
    }
}

void yaget::gui::VideoOptions::onDialogButton(const std::string& name)
{
    if (name == "Apply")
    {
        // grab all settings,
        uint32_t monitorId = InvalidId;     // Monitor 1, Monitor 2
        uint32_t diplayMode = InvalidId;    // kModeFullScreen, kModeBorderlessWindow, kModeWindow 
        uint32_t resX = InvalidId;          // 1920;
        uint32_t resY = InvalidId;          // 1080;
        bool sync = false;

        mVideoOptions = { monitorId, diplayMode, resX, resY, sync, false };
        mDevice.App().ChangeVideoSettings(mVideoOptions);
        // switch to state where we show timer/Cancel or OK dialog to confirm that mode change worked
    }
    else if (name == "Cancel")
    {
        // reapply mStartingVideoOptions to all gui controls, so next time when dialog is open, user will see not change values
        mFrameActive = false;
    }
}
