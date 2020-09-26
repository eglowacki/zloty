#include "Resources/SkinShaderResource.h"
#include "VTS/ResolvedAssets.h"
#include "Streams/Buffers.h"
#include "VTS/VirtualTransportSystem.h"
#include "Json/JsonHelpers.h"
#include "Resources/ShaderResources.h"
#include "Device.h"
#include "Debugging/Assert.h"
#include "App/Application.h"
#include "Logger/YLog.h"
#include "imgui.h"

/*
{
    "Type": "Material",

    "Position" : {
        "Vertex": "Shaders@BasicVS",
        "Pixel" : "Shaders@BasicPS"
    }
}
*/

//--------------------------------------------------------------------------------------------------
yaget::render::SkinShaderResource::SkinShaderResource(render::Device& device, std::shared_ptr<io::render::MaterialAsset> asset)
    : ResourceView(device, asset->mTag, std::type_index(typeid(SkinShaderResource)))
    , mRefreshVertexId(idspace::get_burnable(mDevice.App().IdCache))
    , mRefreshPixelId(idspace::get_burnable(mDevice.App().IdCache))
{
    mDevice.RequestResourceView<VertexShaderResource>(asset->mVertexTag, std::ref(mVertex), mRefreshVertexId);
    mDevice.RequestResourceView<ConstantsResource>(asset->mVertexTag, std::ref(mConstantBuffer), 0);
    mDevice.RequestResourceView<InputLayoutResource>(asset->mVertexTag, std::ref(mInput), 0);

    mDevice.RequestResourceView<PixelShaderResource>(asset->mPixelTag, std::ref(mPixel), mRefreshPixelId);

    std::size_t hashValue = asset->mTag.Hash();
    SetHashValue(hashValue);
}

yaget::render::SkinShaderResource::~SkinShaderResource()
{
    mDevice.RemoveWatch(mRefreshVertexId);
    mDevice.RemoveWatch(mRefreshPixelId);
}

//--------------------------------------------------------------------------------------------------
bool yaget::render::SkinShaderResource::Activate()
{
    mDevice.ActivatedResource(this, fmt::format("This: {}, Hash: {}", static_cast<void*>(this), GetStateHash()).c_str());

    mt::SmartVariable<VertexShaderResource>::SmartType vs = mVertex;
    mt::SmartVariable<PixelShaderResource>::SmartType ps = mPixel;
    mt::SmartVariable<InputLayoutResource>::SmartType il = mInput;
    mt::SmartVariable<ConstantsResource>::SmartType constants = mConstantBuffer;

    if (vs && ps && il && constants)
    {
        mDevice.SetConstants(constants);
        return vs->Activate() &&
            ps->Activate() &&
            il->Activate();
    }

    return false;
}

void yaget::render::SkinShaderResource::UpdateGui(comp::Component::UpdateGuiType updateGuiType)
{
    using Section = io::VirtualTransportSystem::Section;

    if (updateGuiType == comp::Component::UpdateGuiType::Default)
    {
        mt::SmartVariable<VertexShaderResource>::SmartType vertex = mVertex;
        mt::SmartVariable<PixelShaderResource>::SmartType pixel = mPixel;
        mt::SmartVariable<InputLayoutResource>::SmartType input = mInput;
        mt::SmartVariable<ConstantsResource>::SmartType constants = mConstantBuffer;

        math3d::Color tagColor(dev::CurrentConfiguration().mGuiColors.at("SectionText"));
        if (vertex)
        {
            gui::UpdateSectionText("Vertex Tag:", tagColor, vertex.get(), updateGuiType);
        }
        if (pixel)
        {
            gui::UpdateSectionText("Pixel Tag:", tagColor, pixel.get(), updateGuiType);
        }
        if (input)
        {
            gui::UpdateSectionText("Input Tag:", tagColor, input.get(), updateGuiType);
        }
        if (constants)
        {
            gui::UpdateSectionText("Constants Tag:", tagColor, constants.get(), updateGuiType);
        }
    }
}

