#include "Gui/Layout.h"
#include "RenderMathFacade.h"
#include "Debugging/Assert.h"
#include "imgui.h"


void yaget::gui::SnapTo(Border border, const std::string& name)
{
    const float kSideOffset = 5.0f;
    float controlHeight = 150.0f;
    ImGuiIO& io = ImGui::GetIO();
    math3d::Vector2 areaSize(io.DisplaySize.x, io.DisplaySize.y);

    if (!name.empty())
    {
        ImGui::Begin(name.c_str(), nullptr);
        ImVec2 winSize = ImGui::GetWindowSize();
        ImVec2 winPos = ImGui::GetWindowPos();

        controlHeight = io.DisplaySize.y - winPos.y;
        ImGui::End();
    }

    if (border == Border::Bottom)
    {
        //const float kXOffset = 5.0f;
        //const float kYOffset = 150.0f;

        ImVec2 logWindowPos(kSideOffset, areaSize.y - controlHeight);
        ImGui::SetNextWindowPos(logWindowPos, ImGuiCond_Always);

        ImVec2 logWindowSize(areaSize.x - (kSideOffset * 2), controlHeight);
        ImGui::SetNextWindowSize(logWindowSize, ImGuiCond_Always);
    }
    else
    {
        YAGET_ASSERT(false, "Border type: '%d' in gui snap to is not implemented.", border);
    }
}
