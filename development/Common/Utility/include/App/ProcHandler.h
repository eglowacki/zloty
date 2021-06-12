/////////////////////////////////////////////////////////////////////////
// ProcHandler.h
//
//  Copyright 4/11/2009 Edgar Glowacki.
//
// NOTES:
//      Provides unified way of handling window procedure messages
//      including quitting.
//
//
// #include "App/ProcHandler.h"
//
/////////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "YagetCore.h"
#include "Json/JsonHelpers.h"
#include "Streams/Buffers.h"
#include "Platform/WindowsLean.h"
#include "Debugging/DevConfiguration.h"
#include <functional>


namespace yaget::io { class VirtualTransportSystem; }

namespace yaget::app
{
    enum class Appearance { Fullscreen, Window, Borderless };

    NLOHMANN_JSON_SERIALIZE_ENUM(Appearance, {
        { Appearance::Fullscreen, "Fullscreen" },
        { Appearance::Window, "Window" },
        { Appearance::Borderless, "Borderless" }
        });

    //--------------------------------------------------------------------------------------------------
    // data representation of json file window settings
    struct WindowAppearance
    {
        // serialized to json as a configuration
        Appearance mAppearance = Appearance::Borderless;
        int mResX = 0;
        int mResY = 0;
        // which monitor to display this window, 0 means is not set yet
        int mMonitorIndex = 0;

        int mTop = 0;
        int mLeft = 0;

        // only serialized if it's true
        bool mMaximized = false;

        // not serialized
        int mBottom = 0;
        int mRight = 0;
        uint32_t mWindowStyle = 0;
        uint32_t mWindowExStyle = 0;

        // Helper utility methods
        bool IsResValid() const { return mResX > 0 && mResY > 0; }
        int FrameWidth() const { return mRight - mLeft; }
        int FrameHeight() const { return mBottom - mTop; }
    };

    inline void to_json(nlohmann::json& j, const WindowAppearance& appearance)
    {
        j["Top"] = appearance.mTop;
        j["Left"] = appearance.mLeft;
        j["ResolutionX"] = appearance.mResX;
        j["ResolutionY"] = appearance.mResY;
        j["Appearance"] = appearance.mAppearance;
        j["MonitorIndex"] = appearance.mMonitorIndex;

        if (appearance.mMaximized)
        {
            j["Maximized"] = appearance.mMaximized;
        }
    }

    inline void from_json(const nlohmann::json& j, WindowAppearance& appearance)
    {
        appearance.mTop = json::GetValue(j, "Top", appearance.mTop);
        appearance.mLeft = json::GetValue(j, "Left", appearance.mLeft);
        appearance.mResX = json::GetValue(j, "ResolutionX", appearance.mResX);
        appearance.mResY = json::GetValue(j, "ResolutionY", appearance.mResY);
        appearance.mAppearance = json::GetValue<Appearance>(j, "Appearance", appearance.mAppearance);
        appearance.mMonitorIndex = json::GetValue(j, "MonitorIndex", appearance.mMonitorIndex);
        appearance.mMaximized = json::GetValue(j, "Maximized", appearance.mMaximized);
    }


    //--------------------------------------------------------------------------------------------------
    // Handler for processing, handling and dispatching of window messages
    class ProcHandler
    {
    public:
        using ProcessMessage = std::function<int64_t(uint32_t message, uint64_t wParam, int64_t lParam)>;
        using ProcessInput = std::function<int64_t(uint32_t message, uint64_t wParam, int64_t lParam)>;
        using ProcessResize = std::function<void()>;

        using RequestClose = std::function<bool()>;

        using WindowAppearances = std::map<Appearance, WindowAppearance>;

        ProcHandler(const dev::Configuration::Init& init, const std::string& windowTitle, io::VirtualTransportSystem& vts, ProcessInput processInput, ProcessResize processResize, RequestClose requestClose);

        HWND WinHandle() const { return mWindowHandle; }
        Appearance ActiveAppearance() const { return mActiveAppearance; }
        // Return true if current window is minimized/suspended. 
        bool IsSuspended() const;

    private:
        static LRESULT CALLBACK WindowCallback(HWND hWnd, uint32_t message, uint64_t wParam, int64_t lParam);

        int64_t onInitialize(uint32_t message, uint64_t wParam, int64_t lParam);
        int64_t onMessage(uint32_t message, uint64_t wParam, int64_t lParam);
        void onResize(bool sizeChanged);
        void onSuspend(bool suspended);

        void ToggleBorderless();
        void SaveAppearance() const;

        const dev::Configuration::Init& mInit;
        ProcessMessage mProcessMessage;
        ProcessInput mProcessInput;
        ProcessResize mProcessResize;
        HWND mWindowHandle = nullptr;
        RequestClose mRequestClose;
        io::VirtualTransportSystem& mVTS;
        struct ReflectedState
        {
            bool mMoving = false;
            bool mAllowUpdateWhileMoving = false;
            bool mSuspended = false;
            std::pair<int32_t, int32_t> mLastResolution;
        };
        ReflectedState mReflectedState;

        Appearance mActiveAppearance = Appearance::Window;
        WindowAppearances mWindowAppearances;
    };

}

