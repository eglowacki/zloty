#include "Render/Commands/RenderPasses.h"
#include "VTS/ResolvedAssets.h"
#include "Debugging/DevConfigurationParsers.h"
#include "Render/RenderStringHelpers.h"
#include "App/WindowFrame.h"

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


    uint32_t cast_to_int(auto enumType)
    {
        return static_cast<uint32_t>(enumType);
    }


    void ReadProjectionValue(const nlohmann::json& j, const char* key, yaget::render::commands::ScenePassData& passData, auto projectionValues, float defaultValue)
    {
        using namespace yaget;

        auto projectionIndex = cast_to_int(projectionValues);

        if (json::IsSectionValid(j, key, ""))
        {
            if (j[key].is_string())
            {
                auto enumValue = conv::FromString<render::commands::ScenePassData::ProjectionCalculationType>(j.value(key, "None").c_str());
                passData.mProjectionCalculationType[projectionIndex] = enumValue;
                passData.mProjection[projectionIndex] = -1;
            }
            else
            {
                passData.mProjection[projectionIndex] = j.value(key, defaultValue);
            }
        }
        else
        {
            passData.mProjection[projectionIndex] = defaultValue;
        }
    }

    void UpdateProjectionValue(auto projectionType, float* newValues, const float* oldValues, auto projectionCalculationType, const yaget::app::WindowFrame* windowFrame)
    {
        using namespace yaget;

        auto projectionIndex = cast_to_int(projectionType);

        if (projectionCalculationType[projectionIndex] == render::commands::ScenePassData::ProjectionCalculationType::WindowSizeX)
        {
            newValues[projectionIndex] = windowFrame->GetSurface().GetSizeX<float>();
        }
        else if (projectionCalculationType[projectionIndex] == render::commands::ScenePassData::ProjectionCalculationType::WindowSizeY)
        {
            newValues[projectionIndex] = windowFrame->GetSurface().GetSizeY<float>();
        }
        else
        {
            newValues[projectionIndex] = oldValues[projectionIndex];

        }
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

                ReadProjectionValue(orthographicSection, "Width", passData, ScenePassData::ProjectionValues1::Width, 1.0f);
                ReadProjectionValue(orthographicSection, "Height", passData, ScenePassData::ProjectionValues1::Height, 1.0f);
                ReadProjectionValue(orthographicSection, "Near", passData, ScenePassData::ProjectionValues1::Near, 0.0f);
                ReadProjectionValue(orthographicSection, "Far", passData, ScenePassData::ProjectionValues1::Far, 1.0f);

                break;
            }
            case ScenePassData::ProjectionType::OrthographicOffCenter:
            {
                auto orthographicOffCenter = j["OrthographicOffCenter"];

                ReadProjectionValue(orthographicOffCenter, "Left", passData, ScenePassData::ProjectionValues::Left, 0.0f);
                ReadProjectionValue(orthographicOffCenter, "Right", passData, ScenePassData::ProjectionValues::Right, 1.0f);
                ReadProjectionValue(orthographicOffCenter, "Top", passData, ScenePassData::ProjectionValues::Top, 0.0f);
                ReadProjectionValue(orthographicOffCenter, "Bottom", passData, ScenePassData::ProjectionValues::Bottom, 1.0f);
                ReadProjectionValue(orthographicOffCenter, "Near", passData, ScenePassData::ProjectionValues::Near, 0.0f);
                ReadProjectionValue(orthographicOffCenter, "Far", passData, ScenePassData::ProjectionValues::Far, 1.0f);

                break;
            }
            case ScenePassData::ProjectionType::PerspectiveFOV:
            {
                auto perspectiveFOVSection = j["PerspectiveFOV"];


                ReadProjectionValue(perspectiveFOVSection, "FOV", passData, ScenePassData::ProjectionValues2::FOV, 45.0f);
                ReadProjectionValue(perspectiveFOVSection, "AspectRatio", passData, ScenePassData::ProjectionValues2::AspectRatio, 1.0f);
                ReadProjectionValue(perspectiveFOVSection, "Near", passData, ScenePassData::ProjectionValues2::Near, 0.0f);
                ReadProjectionValue(perspectiveFOVSection, "Far", passData, ScenePassData::ProjectionValues2::Far, 1.0f);

                break;
            }
            case ScenePassData::ProjectionType::Perspective:
            {
                auto perspectiveSection = j["Perspective"];

                ReadProjectionValue(perspectiveSection, "Width", passData, ScenePassData::ProjectionValues1::Width, 1.0f);
                ReadProjectionValue(perspectiveSection, "Height", passData, ScenePassData::ProjectionValues1::Height, 1.0f);
                ReadProjectionValue(perspectiveSection, "Near", passData, ScenePassData::ProjectionValues1::Near, 0.0f);
                ReadProjectionValue(perspectiveSection, "Far", passData, ScenePassData::ProjectionValues1::Far, 1.0f);

                break;
            }
            case ScenePassData::ProjectionType::PerspectiveOffCenter:
            {
                auto perspectiveOffCenterSection = j["PerspectiveOffCenter"];

                ReadProjectionValue(perspectiveOffCenterSection, "Left", passData, ScenePassData::ProjectionValues::Left, 0.0f);
                ReadProjectionValue(perspectiveOffCenterSection, "Right", passData, ScenePassData::ProjectionValues::Right, 1.0f);
                ReadProjectionValue(perspectiveOffCenterSection, "Top", passData, ScenePassData::ProjectionValues::Top, 0.0f);
                ReadProjectionValue(perspectiveOffCenterSection, "Bottom", passData, ScenePassData::ProjectionValues::Bottom, 1.0f);
                ReadProjectionValue(perspectiveOffCenterSection, "Near", passData, ScenePassData::ProjectionValues::Near, 0.0f);
                ReadProjectionValue(perspectiveOffCenterSection, "Far", passData, ScenePassData::ProjectionValues::Far, 1.0f);

                break;
            }
        }
    }

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
    YAGET_ASSERT(mWindowFrame, "There is no valid window frame.");
    float projectionValues[NumElementsInProjection] = {};

    switch (mProjectionType)
    {
        case ProjectionType::Orthographic:
        {
            UpdateProjectionValue(ProjectionValues1::Width, projectionValues, mProjection, mProjectionCalculationType, mWindowFrame);
            UpdateProjectionValue(ProjectionValues1::Height, projectionValues, mProjection, mProjectionCalculationType, mWindowFrame);
            UpdateProjectionValue(ProjectionValues1::Near, projectionValues, mProjection, mProjectionCalculationType, mWindowFrame);
            UpdateProjectionValue(ProjectionValues1::Far, projectionValues, mProjection, mProjectionCalculationType, mWindowFrame);

            projectionMatrix = math3d::Matrix::CreateOrthographic(projectionValues[static_cast<uint32_t>(ProjectionValues1::Width)],
                                                                  projectionValues[static_cast<uint32_t>(ProjectionValues1::Height)],
                                                                  projectionValues[static_cast<uint32_t>(ProjectionValues1::Near)],
                                                                  projectionValues[static_cast<uint32_t>(ProjectionValues1::Far)]);
        }

            break;
        case ProjectionType::OrthographicOffCenter:
        {
            UpdateProjectionValue(ProjectionValues::Left, projectionValues, mProjection, mProjectionCalculationType, mWindowFrame);
            UpdateProjectionValue(ProjectionValues::Right, projectionValues, mProjection, mProjectionCalculationType, mWindowFrame);
            UpdateProjectionValue(ProjectionValues::Bottom, projectionValues, mProjection, mProjectionCalculationType, mWindowFrame);
            UpdateProjectionValue(ProjectionValues::Top, projectionValues, mProjection, mProjectionCalculationType, mWindowFrame);
            UpdateProjectionValue(ProjectionValues::Near, projectionValues, mProjection, mProjectionCalculationType, mWindowFrame);
            UpdateProjectionValue(ProjectionValues::Far, projectionValues, mProjection, mProjectionCalculationType, mWindowFrame);

            projectionMatrix = math3d::Matrix::CreateOrthographicOffCenter(projectionValues[static_cast<uint32_t>(ProjectionValues::Left)],
                                                                           projectionValues[static_cast<uint32_t>(ProjectionValues::Right)],
                                                                           projectionValues[static_cast<uint32_t>(ProjectionValues::Bottom)],
                                                                           projectionValues[static_cast<uint32_t>(ProjectionValues::Top)],
                                                                           projectionValues[static_cast<uint32_t>(ProjectionValues::Near)],
                                                                           projectionValues[static_cast<uint32_t>(ProjectionValues::Far)]);
        }
            break;
        case ProjectionType::PerspectiveFOV:
        {
            UpdateProjectionValue(ProjectionValues2::FOV, projectionValues, mProjection, mProjectionCalculationType, mWindowFrame);
            UpdateProjectionValue(ProjectionValues2::AspectRatio, projectionValues, mProjection, mProjectionCalculationType, mWindowFrame);
            UpdateProjectionValue(ProjectionValues2::Near, projectionValues, mProjection, mProjectionCalculationType, mWindowFrame);
            UpdateProjectionValue(ProjectionValues2::Far, projectionValues, mProjection, mProjectionCalculationType, mWindowFrame);
            
            projectionMatrix = math3d::Matrix::CreatePerspectiveFieldOfView(projectionValues[static_cast<uint32_t>(ProjectionValues2::FOV)],
                                                                            projectionValues[static_cast<uint32_t>(ProjectionValues2::AspectRatio)],
                                                                            projectionValues[static_cast<uint32_t>(ProjectionValues2::Near)],
                                                                            projectionValues[static_cast<uint32_t>(ProjectionValues2::Far)]);
        }
            break;
        case ProjectionType::Perspective:
        {
            UpdateProjectionValue(ProjectionValues1::Width, projectionValues, mProjection, mProjectionCalculationType, mWindowFrame);
            UpdateProjectionValue(ProjectionValues1::Height, projectionValues, mProjection, mProjectionCalculationType, mWindowFrame);
            UpdateProjectionValue(ProjectionValues1::Near, projectionValues, mProjection, mProjectionCalculationType, mWindowFrame);
            UpdateProjectionValue(ProjectionValues1::Far, projectionValues, mProjection, mProjectionCalculationType, mWindowFrame);

            
            projectionMatrix = math3d::Matrix::CreatePerspective(projectionValues[static_cast<uint32_t>(ProjectionValues1::Width)],
                                                                 projectionValues[static_cast<uint32_t>(ProjectionValues1::Height)],
                                                                 projectionValues[static_cast<uint32_t>(ProjectionValues1::Near)],
                                                                 projectionValues[static_cast<uint32_t>(ProjectionValues1::Far)]);
        }
            break;
        case ProjectionType::PerspectiveOffCenter:
        {
            UpdateProjectionValue(ProjectionValues::Left, projectionValues, mProjection, mProjectionCalculationType, mWindowFrame);
            UpdateProjectionValue(ProjectionValues::Right, projectionValues, mProjection, mProjectionCalculationType, mWindowFrame);
            UpdateProjectionValue(ProjectionValues::Bottom, projectionValues, mProjection, mProjectionCalculationType, mWindowFrame);
            UpdateProjectionValue(ProjectionValues::Top, projectionValues, mProjection, mProjectionCalculationType, mWindowFrame);
            UpdateProjectionValue(ProjectionValues::Near, projectionValues, mProjection, mProjectionCalculationType, mWindowFrame);
            UpdateProjectionValue(ProjectionValues::Far, projectionValues, mProjection, mProjectionCalculationType, mWindowFrame);

            projectionMatrix = math3d::Matrix::CreatePerspectiveOffCenter(projectionValues[static_cast<uint32_t>(ProjectionValues::Left)],
                                                                          projectionValues[static_cast<uint32_t>(ProjectionValues::Right)],
                                                                          projectionValues[static_cast<uint32_t>(ProjectionValues::Bottom)],
                                                                          projectionValues[static_cast<uint32_t>(ProjectionValues::Top)],
                                                                          projectionValues[static_cast<uint32_t>(ProjectionValues::Near)],
                                                                          projectionValues[static_cast<uint32_t>(ProjectionValues::Far)]);
        }
            break;
    }

    return projectionMatrix;
}

//-------------------------------------------------------------------------------------------------
yaget::render::commands::RenderPasses::RenderPasses(io::VirtualTransportSystem& vts, const app::WindowFrame& windowFrame)
    : mVTS{ vts }
    , mWindowFrame{ windowFrame }
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
            passData.mWindowFrame = &This->mWindowFrame;
            passData.mRenderTargetTag = This->mVTS.GetTag(passData.mRenderTargetSection);
            auto& itemsTags = passData.mSceneItemTags;

            std::ranges::for_each(passData.mSceneItemSections, [This, &itemsTags](auto& sceneItem)
            {
                itemsTags.push_back(This->mVTS.GetTag(sceneItem));
            });
        });
    }
}


//-------------------------------------------------------------------------------------------------
bool yaget::render::commands::RenderPasses::RenderThisPass(const io::Tag& tag, const ScenePassData& pass) const
{
    auto results = mPasses | std::views::take_while([&pass](const auto& element)
    {
        return pass.mName != element.mName;
    }) | std::views::filter([tag](const auto& element)
    {
        return std::ranges::find(element.mSceneItemTags, tag) != element.mSceneItemTags.end();
    });

    return results.empty();
}
