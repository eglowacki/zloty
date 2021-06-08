//////////////////////////////////////////////////////////////////////
// Support.h
//
//  Copyright 7/22/2018 Edgar Glowacki
//
//  Maintained by: Edgar
//
//  NOTES:
//      Interface to ui systems to abstract basic management of gui
//
//
//  #include "Gui/Support.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "YagetCore.h"
#include "RenderMathFacade.h"
#include "StringHelpers.h"
#include "Meta/CompilerAlgo.h"


namespace yaget
{
    namespace render { class Device; }
    namespace io { class VirtualTransportSystem; }

    namespace gui
    {
        // resets and regenerates ui device context, die to reset of device, resize window, etc
        struct Reseter
        {
            Reseter(render::Device& device);
            ~Reseter();

        private:
            render::Device& mDevice;
        };

        struct Fonter
        {
            Fonter(const std::string& name);
            ~Fonter();

        private:
            std::string mName;
        };

        void Initialize(render::Device& device);
        void Shutdown();
        void NewFrame();
        void Draw();
        int64_t ProcessInput(void* handle, uint32_t message, uint64_t param1, int64_t param2);

        void DrawLogs(const char* title, bool* p_opened = nullptr);

        namespace layout
        {
            void CenterFrame(int width, int height);
            void CenterText(const std::string& text, float red, float green, float blue, float alpha);
        } // namespace layout

        bool ColorEdit4(const std::string& label, math3d::Color& color);
        void MakeDisabled(bool disabled, std::function<void()> callback);
        void SetTooltip(const std::string& tooltipText);
        void Text(const std::string& text, const colors::Color& textColor);
        void Text(const std::string& text, const std::string& colorName);

        void SameLine();
        void NewLine();

        // Print colorized text
        // TextColors(color1, text1, color2, text2 ... colorN, textN);
        template<typename ...Args>
        void TextColors(Args&&... args)
        {
            using Sources = std::tuple<Args...>;
            static constexpr size_t NumSources = std::tuple_size_v<Sources>;
            static_assert(NumSources % 2 == 0, "Number of elements passed must be even.");

            Sources sources(args...);

            yaget::meta::for_each_pair(sources, [](const auto& textColor, const auto& valueToPrint)
            {
                using ValueType = std::decay_t<decltype(valueToPrint)>;

                yaget::gui::Text(fmt::format("{}", conv::Convertor<ValueType>::ToString(valueToPrint)), textColor);
                yaget::gui::SameLine();
            });

            yaget::gui::NewLine();
        }


    } // namespace gui
} // namespace yaget

