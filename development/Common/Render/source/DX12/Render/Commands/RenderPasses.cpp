#include "Render/Commands/RenderPasses.h"
#include "VTS/ResolvedAssets.h"
#include "Debugging/DevConfigurationParsers.h"
#include "Render/RenderStringHelpers.h"

namespace
{
    //-------------------------------------------------------------------------------------------------
    yaget::render::commands::ScenePassData::ProjectionType GetProjectionType(const nlohmann::json& j)
    {
        using namespace yaget::render::commands;

        ScenePassData::ProjectionType projectionType = ScenePassData::ProjectionType::None;

        if (yaget::json::IsSectionValid(j, "Orthographic", ""))
        {
            projectionType = ScenePassData::ProjectionType::Orthographic;
        }
        else if (yaget::json::IsSectionValid(j, "OrthographicOffCenter", ""))
        {
            projectionType = ScenePassData::ProjectionType::OrthographicOffCenter;
        }
        else if (yaget::json::IsSectionValid(j, "PerspectiveFOV", ""))
        {
            projectionType = ScenePassData::ProjectionType::PerspectiveFOV;
        }
        else if (yaget::json::IsSectionValid(j, "Perspective", ""))
        {
            projectionType = ScenePassData::ProjectionType::Perspective;
        }
        else if (yaget::json::IsSectionValid(j, "PerspectiveOffCenter", ""))
        {
            projectionType = ScenePassData::ProjectionType::PerspectiveOffCenter;
        }

        return projectionType;
    }

}


namespace yaget::render::commands
{
    //-------------------------------------------------------------------------------------------------
    void from_json(const nlohmann::json& j, ScenePassData& passData)
    {
        passData.mName = j.value("Name", "");
        passData.mRenderTargetSection = j.value("RenderTarget", io::VirtualTransportSystem::Section{});
        passData.mSceneItemSections = j.value("SceneItems", io::VirtualTransportSystem::Sections{});

        passData.mClearValues.mUseClearColor = json::GetValue(j, "ColorClear", false);
        passData.mClearValues.mUseClearDepth = json::GetValue(j, "DepthStencilClear", false);

        if (json::IsSectionValid(j, "LookAt", ""))
        {
            passData.mCameraValid = true;
            auto lookAtSection = j["LookAt"];

            std::array<float, 3> values{};
            values = json::GetValue(lookAtSection, "Position", values);
            passData.mViewCamera[static_cast<uint32_t>(ScenePassData::CameraValues::Position)] = math3d::Vector3(values[0], values[1], values[2]);

            values = {};
            values = json::GetValue(lookAtSection, "Target", values);
            passData.mViewCamera[static_cast<uint32_t>(ScenePassData::CameraValues::Target)] = math3d::Vector3(values[0], values[1], values[2]);

            values = {};
            values = json::GetValue(lookAtSection, "Up", values);
            passData.mViewCamera[static_cast<uint32_t>(ScenePassData::CameraValues::Up)] = math3d::Vector3(values[0], values[1], values[2]);
        }

        passData.mProjectionType = GetProjectionType(j);

        switch (passData.mProjectionType)
        {
            case ScenePassData::ProjectionType::Orthographic:
            {
                auto orthographicSection = j["Orthographic"];
                passData.mProjection[static_cast<uint32_t>(ScenePassData::ProjectionValues1::Width)] = orthographicSection.value("Width", 1.f);
                passData.mProjection[static_cast<uint32_t>(ScenePassData::ProjectionValues1::Height)] = orthographicSection.value("Height", 1.f);
                passData.mProjection[static_cast<uint32_t>(ScenePassData::ProjectionValues1::Near)] = orthographicSection.value("Near", 0.f);
                passData.mProjection[static_cast<uint32_t>(ScenePassData::ProjectionValues1::Far)] = orthographicSection.value("Far", 1.f);
                break;
            }
            case ScenePassData::ProjectionType::OrthographicOffCenter:
            {
                auto orthographicOffCenter = j["OrthographicOffCenter"];
                passData.mProjection[static_cast<uint32_t>(ScenePassData::ProjectionValues::Left)] = orthographicOffCenter.value("Left", 0.f);
                passData.mProjection[static_cast<uint32_t>(ScenePassData::ProjectionValues::Right)] = orthographicOffCenter.value("Right", 1.f);
                passData.mProjection[static_cast<uint32_t>(ScenePassData::ProjectionValues::Top)] = orthographicOffCenter.value("Top", 0.f);
                passData.mProjection[static_cast<uint32_t>(ScenePassData::ProjectionValues::Bottom)] = orthographicOffCenter.value("Bottom", 1.f);
                passData.mProjection[static_cast<uint32_t>(ScenePassData::ProjectionValues::Near)] = orthographicOffCenter.value("Near", 0.f);
                passData.mProjection[static_cast<uint32_t>(ScenePassData::ProjectionValues::Far)] = orthographicOffCenter.value("Far", 1.f);
                break;
            }
            case ScenePassData::ProjectionType::PerspectiveFOV:
            {
                auto perspectiveFOVSection = j["PerspectiveFOV"];
                passData.mProjection[static_cast<uint32_t>(ScenePassData::ProjectionValues2::FOV)] = perspectiveFOVSection.value("FOV", 45.f);
                passData.mProjection[static_cast<uint32_t>(ScenePassData::ProjectionValues2::AspectRatio)] = perspectiveFOVSection.value("AspectRatio", 1.f);
                passData.mProjection[static_cast<uint32_t>(ScenePassData::ProjectionValues2::Near)] = perspectiveFOVSection.value("Near", 0.f);
                passData.mProjection[static_cast<uint32_t>(ScenePassData::ProjectionValues2::Far)] = perspectiveFOVSection.value("Far", 1.f);
                break;
            }
            case ScenePassData::ProjectionType::Perspective:
            {
                auto perspectiveSection = j["Perspective"];
                passData.mProjection[static_cast<uint32_t>(ScenePassData::ProjectionValues1::Width)] = perspectiveSection.value("Width", 1.f);
                passData.mProjection[static_cast<uint32_t>(ScenePassData::ProjectionValues1::Height)] = perspectiveSection.value("Height", 1.f);
                passData.mProjection[static_cast<uint32_t>(ScenePassData::ProjectionValues1::Near)] = perspectiveSection.value("Near", 0.f);
                passData.mProjection[static_cast<uint32_t>(ScenePassData::ProjectionValues1::Far)] = perspectiveSection.value("Far", 1.f);
                break;
            }
            case ScenePassData::ProjectionType::PerspectiveOffCenter:
            {
                auto perspectiveOffCenterSection = j["PerspectiveOffCenter"];
                passData.mProjection[static_cast<uint32_t>(ScenePassData::ProjectionValues::Left)] = perspectiveOffCenterSection.value("Left", 0.f);
                passData.mProjection[static_cast<uint32_t>(ScenePassData::ProjectionValues::Right)] = perspectiveOffCenterSection.value("Right", 1.f);
                passData.mProjection[static_cast<uint32_t>(ScenePassData::ProjectionValues::Top)] = perspectiveOffCenterSection.value("Top", 0.f);
                passData.mProjection[static_cast<uint32_t>(ScenePassData::ProjectionValues::Bottom)] = perspectiveOffCenterSection.value("Bottom", 1.f);
                passData.mProjection[static_cast<uint32_t>(ScenePassData::ProjectionValues::Near)] = perspectiveOffCenterSection.value("Near", 0.f);
                passData.mProjection[static_cast<uint32_t>(ScenePassData::ProjectionValues::Far)] = perspectiveOffCenterSection.value("Far", 1.f);
                break;
            }
        }
    }


    ////-------------------------------------------------------------------------------------------------
    //void to_json(nlohmann::json& j, const ScenePassData& passData)
    //{
    //    j["Name"] = passData.mName;
    //    j["RenderTarget"] = passData.mRenderTargetSection.ToString();
    //    j["SceneItems"] = passData.mSceneItemSections;
    //}

}


//-------------------------------------------------------------------------------------------------
const math3d::Color* yaget::render::commands::ScenePassData::GetColorClear(const RenderTarget* renderTarget) const
{
    if (mClearValues.mUseClearColor && renderTarget)
    {
        return &renderTarget->GetColorClear();
    }

    return nullptr;
}


//-------------------------------------------------------------------------------------------------
const yaget::render::commands::DepthStencilClear* yaget::render::commands::ScenePassData::GetDepthStencilClear(const RenderTarget* renderTarget) const
{
    if (mClearValues.mUseClearDepth && renderTarget)
    {
        return &renderTarget->GetDepthStencilClear();
    }

    return nullptr;
}

//-------------------------------------------------------------------------------------------------
math3d::Matrix yaget::render::commands::ScenePassData::GetViewMatrix() const
{
    math3d::Matrix viewMatrix = math3d::Matrix::Identity;
    if (mCameraValid)
    {
        viewMatrix = math3d::Matrix::CreateLookAt(mViewCamera[static_cast<uint32_t>(CameraValues::Position)],
                                                  mViewCamera[static_cast<uint32_t>(CameraValues::Target)],
                                                  mViewCamera[static_cast<uint32_t>(CameraValues::Up)]);
    }

    return viewMatrix;
}


//-------------------------------------------------------------------------------------------------
math3d::Matrix yaget::render::commands::ScenePassData::GetProjectionMatrix() const
{
    math3d::Matrix projectionMatrix = math3d::Matrix::Identity;

    switch (mProjectionType)
    {
        case ProjectionType::Orthographic:
            projectionMatrix = math3d::Matrix::CreateOrthographic(mProjection[static_cast<uint32_t>(ProjectionValues1::Width)],
                                                                  mProjection[static_cast<uint32_t>(ProjectionValues1::Height)],
                                                                  mProjection[static_cast<uint32_t>(ProjectionValues1::Near)],
                                                                  mProjection[static_cast<uint32_t>(ProjectionValues1::Far)]);
            break;
        case ProjectionType::OrthographicOffCenter:
            projectionMatrix = math3d::Matrix::CreateOrthographicOffCenter(mProjection[static_cast<uint32_t>(ProjectionValues::Left)],
                                                                           mProjection[static_cast<uint32_t>(ProjectionValues::Right)],
                                                                           mProjection[static_cast<uint32_t>(ProjectionValues::Bottom)],
                                                                           mProjection[static_cast<uint32_t>(ProjectionValues::Top)],
                                                                           mProjection[static_cast<uint32_t>(ProjectionValues::Near)],
                                                                           mProjection[static_cast<uint32_t>(ProjectionValues::Far)]);
            break;
        case ProjectionType::PerspectiveFOV:
            projectionMatrix = math3d::Matrix::CreatePerspectiveFieldOfView(mProjection[static_cast<uint32_t>(ProjectionValues2::FOV)],
                                                                            mProjection[static_cast<uint32_t>(ProjectionValues2::AspectRatio)],
                                                                            mProjection[static_cast<uint32_t>(ProjectionValues2::Near)],
                                                                            mProjection[static_cast<uint32_t>(ProjectionValues2::Far)]);
            break;
        case ProjectionType::Perspective:
            projectionMatrix = math3d::Matrix::CreatePerspective(mProjection[static_cast<uint32_t>(ProjectionValues1::Width)],
                                                                 mProjection[static_cast<uint32_t>(ProjectionValues1::Height)],
                                                                 mProjection[static_cast<uint32_t>(ProjectionValues1::Near)],
                                                                 mProjection[static_cast<uint32_t>(ProjectionValues1::Far)]);
            break;
        case ProjectionType::PerspectiveOffCenter:
            projectionMatrix = math3d::Matrix::CreatePerspectiveOffCenter(mProjection[static_cast<uint32_t>(ProjectionValues::Left)],
                                                                          mProjection[static_cast<uint32_t>(ProjectionValues::Right)],
                                                                          mProjection[static_cast<uint32_t>(ProjectionValues::Bottom)],
                                                                          mProjection[static_cast<uint32_t>(ProjectionValues::Top)],
                                                                          mProjection[static_cast<uint32_t>(ProjectionValues::Near)],
                                                                          mProjection[static_cast<uint32_t>(ProjectionValues::Far)]);
            break;
    }

    return projectionMatrix;
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
