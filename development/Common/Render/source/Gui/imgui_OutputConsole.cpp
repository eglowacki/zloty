/**
 * @file    OutputConsole.cpp
 * @ingroup LoggerCpp
 * @brief   Output to the standard console using fprintf() with stdout
 *
 * Copyright (c) 2013 Sebastien Rombauts (sebastien.rombauts@gmail.com)
 *
 * Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
 * or copy at http://opensource.org/licenses/MIT)
 */

#include "Gui/imgui_OutputConsole.h"
#include "Base.h"
#include "imgui.h"
#include "Fmt/format.h"
#include "Gui/Support.h"
#include "RenderMathFacade.h"
#include <cstdio>
#include <mutex>
 
using namespace yaget;
using namespace yaget::imgui;

namespace
{
    struct ExampleAppLog
    {
        struct LogLine
        {
            std::string mText;
            math3d::Color mColor;
        };
        std::vector<LogLine> mLogLines;
        bool ScrollToBottom = true;
        std::mutex mLinesMutex;

        void AddLog(const std::string& text, const math3d::Color& color)
        {
            mLinesMutex.lock();
            mLogLines.push_back(LogLine{ text , color });
            ScrollToBottom = true;
            mLinesMutex.unlock();
        }

        void Draw(const char* title, bool* p_opened = nullptr)
        {
            mLinesMutex.lock();
            ImGui::Begin(title, p_opened);
            for (auto&& it : mLogLines)
            {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(it.mColor.R(), it.mColor.G(), it.mColor.B(), it.mColor.A()));
                ImGui::TextWrapped(it.mText.c_str());
                ImGui::PopStyleColor();
            }
            if (ScrollToBottom)
            {
                ImGui::SetScrollHereY(1.0f);
            }

            ScrollToBottom = false;
            ImGui::End();

            if (mLogLines.size() > 1500)
            {
                mLogLines.erase(mLogLines.begin(), mLogLines.begin() + 1000);
            }

            mLinesMutex.unlock();
        }
    };

    ExampleAppLog exampleAppLog;

} // namespace


// declearation is is Gui/Support.h file
void yaget::gui::DrawLogs(const char* title, bool* p_opened /*= nullptr*/)
{
    exampleAppLog.Draw(title, p_opened);
}

OutputConsole::OutputConsole(const ylog::Config::Ptr& /*aConfigPtr*/)
{ }

OutputConsole::~OutputConsole()
{ }

// Convert a Level to a Win32 console color text attribute
/*static*/ math3d::Color OutputConsole::toWin32Attribute(ylog::Log::Level aLevel)
{
    math3d::Color color;

    switch (aLevel)
    {
        case ylog::Log::Level::eDebug:
            color = DirectX::Colors::Gray;
            break;
        case ylog::Log::Level::eInfo:
            color = DirectX::Colors::Green;
            break;
        case ylog::Log::Level::eNotice:
            color = DirectX::Colors::Yellow;
            break;
        case ylog::Log::Level::eWarning:
            color = DirectX::Colors::Orchid;
            break;
        case ylog::Log::Level::eError:
            color = DirectX::Colors::Red;
            break;
        case ylog::Log::Level::eCritic:
            color = DirectX::Colors::Purple;
            break;
        default:
            color = DirectX::Colors::Aqua;
            break;
    }

    return color;
}

// Output the Log to the standard console using fprintf
void OutputConsole::OnOutput(const ylog::Channel::Ptr& aChannelPtr, const ylog::Log& aLog) const
{
    const ylog::DateTime& time = aLog.getTime();
    math3d::Color color = toWin32Attribute(aLog.getSeverity());

    char tag[5] = { '\0' };
    if (aLog.GetTag())
    {
        *(reinterpret_cast<uint32_t*>(tag)) = aLog.GetTag();
    }

    std::string message = fmt::format("{}  {} [{}{}{}] {}",
        time.ToString(),
        aChannelPtr->getName(),
        ylog::Log::toString(aLog.getSeverity()), tag ? ":" : "", tag,
        aLog.getStream().str().c_str());

    exampleAppLog.AddLog(message, color);
}
