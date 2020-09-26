#include "Scene.h"
#include "Device.h"
#include "RenderTarget.h"
#include "Resources/ShaderResources.h"
#include "App/Application.h"
#include "StringHelpers.h"
#include "Debugging/Assert.h"
#include "Debugging/DevConfiguration.h"
#include "Debugging/PhysicsDebugDraw.h"
#include "Exception/Exception.h"
#include "App/AppUtilities.h"
#include "Loaders/GeometryConvertor.h"
#include "STLHelper.h"
#include "tinyxml.h"
#include "Gui/Support.h"
#include "Gui/Layout.h"

#include "imgui.h"
#include "Gui/imgui_OutputConsole.h"

#include <assimp/Importer.hpp>      // C++ importer interface
#include <assimp/scene.h>           // Output data structure
#include <assimp/postprocess.h>     // Post processing flags
#include <regex>

#include <filesystem>

#pragma warning( push )
#pragma warning( disable : 4127 )  // conditional expression is constant
#pragma warning( disable : 4099 )  // 'btDefaultMotionState': type name first seen using 'class' now seen using 'struct'
#include "btBulletDynamicsCommon.h" 
#pragma warning( pop )

using namespace yaget;
namespace fs = std::filesystem;

render::Scene::PBPtr_t render::Scene::PBR_SET;
render::Scene::RBPtr_t render::Scene::RBR_SET;


namespace
{
    const char* RTSizesString[] = { "512", "1024", "2048", "4096" };
    const uint32_t RTSizes[] = { 512, 1024, 2048, 4096 };

    render::Scene::TerrainMetadata LoadTerrainMetadata(const std::string& xmlFilePath)
    {
        const int kMetaVersion = 1;

        render::Scene::TerrainMetadata terrainMetadata;
        TiXmlDocument doc(xmlFilePath);
        if (doc.LoadFile())
        {
            const TiXmlElement* element = doc.RootElement();
            terrainMetadata.Version = yaget::conv::Convertor<int>::FromString(element->Attribute("version"));
            if (terrainMetadata.Version == kMetaVersion)
            {
                for (const TiXmlNode* child = element->FirstChild(); child; child = child->NextSibling())
                {
                    if (child->Type() == TiXmlNode::TINYXML_ELEMENT && child->ValueStr() == "SRS")
                    {
                        if (const TiXmlNode* srsNode = child->FirstChild())
                        {
                            std::string srsCoordinates = srsNode->ValueTStr();
                            if (srsCoordinates.find("ENU:") != std::string::npos)
                            {
                                srsCoordinates.replace(0, 4, "");
                            }
                            std::vector<std::string> tokens = conv::Split(srsCoordinates, ",");
                            if (tokens.size() == 2)
                            {
                                terrainMetadata.Latitude = yaget::conv::Convertor<float>::FromString(tokens[0].c_str());
                                terrainMetadata.Longitude = yaget::conv::Convertor<float>::FromString(tokens[1].c_str());
                            }
                        }
                    }
                    else if (child->Type() == TiXmlNode::TINYXML_ELEMENT && child->ValueStr() == "SRSOrigin")
                    {
                        if (const TiXmlNode* srsNode = child->FirstChild())
                        {
                            std::string srsCoordinates = srsNode->ValueTStr();
                            std::vector<std::string> tokens = conv::Split(srsCoordinates, ",");
                            if (tokens.size() == 3)
                            {
                                terrainMetadata.Origin.x = yaget::conv::Convertor<float>::FromString(tokens[0].c_str());
                                terrainMetadata.Origin.y = yaget::conv::Convertor<float>::FromString(tokens[1].c_str());
                                terrainMetadata.Origin.z = yaget::conv::Convertor<float>::FromString(tokens[2].c_str());
                            }
                        }
                    }
                }
            }
            else
            {
                YLOG_WARNING("SCEN", "Metadata: '%s' is not valid. Requires version '%d', file version is: '%d'", xmlFilePath.c_str(), kMetaVersion, terrainMetadata.Version);
            }
        }
        else
        {
            YLOG_WARNING("SCEN", "Could not load '%s' file. Loader Error: '%s'.", xmlFilePath.c_str(), doc.ErrorDesc());
        }

        return terrainMetadata;
    }

} // namespace


struct TransformGlue
{
    static void SetT(const math3d::Matrix& matrix, comp::LocationComponent* component)
    {
        math3d::Vector3 scale;
        math3d::Quaternion quat;
        math3d::Vector3 pos;
        if (const_cast<math3d::Matrix&>(matrix).Decompose(scale, quat, pos))
        {
            component->SetPosition(pos);
            component->SetOrientation(quat);
        }
    }
};


render::Scene::Scene(Device& device) 
    : mDevice(device)
    , mComponentPools()
    , mVideoOptions(device)
{
    // get size of render target from configuration
    for (int i = 0; i < sizeof_array(RTSizes); ++i)
    {
        if (1024 == RTSizes[i])
        {
            mDepthMapSize = i;
            break;
        }
    }

    // -----------------------------------------------------------------------------------
    // setup debug draw support for bullet physics
    const comp::Id_t debugId = idspace::get_burnable(mDevice.App().IdCache);
    Item newDebugtem;

    newDebugtem.mLocationComp = mComponentPools.mLocation.New(debugId);
    newDebugtem.mLineComp = mComponentPools.mLine.New(debugId, mDevice, false);

    mItems.emplace_back(newDebugtem);
    newDebugtem.Participate();
    mComponentPools.mPhysics.SetDebugDraw(new PhysicsDebugDraw(mDevice, *newDebugtem.mLineComp.get()));
    // -----------------------------------------------------------------------------------

    // -----------------------------------------------------------------------------------
    // get first item as a floor, rendered as grid
    const comp::Id_t gridId = idspace::get_burnable(mDevice.App().IdCache);
    Item newGridItem;

    newGridItem.mLocationComp = mComponentPools.mLocation.New(gridId);

    comp::PhysicsComponent::Params params;
    params.collisionShape = new btBoxShape(btVector3(btScalar(8.0), btScalar(0.1), btScalar(8.0)));

    math3d::Matrix matrix = math3d::Matrix::Identity;
    matrix.Translation(math3d::Vector3(0, -2, 0));
    params.matrix = &matrix;
    params.mass = 0.0f;
    params.worldTransformCallback = [&newGridItem](auto&&... params) { newGridItem.mLocationComp->Set(params...); };
    newGridItem.mPhysicsComp = mComponentPools.mPhysics.New(gridId, params);

    //std::shared_ptr<comp::Component> gridAsset = mComponentPools.mGrid.New(gridId, mDevice, *this);
    newGridItem.mGridComp = mComponentPools.mGrid.New(gridId, mDevice);
    mItems.emplace_back(newGridItem);
    newGridItem.Participate();
    // -----------------------------------------------------------------------------------

    // -----------------------------------------------------------------------------------
    // add camera
    mCameraData.mCameraId = idspace::get_burnable(mDevice.App().IdCache);
    Item newItem3;

    newItem3.mLocationComp = mComponentPools.mLocation.New(mCameraData.mCameraId);

    params.collisionShape = new btSphereShape(btScalar(0.25f));

    matrix.Translation(math3d::Vector3(4.5f, 800.0f, 0));
    params.matrix = &matrix;
    params.mass = 0.0f;
    params.worldTransformCallback = [&newItem3](auto&&... params) { newItem3.mLocationComp->Set(params...); };

    newItem3.mPhysicsComp = mComponentPools.mPhysics.New(mCameraData.mCameraId, params);

    newItem3.mCameraComp = mComponentPools.mCamera.New(mCameraData.mCameraId, mDevice, newItem3.mPhysicsComp.get());

    mItems.emplace_back(newItem3);
    newItem3.Participate();
    // -----------------------------------------------------------------------------------

    // -----------------------------------------------------------------------------------
    // render quad as a 2d panel
    bool renderQuad = true;
    if (renderQuad)
    {
        const comp::Id_t itemQuadId = idspace::get_burnable(mDevice.App().IdCache);
        Item newQuadItem;

        newQuadItem.mLocationComp = mComponentPools.mLocation.New(itemQuadId);

        comp::PhysicsComponent::Params paramsQuad = {};
        paramsQuad.collisionShape = new btSphereShape(btScalar(0.25f));

        bool bInitialVisibility = false;
        math3d::Matrix matrixQuad;
        paramsQuad.matrix = &matrixQuad;
        paramsQuad.mass = 0.0f;
        params.worldTransformCallback = [&newQuadItem](auto&&... params) { newQuadItem.mLocationComp->Set(params...); };
        newQuadItem.mPhysicsComp = mComponentPools.mPhysics.New(itemQuadId, paramsQuad);
        newQuadItem.mQuadComp = mComponentPools.mQuad.New(itemQuadId, mDevice, bInitialVisibility);
        // position quad in lower-right corner
        newQuadItem.mQuadComp->SetScreenMatrix(math3d::Matrix::CreateScale(0.5f, 0.5f, 1.0f) * math3d::Matrix::CreateTranslation(0.5f, -0.5f, 0.0f));
        //newItem.mModelComp->SetColor(math3d::Color(Colors::Blue));

        mItems.emplace_back(newQuadItem);
        newQuadItem.Participate();

        Application& app = mDevice.App();
        app.Input().RegisterSimpleActionCallback("Map.View", [this, itemQuadId, bInitialVisibility]()
        {
            comp::ComponentPools& pools = Pools();
            if (render::QuadComponent* quadComponent = pools.mQuad.Find(itemQuadId))
            {
                //static bool bVisible = bInitialVisibility;
                //bVisible = !bVisible;
                //quadComponent->SetValid(bVisible);
            }
        });
    }
    // -----------------------------------------------------------------------------------

    // -----------------------------------------------------------------------------------
    // add terrain component, which manages terrain tiles, etc
    if (true)
    {
        mTerrainId = idspace::get_burnable(mDevice.App().IdCache);
        Item terrainItem;
        terrainItem.mLocationComp = mComponentPools.mLocation.New(mTerrainId);
        terrainItem.mTerrainComp = mComponentPools.mTerrain.New(mTerrainId, mDevice, *this);
        terrainItem.mTerrainComp->ConnectTrigger(TerrainComponent::SignalChunkLoaded, [this](auto&&... params) { OnTerrainChunkLoaded(params...); });
        terrainItem.mLineComp = mComponentPools.mLine.New(mTerrainId, mDevice, false);

        mItems.emplace_back(terrainItem);
        terrainItem.Participate();
    }
    // -----------------------------------------------------------------------------------
    CameraAutoView();

    YLOG_DEBUG("SCEN", "Added new items.");

    Application& app = mDevice.App();

    app.Input().RegisterSimpleActionCallback("Konsole", []()
    {
        YAGET_ASSERT(false, "Fix it!!!");
    });
}

void render::Scene::OnTerrainChunkLoaded(const comp::Component& /*from*/)
{
    CameraAutoView();
}

render::Scene::~Scene()
{
}

void render::Scene::CameraAutoView()
{
    if (render::CameraComponent* cameraComponent = mComponentPools.mCamera.Find(mCameraData.mCameraId))
    {
        render::RenderTarget* renderTarget = mDevice.FindRenderTarget("BackBuffer");
        YAGET_ASSERT(renderTarget, "Calling CameraAutoView without valid 'BackBuffer' render target.");

        if (render::TerrainComponent* terainComponent = mComponentPools.mTerrain.Find(mTerrainId))
        {
            const math::Box terrainBounds = terainComponent->BoundingBox();
            math3d::Vector3 bbSize = terrainBounds.GetSize();

            float d = 0.0f;
            float s = bbSize.x > bbSize.z ? bbSize.x : bbSize.z;
            float aspectRatio = renderTarget->GetAspectRatio();
            if (bbSize.x > bbSize.z)
            {
                d = (s / 2.0f) / aspectRatio / TanD(mCameraData.mFOV / 2.0f);
            }
            else
            {
                d = (s / 2.0f) / TanD(mCameraData.mFOV / 2.0f);
            }

            math3d::Vector3 midPoint = terrainBounds.GetMid();
            midPoint.y = terrainBounds.mMax.y + d;

            cameraComponent->SetView(math3d::Vector3(midPoint.x, midPoint.y, midPoint.z), kPI / 2.0f, 0.0f, 0.0f);
        }
        else
        {
            cameraComponent->SetView(math3d::Vector3(0.0f, 5.0f, 0.0f), kPI / 2.0f, 0.0f, 0.0f);
        }
    }
}

void render::Scene::RunLogic(Application& /*app*/, const time::GameClock& gameClock, metrics::Channel& /*channel*/)
{
    if (mItems.empty())
    {
        return;
    }
    {
        mComponentPools.mPhysics.Tick(gameClock);
        mComponentPools.mCamera.Tick(gameClock);
        mComponentPools.mTerrain.Tick(gameClock);
    }

    // prepare for render by packaging data
    {
        PBPtr_t workingCommandBuffer = mLogicToRenderStager.GetNewStage(BufferStager::Stager::ES_PREPARE, PBR_SET);

        // camera will need to be updated in tick method above and be treated like a component and not special case
        math3d::Vector2 winSize = mDevice.App().GetSurface().Size();
        mCameraData.mViewportRatio = winSize.x / winSize.y;

        if (comp::LocationComponent* locationComponent = mComponentPools.mLocation.Find(mCameraData.mCameraId))
        {
            if (mCameraData.mAutoView)
            {
                CameraAutoView();
            }
            workingCommandBuffer->mViewMatrix = locationComponent->Matrix();
        }

        if (render::TerrainComponent* terainComponent = mComponentPools.mTerrain.Find(mTerrainId))
        {
            workingCommandBuffer->mSceneBoundingBox = terainComponent->BoundingBox();
        }
        else
        {
            workingCommandBuffer->mSceneBoundingBox.mMax = math3d::Vector3::Zero;
            workingCommandBuffer->mSceneBoundingBox.mMin = math3d::Vector3::Zero;
        }

        size_t numComponents = mComponentPools.mLocation.TokenizeState(nullptr, 0);

        workingCommandBuffer->mBuffers.resize(numComponents);
        mComponentPools.mLocation.TokenizeState(reinterpret_cast<char*>(workingCommandBuffer->mBuffers.data()), workingCommandBuffer->mBuffers.size());

        mLogicToRenderStager.SetStage(BufferStager::Stager::ES_PREPARE, workingCommandBuffer);
        mDevice.App().AddTask([this]() { PrepareData(); });
    }
}

void render::Scene::PrepareData()
{
    if (PBPtr_t commandsToExecute = mLogicToRenderStager.GetAndClearStage(BufferStager::Stager::ES_PREPARE, PBR_SET))
    {
        RBPtr_t renderCommandBuffer = mLogicToRenderStager.GetNewStage(BufferStager::Stager::ES_RENDER, RBR_SET);
        renderCommandBuffer->mBuffers.reserve(commandsToExecute->mBuffers.size());
        renderCommandBuffer->mViewMatrix = commandsToExecute->mViewMatrix;
        renderCommandBuffer->mSceneBoundingBox = commandsToExecute->mSceneBoundingBox;

        for (auto&& it: commandsToExecute->mBuffers)
        {
            renderCommandBuffer->mBuffers.emplace_back(RenderBuffer());
            renderCommandBuffer->mBuffers.back().Id = it.mId;
            renderCommandBuffer->mBuffers.back().Matrix = math3d::Matrix::CreateFromQuaternion(it.mOrientation) * math3d::Matrix::CreateTranslation(it.mPosition);
        }

        mLogicToRenderStager.SetStage(BufferStager::Stager::ES_RENDER, renderCommandBuffer);
    }
}

void render::Scene::RunRender(Application& /*app*/, const time::GameClock& gameClock, metrics::Channel& channel)
{
    YAGET_ASSERT(mDepthMapSize >= 0 && mDepthMapSize < sizeof_array(RTSizes), "mDepthMapSize '%d' is out of bounds. Valid range is 0 - %d.", mDepthMapSize, sizeof_array(RTSizes));
    if (1024 != RTSizes[ mDepthMapSize])
    {
        // resize depth map render target
        mDevice.CreateRenderTarget("MapTarget", 1024, 1024);
    }

    if (RBPtr_t commands = mLogicToRenderStager.GetStageValue(BufferStager::Stager::ES_RENDER, RBR_SET))
    {
        //auto backRenderCB = [this, commands](const math3d::Matrix& projectionMatrix)
        //{
        //    math3d::Matrix view = commands->mViewMatrix;
        //    view._43 = commands->mSceneBoundingBox.mMax.y + 5.0f;
        //    math3d::Matrix projection = projectionMatrix;

        //    RenderComponent::RenderOptions renderOptions;
        //    mComponentPools.mTerrain.Render(commands->mBuffers, view, projection, shaderMaterial, &renderOptions);
        //    mComponentPools.mModel.Render(commands->mBuffers, view, projection, shaderMaterial, &renderOptions);
        //};

        auto finalRenderCB = [this, commands](const time::GameClock& /*gameClock*/, metrics::Channel& /*channel*/, const math3d::Matrix& projectionMatrix)
        {
            yaget::gui::Fonter fonter("Consola");
            //bool show_demo_window = true;
            //ImGui::ShowDemoWindow(&show_demo_window);

            GuiPass();

            math3d::Matrix view = commands->mViewMatrix;
            math3d::Matrix projection = projectionMatrix;

            RenderComponent::RenderOptions renderOptions;
            renderOptions.bShowGUI = dev::CurrentConfiguration().mDebug.mFlags.Gui;
            mComponentPools.mGrid.Render(commands->mBuffers, view, projection, &renderOptions);
            mComponentPools.mCamera.Render(commands->mBuffers, view, projection, &renderOptions);
            mComponentPools.mTerrain.Render(commands->mBuffers, view, projection, &renderOptions);
            mComponentPools.mModel.Render(commands->mBuffers, view, projection, &renderOptions);
            mComponentPools.mLine.Render(commands->mBuffers, view, projection, &renderOptions);
            mComponentPools.mQuad.Render(commands->mBuffers, view, projection, &renderOptions);
        };

        //if (render::RenderTarget* renderTarget = mDevice.FindRenderTarget("MapTarget"))
        //{
        //    math3d::Matrix projectionMatrix = CreateOrthographicLH(800, 800, 1, 10000);
        //    if (render::TerrainComponent* terainComponent = mComponentPools.mTerrain.Find(mTerrainId))
        //    {
        //        const math::Box bb = terainComponent->BoundingBox();
        //        math3d::Vector3 bbSize = bb.GetSize();
        //        bbSize.x = std::max(1.0f, bbSize.x);
        //        bbSize.z = std::max(1.0f, bbSize.z);
        //        if (bbSize.z > bbSize.x)
        //        {
        //            // works with tall vertical objects
        //            projectionMatrix = CreateOrthographicLH(bbSize.x * (bbSize.z / bbSize.x), bbSize.z, 1, 10000);
        //        }
        //        else
        //        {
        //            // works with tall horizontal objects
        //            projectionMatrix = CreateOrthographicLH(bbSize.x, bbSize.z * (bbSize.x / bbSize.z), 1, 10000);
        //        }
        //    }
        //
        //    mDevice.RenderFrame(renderTarget, [backRenderCB, projectionMatrix]() { backRenderCB(projectionMatrix); });
        //}

        render::RenderTarget* renderTarget = mDevice.FindRenderTarget("BackBuffer");
        math3d::Matrix projectionMatrix = CreatePerspectiveFieldOfViewLH(DegToRad(mCameraData.mFOV), renderTarget->GetAspectRatio(), mCameraData.mNear, mCameraData.mFar);

        mDevice.RenderFrame(gameClock, channel, renderTarget, [finalRenderCB, projectionMatrix](auto&&... params) { finalRenderCB(params..., projectionMatrix); });
    }
    else
    {
        // we sill want to trigger render frame, things like clearing the backbuffer, responding to resize for example
        mDevice.RenderFrame(gameClock, channel, nullptr, [this](const time::GameClock& /*gameClock*/, metrics::Channel& /*channel*/)
        {
            yaget::gui::Fonter fonter("Consola");

            GuiPass();
        });
    }
}

void render::Scene::BufferStager::SetStage(Stager id, PBPtr_t buffer)
{
    if (id == Stager::ES_PREPARE)
    {
        std::atomic_store(&mPrepareReady, buffer);
    }
    else
    {
        YAGET_ASSERT(false, "Stager id not handled by SetStage.");
    }
}

void render::Scene::BufferStager::SetStage(Stager id, RBPtr_t buffer)
{
    if (id == Stager::ES_RENDER)
    {
        std::atomic_store(&mRenderReady, buffer);
    }
    else
    {
        YAGET_ASSERT(false, "Stager id not handled by SetStage.");
    }
}

render::Scene::PBPtr_t render::Scene::BufferStager::GetAndClearStage(Stager id, PBPtr_t g)
{
    if (id == Stager::ES_PREPARE)
    {
        PBPtr_t value = std::exchange(mPrepareReady, g);
        return value;
    }
    else
    {
        YAGET_ASSERT(false, "Stager id not handled by GetAndClearStage.");
    }

    return nullptr;
}

render::Scene::RBPtr_t render::Scene::BufferStager::GetAndClearStage(Stager id, RBPtr_t g)
{
    if (id == Stager::ES_RENDER)
    {
        RBPtr_t value = std::exchange(mRenderReady, g);
        return value;
    }
    else
    {
        YAGET_ASSERT(false, "Stager id not handled by GetAndClearStage.");
    }
    return nullptr;
}

render::Scene::PBPtr_t render::Scene::BufferStager::GetNewStage(Stager id, PBPtr_t /*g*/) const
{
    if (id == Stager::ES_PREPARE)
    {
        return std::make_shared<PBHead>(); //TODO examine to see if we can just use PBPtr_t over template syntax
    }
    else
    {
        YAGET_ASSERT(false, "Stager id not handled by GetNewStage.");
    }
    return nullptr;
}

render::Scene::RBPtr_t render::Scene::BufferStager::GetNewStage(Stager id, RBPtr_t /*g*/) const
{
    if (id == Stager::ES_RENDER)
    {
        return std::make_shared<RBHead>();
    }
    else
    {
        YAGET_ASSERT(false, "Stager id not handled by GetNewStage.");
    }
    return nullptr;
}

render::Scene::PBPtr_t render::Scene::BufferStager::GetStageValue(Stager id, PBPtr_t g) const
{
    if (id == Stager::ES_PREPARE)
    {
        PBPtr_t value = std::atomic_load(&mPrepareReady);
        return value;
    }
    else
    {
        YAGET_ASSERT(false, "Stager id not handled by GetStageValue.");
    }
    return nullptr;
}

render::Scene::RBPtr_t render::Scene::BufferStager::GetStageValue(Stager id, RBPtr_t g) const
{
    if (id == Stager::ES_RENDER)
    {
        RBPtr_t value = std::atomic_load(&mRenderReady);
        return value;
    }
    else
    {
        YAGET_ASSERT(false, "Stager id not handled by GetStageValue.");
    }
    return nullptr;
}

void render::Scene::GuiPass()
{
    Application& app = mDevice.App();

    static bool demoWindow = false;
    static bool metricsWindow = false;
    static bool styleEditor = false;
    static bool styleSelector = false;
    static bool fontSelector = false;
    static bool userGuide = false;

    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            bool bSaveAs = false;
            if (ImGui::MenuItem("Save As..", "", &bSaveAs))
            {
                if (bSaveAs)
                {
                    mSavingState = Scene::SavingStage::Gray;
                    app.Input().TriggerAction("Screenshot", 0, 0);
                }
            }

            onGuiPass();

            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Demos"))
        {
            ImGui::MenuItem("Demo Window", "", &demoWindow);
            ImGui::MenuItem("Metrics Window", "", &metricsWindow);
            ImGui::MenuItem("Style Editor", "", &styleEditor);
            ImGui::MenuItem("Style Selector", "", &styleSelector);
            ImGui::MenuItem("Font Selector", "", &fontSelector);
            ImGui::MenuItem("User Guide", "", &userGuide);

            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }

    if (demoWindow)
    {
        ImGui::ShowDemoWindow(&demoWindow);
    }
    if (metricsWindow)
    {
        ImGui::ShowMetricsWindow(&metricsWindow);
    }
    if (styleEditor)
    {
        ImGui::ShowStyleEditor();
    }
    if (styleSelector)
    {
        ImGui::ShowStyleSelector("Style Selector");
    }
    if (fontSelector)
    {
        ImGui::ShowFontSelector("Font Selector");
    }
    if (userGuide)
    {
        ImGui::ShowUserGuide();
    }

    mVideoOptions.OnGuiPass();

    if (mSavingState == Scene::SavingStage::Gray)
    {
        render::RenderTarget* renderTarget = mDevice.FindRenderTarget("MapTarget");
        if (renderTarget && !renderTarget->IsScreenshotToTake())
        {
            mRenderTargetDepthMap = false;
            mSavingState = Scene::SavingStage::Color;
            app.Input().TriggerAction("Screenshot", 0, 0);
        }
    }
    else if (mSavingState == Scene::SavingStage::Color)
    {
        render::RenderTarget* renderTarget = mDevice.FindRenderTarget("MapTarget");
        if (renderTarget && !renderTarget->IsScreenshotToTake())
        {
            mRenderTargetDepthMap = true;
            mSavingState = Scene::SavingStage::None;
        }
    }

    if (dev::CurrentConfiguration().mDebug.mFlags.Gui)
    {
        static float kWidgetAlpha = 0.5f;
        ImGui::SetNextWindowBgAlpha(kWidgetAlpha);

        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Yaget Debug Options"))//, nullptr, ImVec2(0, 0), kWidgetAlpha))
        {
            math3d::Vector2 winSize = mDevice.App().GetSurface().Size();

            ImGui::SliderFloat("UI Panels Transparency", &kWidgetAlpha, 0.05f, 1.0f);

            ImGui::Separator();
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 0.25f), "Mouse Position: (%.1f,%.1f)", ImGui::GetIO().MousePos.x, ImGui::GetIO().MousePos.y);

            ImGui::Separator();
            ImGui::Checkbox("Camera Auto View", &mCameraData.mAutoView);
            ImGui::SameLine();
            ImGui::Checkbox("Render Depth Map", &mRenderTargetDepthMap);
            ImGui::SliderInt("Depth Map Size", &mDepthMapSize, 0, 3);
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 0.25f), "Current Size: %sx%s", RTSizesString[mDepthMapSize], RTSizesString[mDepthMapSize]);
        }

        //ImGui::Separator();
        //ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 0.4f), "Camera:");
        //std::string camLoc = conv::Convertor<math3d::Vector3>::ToString(mCameraData.mPosition);
        //ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 0.4f), fmt::format("Loc: {}", camLoc).c_str());
        //ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 0.4f), fmt::format("Ratio: {:.3f} (w/h)", mCameraData.mViewportRatio).c_str());
        //ImGui::SliderFloat("FOV", &mCameraData.mFOV, 1, 120);
        //ImGui::SliderFloat("Near", &mCameraData.mNear, 0.1f, 1000);
        //ImGui::SliderFloat("Far", &mCameraData.mFar, 0.1f, 1000);
        ImGui::End();

        yaget::gui::SnapTo(yaget::gui::Border::Bottom, "Log Output");
        yaget::gui::DrawLogs("Log Output");

        //ImGui::OpenPopup("Quit?");
        //if (ImGui::BeginPopupModal("Quit?", NULL, ImGuiWindowFlags_AlwaysAutoResize))
        //{
        //    ImGui::Text("Are you shure you want to quit?");
        //    ImGui::Separator();
        //    //static bool dont_ask_me_next_time = false;
        //    //ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
        //    //ImGui::Checkbox("Don't ask me next time", &dont_ask_me_next_time);
        //    //ImGui::PopStyleVar();

        //    if (ImGui::Button("OK", ImVec2(120, 0)))
        //    {
        //        ImGui::CloseCurrentPopup();
        //    }
        //    ImGui::SameLine();
        //    if (ImGui::Button("Cancel", ImVec2(120, 0)))
        //    {
        //        ImGui::CloseCurrentPopup();
        //    }
        //    ImGui::EndPopup();
        //}
    }

}

comp::Id_t render::Scene::NewItem(const math3d::Matrix& matrix)
{
    const comp::Id_t itemId = idspace::get_burnable(mDevice.App().IdCache);
    Item newItem;

    newItem.mLocationComp = mComponentPools.mLocation.New(itemId);
    comp::PhysicsComponent::Params params;
    params.collisionShape = new btSphereShape(btScalar(0.25f));

    params.matrix = &matrix;
    params.mass = 0.0f;
    params.worldTransformCallback = [&newItem](auto&&... params) { newItem.mLocationComp->Set(params...); };
    newItem.mPhysicsComp = mComponentPools.mPhysics.New(itemId, params);
    newItem.mLineComp = mComponentPools.mLine.New(itemId, mDevice, false);

    try
    {
        newItem.mModelComp = mComponentPools.mModel.New(itemId, mDevice);
    }
    catch (const ex::bad_init& e)
    {
        YLOG_WARNING("SCEN", "Could not load/initialize asset. %s", e.what());
    }

    mItems.emplace_back(newItem);
    newItem.Participate();

    return itemId;
}
