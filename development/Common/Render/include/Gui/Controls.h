//////////////////////////////////////////////////////////////////////
// Controls.h
//
//  Copyright 7/24/2018 Edgar Glowacki
//
//  Maintained by: Edgar
//
//  NOTES:
//      Various premade controls
//
//
//  #include "Gui/Controls.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "YagetCore.h"
#include "MathFacade.h"


namespace yaget
{
    namespace gui
    {
        bool BufferingBar(const std::string& label, float value, const math3d::Vector2& size_arg, uint32_t bg_col, uint32_t fg_col);

        
        //--------------------------------------------------------------------------------------------------
        class ComboBox
        {
        public:
            using OnEntrySelection = std::function<void (const std::string& newEntry, const std::string& oldEntry)>;

            ComboBox(OnEntrySelection onEntrySelection, const std::string& label);

            void SetEntries(const Strings& entries, const std::string& selected);
            void OnGuiPass(bool active);

        private:
            Strings mEntries;
            std::vector<bool> mSelected;
            std::string mSelectedText;
            OnEntrySelection mOnEntrySelection;
            std::string mLabel;
        };

        
        //--------------------------------------------------------------------------------------------------
        class CheckBox
        {
        public:
            using OnStateChange = std::function<void(bool selected)>;

            CheckBox(OnStateChange onStateChange, const std::string& label);

            void OnGuiPass(bool active);

        private:
            OnStateChange mOnStateChange;
            std::string mLabel;
            bool mSelected = false;
        };

        class DialogButtons
        {
        public:
            using OnButton = std::function<void(const std::string& buttonLabel)>;

            DialogButtons(OnButton onButton, const std::string& buttonA, const std::string& buttonB = "", const std::string& buttonC = "", const std::string& buttonD = "");

            void OnGuiPass(bool active);

        private:
            OnButton mOnButton;
            Strings mButtons;
        };

        
        //--------------------------------------------------------------------------------------------------
        class NonActive
        {
        public:
            NonActive(bool nonActive);
            ~NonActive();

        private:
            bool mNonActive = false;
        };


        namespace layout
        {
            std::string ButtonStrip(const Strings butonNames);
        }

    } // namespace gui
} // namespace yaget
