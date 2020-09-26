#include "Resources/ResourceView.h"
#include "VTS/VirtualTransportSystem.h"
#include "imgui.h"
#include "Gui/Support.h"


void yaget::render::gui::UpdateSectionText(const std::string& nodeText, const colors::Color& textColor, render::ResourceView* resourceView, comp::Component::UpdateGuiType updateGuiType)
{
    using Section = io::VirtualTransportSystem::Section;
    const math3d::Color& nodeColor(dev::CurrentConfiguration().mGuiColors.at("NodeText"));

    ImGui::PushStyleColor(ImGuiCol_Text, nodeColor);
    bool visible = ImGui::TreeNode(nodeText.c_str());
    ImGui::PopStyleColor();

    ImGui::SameLine();
    yaget::gui::Text(fmt::format("'{}'", Section(resourceView->mAssetTag).ToString()), textColor);

    if (visible)
    {
        resourceView->UpdateGui(updateGuiType);
        ImGui::TreePop();
    }
}

std::size_t yaget::render::ResourceView::GetStateHash() const
{
    YAGET_ASSERT(mHashValue, "Hash value is not calculated for state resource.");
    return mHashValue;
}
