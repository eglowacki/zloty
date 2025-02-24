#include "Ponger/PongerRenderer.h"
#include "Components/LocationComponent.h"
#include "Components/GeometryComponent.h"
#include "Components/TextComponent.h"
#include "RenderTarget.h"
#include "Metrics/Concurrency.h"
#include "Device.h"
#include "App/Application.h"
#include "YagetVersion.h"
#include "RenderMathFacade.h"
#include "imgui.h"
#include "Gui/Support.h"

namespace
{
    const char* kRenderTargetName = "BackBuffer";

} // namespace


ponger::PongerRenderer::PongerRenderer(yaget::render::Device& device)
    : ponger::SceneChunkCollector()
    , mDevice(device)
    , mLineCollectorSystem("LineCollector", [this](auto&&... /*prams*/) {})
    , mLineRendererSystem("LineRenderer", [this](auto&&... /*prams*/) {})
    , mRenderSystemCoordinator(nullptr, &mLineCollectorSystem, &mLineRendererSystem)
    , mActivePasses({ { "Default", true }, { "WireFrame", false } })
{}


ponger::PongerRenderer::~PongerRenderer()
{
    mRenderEntityCoordinator.RemoveItems(mIds);
    mGlobalRenderEntityCoordinator.RemoveComponents(mGlobalLineId);
}


void ponger::PongerRenderer::GatherLocation(yaget::comp::Id_t id, const yaget::time::GameClock& gameClock, yaget::metrics::Channel& /*channel*/, yaget::comp::LocationComponent* location, ponger::DebugComponent* debugComponenet)
{
    using namespace yaget;

    if (IsEndMarker(id, gameClock))
    {
        return;
    }

    size_t locationHash = location->GetStateHash();
    size_t debugHash = debugComponenet->GetStateHash();

    size_t currentHash = yaget::conv::GenerateHash(locationHash, debugHash);

    if (UpdateHash(id, std::type_index(typeid(comp::LocationComponent)), currentHash))
    {
        mPayload->mLocations.push_back(comp::LocationChunk{ id, location->GetPosition(), location->GetOrientation(), location->GetScale(), debugComponenet->GetVisualTag(), debugComponenet->GetColor() });
    }
    else
    {
        mPayload->mActiveIds.insert(id);
    }

    mPayload->mDebugIds.insert(id);
}


void ponger::PongerRenderer::Render(const yaget::time::GameClock& /*gameClock*/, yaget::metrics::Channel& /*channel*/)
{
    using namespace yaget;

    if (mGlobalLineId == comp::INVALID_ID)
    {
        InitializeGlobalComponents();
    }

    if (auto payload = PayloadStager().GetPayload())
    {
        // we go some stuff to render
        ProcessPayload(payload);
    }

    // now we are ready to render our entities that are in mIds
    render::RenderTarget* renderTarget = mDevice.FindRenderTarget(kRenderTargetName);
    YAGET_ASSERT(renderTarget, "Render Target of type: '%s' does not exist.", kRenderTargetName);
    comp::Component::UpdateGuiType updateGuiType = comp::Component::UpdateGuiType::Default;
    if (dev::CurrentConfiguration().mDebug.mFlags.Gui && updateGuiType != comp::Component::UpdateGuiType::None)
    {
        renderTarget->AddProcessor(render::RenderTarget::ProcessorType::PostFrame, [this, updateGuiType]() { onProcessGui(updateGuiType); });
    }

    // update global line component with current set of lines to render
    render::LineComponent* globalLineComponenet = mGlobalRenderEntityCoordinator.FindComponent<render::LineComponent>(mGlobalLineId);
    render::TextComponent* globalTextComponenet = mGlobalRenderEntityCoordinator.FindComponent<render::TextComponent>(mGlobalLineId);

    globalTextComponenet->ClearText();
    std::string appFolder = yaget::util::ExpendEnv("$(AppFolder)", nullptr);
    std::string configurationFolder = yaget::util::ExpendEnv("$(ConfigurationFolder)", nullptr);
    appFolder = conv::ReplaceString(appFolder, configurationFolder, "");

    auto winSize = renderTarget->Size<float>();
    std::string versionText = fmt::format("Yaget: {}, Path: {}", yaget::ToString(yaget::YagetVersion), appFolder);
    auto textSize = globalTextComponenet->MeasureString("Consolas_24", versionText);
    globalTextComponenet->SetText("Consolas_24", versionText, 2, winSize.second - textSize.y, colors::LightGray);

    render::LineComponent::LinesPayload renderableLines = globalLineComponenet->GetNewPayloadLines();

    // this pass collects all requested lines to render into one buffer,
    // and renders all GeometryComponents
    //mat3d::Matrix orthoMatrix = math3d::Matrix::CreateOrthographic(4.0f, 4.0f, 0.0f, 1.0f);
    // NOTE: we want to be able to set view matrix and projection matrix here
    // how??? since we need constant based on shader and we don't have shader
    // do we want to make Predefined dummy vertex shader with just constant inputs
    // 
    //math3d::Matrix projectionMatrix = CreatePerspectiveFieldOfViewLH(DegToRad(mCameraData.mFOV), renderTarget->GetAspectRatio(), mCameraData.mNear, mCameraData.mFar);
    //mDevice.UpdateConstant("PerFrame", "projectionMatrix", projectionMatrix);
    //mDevice.UpdateConstant("PerFrame", "viewMatrix", viewMatrix);

    Strings activePasses;
    for (const auto& passInfo : mActivePasses)
    {
        if (passInfo.mActive)
        {
            activePasses.push_back(passInfo.mName);
        }
    }

    mRenderEntityCoordinator.ForEach<RenderEntity>(mIds, [&activePasses, renderableLines = renderableLines.get(), renderTarget](yaget::comp::Id_t /*id*/, const auto drawComp)
    {
        auto [debugDrawComponent, geometryComponent] = drawComp;
        (*renderableLines).insert(std::end(*renderableLines), std::begin(debugDrawComponent->mLines), std::end(debugDrawComponent->mLines));

        geometryComponent->SetPasses(activePasses);
        geometryComponent->Render(renderTarget, math3d::Matrix::Identity);

        return true;
    });

    globalLineComponenet->Draw(renderableLines);


    // render our passes
    // for now all we have is Lines
    using MainRenderables = comp::RowPolicy<render::LineComponent*, render::TextComponent*>;
    mGlobalRenderEntityCoordinator.ForEach<MainRenderables>([renderTarget](yaget::comp::Id_t /*id*/, const auto comps)
    {
        auto[lineComponent, textComponent] = comps;

        lineComponent->Render(renderTarget, math3d::Matrix::Identity);

        textComponent->Render(renderTarget, math3d::Matrix::Identity);

        return true;
    });

}

void ponger::PongerRenderer::onProcessGui(yaget::comp::Component::UpdateGuiType updateGuiType)
{
    static bool geometryWindow = false;
    static bool optionsWindow = false;
    static bool colorSchemesWindow = false;

    yaget::gui::Fonter fonter("Consola");
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("Renderer"))
        {
            ImGui::MenuItem("Geometry Window", "", &geometryWindow);
            ImGui::MenuItem("Options Window", "", &optionsWindow);
            ImGui::MenuItem("Color Schemes Window", "", &colorSchemesWindow);

            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }

    if (geometryWindow)
    {
        ImGui::Begin("Geometry Resources", &geometryWindow);

        using GeometryEntity = comp::RowPolicy<render::GeometryComponent*>;
        mRenderEntityCoordinator.ForEach<GeometryEntity>(mIds, [updateGuiType](yaget::comp::Id_t /*id*/, const auto comps)
        {
            auto [geometryComponent] = comps;
            geometryComponent->UpdateGui(updateGuiType);

            return true;
        });

        ImGui::End();
    }

    if (optionsWindow)
    {
        ImGui::Begin("Render Options", &optionsWindow);

        int counter = 1;
        for (auto& passInfo : mActivePasses)
        {
            ImGui::Checkbox(fmt::format("##{}", counter++).c_str(), &passInfo.mActive);
            ImGui::SameLine();
            ImGui::Text(passInfo.mName.c_str());
        }

        ImGui::End();
    }

    if (colorSchemesWindow)
    {
        ImGui::Begin("Color Schemes", &colorSchemesWindow);

        auto colors = dev::CurrentConfiguration().mGuiColors;
        for (auto& it : colors)
        {
            yaget::gui::ColorEdit4(fmt::format("{} Text", it.first), it.second);
        }
        dev::CurrentConfiguration().Refresh(colors);

        ImGui::End();
    }
}


void ponger::PongerRenderer::ProcessPayload(typename const Stager::ConstPayload& payload)
{
    for (const auto& it : payload->mLocations)
    {
        render::GeometryComponent* geometryComponent = nullptr;

        if (mIds.find(it.mId) == mIds.end())
        {
            metrics::Channel channel("ProcessPayload");

            YAGET_ASSERT(!mRenderEntityCoordinator.FindComponent<render::GeometryComponent>(it.mId), "mIds list does not have id: '%d' but Render Entity Coordinator has component.");

            geometryComponent = mRenderEntityCoordinator.AddComponent<render::GeometryComponent>(it.mId, mDevice, std::vector<io::Tag>{ it.mTag });

            /*ponger::DebugDrawComponent* drawComponent = */mRenderEntityCoordinator.AddComponent<ponger::DebugDrawComponent>(it.mId);
            //render::primitives::Rectangle({-1, 1, 0}, {1, -1, 0}, colors::Yellow, drawComponent->mLines);

            mIds.insert(it.mId);
            YLOG_INFO("PONG", "Added New Renderable Entity: '%d'.", it.mId);
        }
        else
        {
            geometryComponent = mRenderEntityCoordinator.FindComponent<render::GeometryComponent>(it.mId);
        }

        YAGET_ASSERT(geometryComponent, "GeometryComponent for Entity Id: '%d' is nullptr.", it.mId);

        geometryComponent->SetMatrix(math3d::CreateMatrix(it.mPosition, it.mOrientation, it.mScale));
        geometryComponent->SetColor(it.mColor);
    }
}


void ponger::PongerRenderer::InitializeGlobalComponents()
{
    YAGET_ASSERT(mGlobalLineId == comp::INVALID_ID, "Global Line Componenent is already initialized.");

    mGlobalLineId = idspace::get_burnable(mDevice.App().IdCache);
    mGlobalRenderEntityCoordinator.AddComponent<render::LineComponent>(mGlobalLineId, mDevice, true);

    io::Tag fontTag = mDevice.App().VTS().GetTag({"FontBitmaps@Consolas_24"});
    /*auto textComponent =*/ mGlobalRenderEntityCoordinator.AddComponent<render::TextComponent>(mGlobalLineId, mDevice, io::Tags{ fontTag });
    //std::string appFolder = yaget::util::ExpendEnv("$(AppFolder)", nullptr);
    //std::string configurationFolder = yaget::util::ExpendEnv("$(ConfigurationFolder)", nullptr);
    //appFolder = conv::ReplaceString(appFolder, configurationFolder, "");

    //std::string versionText = fmt::format("Yaget: {}, Path: {}", yaget::ToString(yaget::YagetVersion), appFolder);
    //textComponent->SetText("Consolas_24", versionText, 10, 10, colors::Red);
}
