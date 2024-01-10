#include "App/Application.h"
#include "Components/GeometryComponent.h"
#include "Resources/GeometryResource.h"
#include "Resources/SkinShaderResource.h"
#include "Resources/DescriptionResource.h"
#include "VTS/RenderResolvedAssets.h"
#include "RenderMathFacade.h"
#include "Device.h"
#include "imgui.h"
#include "Gui/Support.h"

/*
{
    "Type": "Geometry",

    "Rectangle" : {
        "Geometry": "Geometry@Predefined/Rectangle",
        "Material" : "Materials@Position"
    }
}
*/

yaget::render::GeometryComponent::GeometryComponent(comp::Id_t id, render::Device& device, const std::vector<io::Tag>& tags)
    : RenderComponent(id, device, Init::Default, tags)
    , mActivePasses({ "Default" })
{
    YLOG_CERROR("REND", tags.size() == ((const std::vector<io::Tag>&)mTags).size(), "Incomming tags for GeometryComponent '%d' are not valid.", id);
}

void yaget::render::GeometryComponent::OnReset()
{
    mDescriptionsWatcher = {};

    std::vector<io::Tag> tags = mTags;
    mDescriptionsWatcher = std::make_unique<DescriptionsWatcher>(mDevice, tags);
}

void yaget::render::GeometryComponent::onRender(const RenderTarget* /*renderTarget*/, const math3d::Matrix& /*matrix*/)
{
    mDevice.ActivatedResource(nullptr, fmt::format("    Geometry '{}' Started. This: {}", static_cast<comp::Id_t>(Id()), static_cast<void*>(this)).c_str());

    bool result = mDescriptionsWatcher->Process([this](auto description)
    {
        return description->Activate(mActivePasses);
    });

    if (result)
    {
        mDevice.UpdateConstant("PerObject", "=", mPerObjectConstants);
        mDevice.ActivateConstants();
    }
}


void yaget::render::GeometryComponent::SetMatrix(const math3d::Matrix& matrix)
{
    YAGET_ASSERT(dev::CurrentThreadIds().IsThreadRender(), "SetMatrix must be called from RENDER thread.");

    mPerObjectConstants.mMatrix = matrix;
}


void yaget::render::GeometryComponent::SetColor(const colors::Color& color)
{
    YAGET_ASSERT(dev::CurrentThreadIds().IsThreadRender(), "SetColor must be called from RENDER thread.");

    mPerObjectConstants.mColor = color;
}


void yaget::render::GeometryComponent::SetPasses(const Strings& passes)
{
    mActivePasses = passes;
}


void yaget::render::GeometryComponent::onUpdateGui(UpdateGuiType updateGuiType)
{
    using Section = io::VirtualTransportSystem::Section;

    if (updateGuiType == Component::UpdateGuiType::Default)
    {
        auto loc = mPerObjectConstants.mMatrix.Translation();

        bool visible = ImGui::TreeNode(fmt::format("Entity Id: '{}'", static_cast<comp::Id_t>(Id())).c_str());
        std::string locText = conv::Convertor<math3d::Vector3>::ToString(loc);
        ImGui::SameLine();
        yaget::gui::Text(fmt::format("Position: '{}'", locText), math3d::Color{ 0.6f, 0.6f, 0.6f, 1.0f });

        if (visible)
        {
            mDescriptionsWatcher->Process([updateGuiType](auto description)
            {
                math3d::Color tagColor(dev::CurrentConfiguration().mGuiColors.at("SectionText"));
                gui::UpdateSectionText("Description, Tag:", tagColor, description.get(), updateGuiType);

                return true;
            });

            ImGui::TreePop();
        }
    }
}
