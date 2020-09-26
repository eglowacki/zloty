//////////////////////////////////////////////////////////////////////
// VideoOptions.h
//
//  Copyright 8/5/2018 Edgar Glowacki
//
//  Maintained by: Edgar
//
//  NOTES:
//      
//
//
//  #include "Gui/VideoOptions.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "YagetCore.h"
#include "Gui/Controls.h"
#include "Device.h"
#include "App/Application.h"
#include <functional>


namespace yaget
{
    namespace gui
    {
        class VideoOptions
        {
        public:
            VideoOptions(render::Device& device);

            void OnGuiPass();

        private:
            void RefreshSettings();
            void onOptions();
            void onEntrySelection(const std::string& newEntry, const std::string& oldEntry, const std::string& controlName);
            void onDialogButton(const std::string& name);

            render::Device& mDevice;
            bool mFrameActive = false;

            //--------------------------------------------------------------------------------------------------
            struct MonitorSelector
            {
                MonitorSelector(ComboBox::OnEntrySelection onEntrySelection, const std::string& label) : mComboBox(onEntrySelection, label) {}

                ComboBox mComboBox;
                std::vector<render::Device::Resolutions> mResolutions;
                std::string mLastSelection;
            };
            MonitorSelector mMonitorSelector;

            //--------------------------------------------------------------------------------------------------
            struct ComboBoxSelector
            {
                ComboBoxSelector(const std::string& label)
                    : mComboBox([this](const std::string& newEntry, const std::string& /*oldEntry*/) { mLastSelection = newEntry; }, label)
                {}

                ComboBox mComboBox;
                std::string mLastSelection;
            };

            ComboBoxSelector mDisplayModeSelector;
            ComboBoxSelector mResolutionSelector;

            //--------------------------------------------------------------------------------------------------
            struct VSyncSelector
            {
                VSyncSelector(const std::string& label) 
                    : mCheckBox([this](auto&&... params) { onStateChange(params...); }, label)
                {}

                void onStateChange(bool selected) { mLastSelection = selected; }

                CheckBox mCheckBox;
                bool mLastSelection = false;
            };
            VSyncSelector mVSyncSelector;

            DialogButtons mDialogButtons;

            Application::VideoOptions mVideoOptions = {};
            render::Device::Resolutions mStartingResolution;
        };

    } // namespace gui
} // namespace yaget
