#include "Gui/Controls.h"
#include "Debugging/Assert.h"
#include "Gui/Support.h"
#include "imgui.h"
#include "..\imgui\imgui_internal.h"
#include <algorithm>


bool yaget::gui::BufferingBar(const std::string& /*label*/, float /*value*/, const math3d::Vector2& /*size_arg*/, uint32_t /*bg_col*/, uint32_t /*fg_col*/)
{
    ////ImGuiWindow* window = ImGui::GetCurrentWindow();
    ////if (window->SkipItems)
    ////{
    ////    return false;
    ////}

    ////ImGuiIO& io = ImGui::GetIO();

    ////ImGuiContext& g = *GImGui;
    ////const ImGuiStyle& style = g.Style;
    //const ImGuiStyle& style = ImGui::GetStyle();
    ////const ImGuiID id = window->GetID(label.c_str());

    //ImVec2 pos = ImGui::GetCursorPos();// window->DC.CursorPos;
    //ImVec2 size(size_arg.x, size_arg.y);
    //size.x -= style.FramePadding.x * 2;

    ////const ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));
    ////ItemSize(bb, style.FramePadding.y);
    ////if (!ItemAdd(bb, id))
    ////    return false;

    //// Render
    //const float circleStart = size.x * 0.7f;
    //const float circleEnd = size.x;
    //const float circleWidth = circleEnd - circleStart;

    //window->DrawList->AddRectFilled(bb.Min, ImVec2(pos.x + circleStart, bb.Max.y), bg_col);
    //window->DrawList->AddRectFilled(bb.Min, ImVec2(pos.x + circleStart * value, bb.Max.y), fg_col);

    //const float t = ImGui::GetTime();
    //const float r = size.y / 2;
    //const float speed = 1.5f;

    //const float a = speed * 0;
    //const float b = speed * 0.333f;
    //const float c = speed * 0.666f;

    //const float o1 = (circleWidth + r) * (t + a - speed * (int)((t + a) / speed)) / speed;
    //const float o2 = (circleWidth + r) * (t + b - speed * (int)((t + b) / speed)) / speed;
    //const float o3 = (circleWidth + r) * (t + c - speed * (int)((t + c) / speed)) / speed;

    //window->DrawList->AddCircleFilled(ImVec2(pos.x + circleEnd - o1, bb.Min.y + r), r, bg_col);
    //window->DrawList->AddCircleFilled(ImVec2(pos.x + circleEnd - o2, bb.Min.y + r), r, bg_col);
    //window->DrawList->AddCircleFilled(ImVec2(pos.x + circleEnd - o3, bb.Min.y + r), r, bg_col);
    // 
    return false;
}


yaget::gui::ComboBox::ComboBox(OnEntrySelection onEntrySelection, const std::string& label)
    : mOnEntrySelection(onEntrySelection)
    , mLabel(label)
{
}

void yaget::gui::ComboBox::SetEntries(const Strings& entries, const std::string& selected)
{
    mEntries = entries;
    auto it = std::find(mEntries.begin(), mEntries.end(), selected);
    if (it == mEntries.end())
    {
        it = mEntries.begin();
    }

    if (mSelectedText != *it)
    {
        mOnEntrySelection(*it, mSelectedText);
        mSelectedText = *it;
    }
}

void yaget::gui::ComboBox::OnGuiPass(bool active)
{
    NonActive nonActiveState(!active);

    gui::Fonter fonter("Medium");

    if (ImGui::BeginCombo(mLabel.c_str(), mSelectedText.c_str()))
    {
        for (const auto& entry : mEntries)
        {
            bool result = ImGui::Selectable(entry.c_str(), entry == mSelectedText);
            if (result)
            {
                if (mSelectedText != entry)
                {
                    // different entry selected
                    mOnEntrySelection(entry, mSelectedText);
                }

                mSelectedText = entry;
            }
        }

        ImGui::EndCombo();
    }
}


yaget::gui::CheckBox::CheckBox(OnStateChange onStateChange, const std::string& label)
    : mOnStateChange(onStateChange)
    , mLabel(label)
{
}


void yaget::gui::CheckBox::OnGuiPass(bool active)
{
    NonActive nonActiveState(!active);

    gui::Fonter fonter("Medium");

    bool lastSelected = mSelected;
    ImGui::Checkbox("VSync", &mSelected);

    if (lastSelected != mSelected)
    {
        mOnStateChange(mSelected);
    }
}

yaget::gui::DialogButtons::DialogButtons(OnButton onButton, const std::string& buttonA, const std::string& buttonB, const std::string& buttonC, const std::string& buttonD)
    : mOnButton(onButton)
    , mButtons({ buttonA , buttonB, buttonC, buttonD })
{
    // remove any empty entries
    mButtons.erase(std::remove_if(std::begin(mButtons), std::end(mButtons), [](const std::string& entry) { return entry.empty(); }), std::end(mButtons));
    YAGET_ASSERT(!mButtons.empty(), "No Button Lables specified.");
}

void yaget::gui::DialogButtons::OnGuiPass(bool active)
{
    NonActive nonActiveState(!active);
    gui::Fonter fonter("Large");

    std::string buttonPressed = layout::ButtonStrip(mButtons);

    if (!buttonPressed.empty())
    {
        mOnButton(buttonPressed);
    }
}

yaget::gui::NonActive::NonActive(bool nonActive)
    : mNonActive(nonActive)
{
    if (mNonActive)
    {
        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
    }
}

yaget::gui::NonActive::~NonActive()
{
    if (mNonActive)
    {
        ImGui::PopItemFlag();
        ImGui::PopStyleVar();
    }
}


std::string yaget::gui::layout::ButtonStrip(const Strings butonNames)
{
    //gui::Fonter fonter("Large");

    const float kSpaceBetweenButtons = 20.0f;
    const float kButtonPadding = 5.0f;
    float buttonsLenght = 0.0f;
    for (const auto& it : butonNames)
    {
        const ImVec2 labelSize = ImGui::CalcTextSize(it.c_str(), nullptr, true);
        ImVec2 size = ImGui::CalcItemSize(ImVec2(0, 0), labelSize.x + ImGui::GetStyle().FramePadding.x * 2.0f, labelSize.y + ImGui::GetStyle().FramePadding.y * 2.0f);
        buttonsLenght += size.x;
    }

    buttonsLenght += (kSpaceBetweenButtons * (butonNames.size() - 1)) + kButtonPadding;

    ImGui::BeginGroup();

        ImGui::BeginChild("filler", ImVec2(0, -(ImGui::GetFrameHeightWithSpacing() + 5))); // Leave room for 1 line below us
        ImGui::EndChild();

        ImGui::BeginChild("filler2", ImVec2(-buttonsLenght, 0));    // leave room between fillers edge and windows right edge
        ImGui::EndChild();

        ImGui::SameLine(0.0f, 0.0f);

        std::string buttonPressed;
        for (const auto& it : butonNames)
        {
            if (ImGui::Button(it.c_str()))
            {
                buttonPressed = it;
            }
            ImGui::SameLine(0.0f, kSpaceBetweenButtons);
        }

    ImGui::EndGroup();

    return buttonPressed;
}

