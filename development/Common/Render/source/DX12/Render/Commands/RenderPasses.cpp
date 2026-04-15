#include "Render/Commands/RenderPasses.h"

#include "VTS/ResolvedAssets.h"

namespace yaget::render::commands
{
    //-------------------------------------------------------------------------------------------------
    void from_json(const nlohmann::json& j, ScenePassData& passData)
    {
        passData.mName = j.value("Name", "");
        passData.mRenderTargetSection = j.value("RenderTarget", yaget::io::VirtualTransportSystem::Section{});
        passData.mSceneItemSections = j.value("SceneItems", yaget::io::VirtualTransportSystem::Sections{});
    }


    //-------------------------------------------------------------------------------------------------
    void to_json(nlohmann::json& j, const ScenePassData& passData)
    {
        j["Name"] = passData.mName;
        j["RenderTarget"] = passData.mRenderTargetSection.ToString();
        j["SceneItems"] = passData.mSceneItemSections;
    }

}


//-------------------------------------------------------------------------------------------------
yaget::render::commands::RenderPasses::RenderPasses(io::VirtualTransportSystem& vts)
    : mVTS(vts)
{
}


//-------------------------------------------------------------------------------------------------
yaget::render::commands::RenderPasses::~RenderPasses() = default;


//-------------------------------------------------------------------------------------------------
void yaget::render::commands::RenderPasses::BindAsset(const io::Tag& tag)
{
    if (mSceneTag != tag)
    {
        mSceneTag = tag;
        mPasses = LoadBlob<ScenePasses>(mVTS, tag);

        std::ranges::for_each(mPasses, [This = this](auto& passData)
        {
            passData.mRenderTargetTag = This->mVTS.GetTag(passData.mRenderTargetSection);
            auto& itemsTags = passData.mSceneItemTags;

            std::ranges::for_each(passData.mSceneItemSections, [This, &itemsTags](auto& sceneItem)
            {
                itemsTags.push_back(This->mVTS.GetTag(sceneItem));
            });
        });
    }
}
