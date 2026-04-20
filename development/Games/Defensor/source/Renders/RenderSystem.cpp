#include "MemoryManager/NewAllocator.h"
#include "Render/DesktopApplication.h"
#include "Render/Device.h"
#include "Render/Pipeline/ShaderBuffers.h"
#include "Render/Platform/Adapter.h"
#include "Renders/RenderSystem.h"
#include "Render/UI/FontRender.h"

#include <ranges>


namespace
{
    yaget::io::VirtualTransportSystem::Section GetSection(const char* cacheName)
    {
        using Section = yaget::io::VirtualTransportSystem::Section;
#ifdef YAGET_DEBUG
        Section section("Caches@" + std::string(cacheName) + "_Debug");
        section.mMatch = Section::FilterMatch::Exact;
        return section;
#else
        Section section("Caches@" + std::string(cacheName));
        section.mMatch = Section::FilterMatch::Exact;
        return section;
#endif
    }

}


//-------------------------------------------------------------------------------------------------
defensor::render::RenderSystem::RenderSystem(Messaging& messaging, Application& app, RenderCoordinatorSet& coordinatorSet)
    : RenderSystemApp("RenderSystem", messaging, app, [this](auto&&... params) { OnUpdate(params...); }, coordinatorSet, false)
    , mColorInterpolator({ 0.4f, 0.6f, 0.9f, 1.0f }, { 0.6f, 0.9f, 0.4f, 1.0f })
    , mMatrixInterpolator(0.0f, 1.0f)
    , mDependencyGraph(app.VTS(), Section("Manifest@RenderDependencies"), [this](auto guid) { HotRebindMaterial(guid); })
    , mRenderSignatures(GetDevice().GetAdapter().GetDevice(), app.VTS(), GetSection("Signatures"))
    , mRenderPipelines(GetDevice().GetAdapter().GetDevice(), app.VTS(), GetSection("Pipelines"), GetDevice().GetSelectedAdapter().GetSelectedResolution().mDepthStencilFormat)
    , mRenderShaders(app.VTS(), GetSection("Shaders"))
    , mPipelineTags{ app.VTS() }
    , mRenderMaterials(mPipelineTags, app.VTS())
    , mRenderTextures(app.VTS(), GetSection("Textures"))
    , mTextureResources(GetDevice(), mRenderTextures)
    , mShaderBuffers(GetDevice().GetWindowFrame().GetSurface().NumBackBuffers(), GetDevice().GetAdapter(), app.VTS(), GetSection("Constants"), GetDevice().GetQueueFenceValues())
    , mRenderGeometries(GetDevice().GetAdapter().GetDevice(), app.VTS(), GetSection("Geometries"))
    , mGeometryResources(GetDevice(), mRenderGeometries)
    , mRenderTargetStorage(GetDevice().GetAdapter().GetDevice(), GetDevice().GetSwapChain(), mTextureResources, app.VTS())
    , mSceneItemsStorage(mRenderMaterials,
                         mRenderSignatures,
                         mRenderPipelines,
                         mShaderBuffers,
                         mTextureResources,
                         mGeometryResources,
                         app.VTS(), GetSection("SceneItems"))
    , mFontStorage(mRenderGeometries, mGeometryResources, mSceneItemsStorage, app.VTS())
    , mRenderPasses{ app.VTS() }
    , mResizeCallbackId{ GetDevice().RegisterResizeCallback([this](auto&&... params) { OnResetDevice(params...); }) }
{
    mFontTag.mGuid = NewGuid();

    mApp.PoolThread().AddTask([this]()
    {
        PreloadAssets();
    });
}


//-------------------------------------------------------------------------------------------------
colors::Color lerp(const colors::Color& a, const colors::Color& b, float t)
{
    return colors::Color::Lerp(a, b, t);
}


//-------------------------------------------------------------------------------------------------
void defensor::render::RenderSystem::OnUpdate(comp::Id_t id, const time::GameClock& gameClock, metrics::Channel& channel, const SceneComponent* sceneComponent)
{
    using namespace yaget::render;
    using RenderEntity = comp::RowPolicy<RenderComponent*>;
    auto& coordinator = GetCS().GetCoordinator<RenderEntity>();

    if (id == comp::END_ID_MARKER)
    {
        memory::StartRecordAllocations();
        auto fontTag = mApp.VTS().GetTag(Section{ "SceneItems@MediumFont" });

        constexpr float FpsAlpha = 0.5f;
        // let's show frame rate here, using stb library for generating text
        static bool flipper = false;
        mFramesThisSecond++;
        mCurrentCalcTime += gameClock.GetDeltaTimeSecond();
        if (mCurrentCalcTime > 1.0f)
        {
            mAverageFps = FpsAlpha * mAverageFps + (1.0f - FpsAlpha) * mFramesThisSecond;
            auto framePerSecond = std::format("FPS: {}", static_cast<uint32_t>(mAverageFps));

            colors::Color textColor(colors::Red);
            mFontStorage.UpdateText(fontTag, framePerSecond, 10, 10, 3.0f, &textColor, commands::Type::Direct);

            mCurrentCalcTime -= 1.0f;
            mFramesThisSecond = 0;
        }

        const auto& renderPasses = mRenderPasses.GetPasses();
        auto& device = GetDevice();

        for (const auto& renderPass: renderPasses)
        {
            struct ItemToRender
            {
                scene::SceneItem* mItem{};
                math3d::Matrix mWorldViewProj;
                float mTime{};
            };

            std::vector<ItemToRender> itemsToRender;
            auto renderTarget = mRenderTargetStorage.FindRenderTarget(renderPass.mRenderTargetTag);
            auto colorClear = renderPass.GetColorClear(renderTarget);
            auto depthClearValue = renderPass.GetDepthStencilClear(renderTarget);

            auto frameCommands = device.GetFrameCommands(*renderTarget, gameClock, channel);
            auto currentFrameIndex = frameCommands.GetFrameIndex();
            auto commandList = frameCommands.BeginFrame(colorClear, depthClearValue);
            auto viewMatrix = renderPass.GetViewMatrix();
            auto orthoMatrix = renderPass.GetProjectionMatrix();
            auto commandType = commandList->GetType();

            if (renderPass.mSceneItemTags.empty())
            {
                coordinator.ForEach<RenderEntity>([&itemsToRender, &viewMatrix, &orthoMatrix, this](comp::Id_t /*id*/, const auto& row)
                {
                    auto renderComponent = std::get<RenderComponent*>(row);
                    auto sceneItem = mSceneItemsStorage.GetSceneItem(renderComponent->mSceneItemTag);

                    auto worldViewProj = (renderComponent->mMatrix * viewMatrix * orthoMatrix).Transpose();
                    //sceneItem->UpdateData(constant_shader_types::ConstantTypes::WorldViewProjection, worldViewProj);

                    float timeData = 1.0f;
                    //sceneItem->UpdateData(constant_shader_types::ConstantTypes::Time, timeData);
                    itemsToRender.push_back({ .mItem = sceneItem, .mWorldViewProj = worldViewProj, .mTime = timeData });

                    return true;
                });

                auto fontItem = mSceneItemsStorage.GetSceneItem(fontTag);
                const auto& windowSize = GetDevice().GetWindowFrame().GetSurface().GetSize<float>();
                auto projectionMatrix = math3d::Matrix::CreateOrthographicOffCenter(0,  std::get<0>(windowSize), std::get<1>(windowSize), 0, 0, 1);

                auto worldViewProj = projectionMatrix.Transpose();
                //fontItem->UpdateData(constant_shader_types::ConstantTypes::WorldViewProjection, worldViewProj);
                itemsToRender.push_back({ .mItem = fontItem, .mWorldViewProj = worldViewProj, .mTime = 1.0f });
            }
            else
            {
                for (const auto& sceneItemTag : renderPass.mSceneItemTags)
                {
                    auto sceneItem = mSceneItemsStorage.GetSceneItem(sceneItemTag);

                    float timeData = 1.0f;
                    //sceneItem->UpdateData(constant_shader_types::ConstantTypes::Time, timeData);
                    itemsToRender.push_back({ .mItem = sceneItem, .mWorldViewProj = math3d::Matrix::Identity, .mTime = timeData });
                }
            }

            std::ranges::sort(itemsToRender, [](const ItemToRender& item1, const ItemToRender& item2)
            {
                return item1.mItem->GetRenderOrder() < item2.mItem->GetRenderOrder();
            });

            //scene::SceneItemsStorage::SortSceneItems(itemsToRender);
            std::ranges::for_each(itemsToRender, [commandList, currentFrameIndex, commandType](ItemToRender& item)
            {
                item.mItem->UpdateData(currentFrameIndex, constant_shader_types::ConstantTypes::WorldViewProjection, item.mWorldViewProj, commandType);
                item.mItem->UpdateData(currentFrameIndex, constant_shader_types::ConstantTypes::Time, item.mTime, commandType);
                item.mItem->Render(currentFrameIndex, commandList);
            });

            frameCommands.EndFrame();
        }

        memory::StopRecordAllocations();
    }
    else
    {
        const auto& newFrameRenderIds = sceneComponent->GetIds();

        mRenderPasses.BindAsset(sceneComponent->mRenderPassTag);

        coordinator.ForEach<RenderEntity>(newFrameRenderIds, [sceneComponent, &vts = mApp.VTS(), this](comp::Id_t id, const auto& row)
        {
            if (auto data = sceneComponent->FindState(id))
            {
                auto renderComponent = std::get<RenderComponent*>(row);
                renderComponent->UpdateMatrix(math3d::Matrix(data->mMatrix));

                if (renderComponent->mSceneItemTag.mGuid != Guid(data->mAssetGuid))
                {
                    renderComponent->mSceneItemTag = mApp.VTS().FindTag(Guid(data->mAssetGuid));
                }
            }

            return true;
        });
    }
}


//-------------------------------------------------------------------------------------------------
void defensor::render::RenderSystem::PreloadAssets()
{
    // we need to have some kind of manifest file which will enumerate all the files that need to be post process and saved into a cache
    auto& vts = mApp.VTS();

    const Section renderTargetsSection("RenderTargets");
    auto renderTargetsTags = vts.GetTags(renderTargetsSection);
    mRenderTargetStorage.Preload(renderTargetsTags);

    const Section vertexShaderSection("VertexShaders");
    auto vertexShaderTags = vts.GetTags(vertexShaderSection);

    const Section pixelShaderSection("PixelShaders");
    auto pixelShaderTags = vts.GetTags(pixelShaderSection);

    mRenderShaders.Preload(vertexShaderTags, yaget::render::RenderShaders::ShaderType::Vertex);
    mRenderShaders.Preload(pixelShaderTags, yaget::render::RenderShaders::ShaderType::Pixel);

    const Section geometrySection("Geometry");
    auto geometryTags = vts.GetTags(geometrySection);
    mRenderGeometries.Preload(geometryTags);
    mGeometryResources.Preload(geometryTags);

    const Section textureSection("Images");
    auto textureTags = vts.GetTags(textureSection);
    mRenderTextures.Preload(textureTags);
    mTextureResources.Preload(textureTags);

    const Section materialSection("Materials");
    auto materialTags = vts.GetTags(materialSection);

    auto materials = mRenderMaterials.GetMaterials(materialTags);
    for (const auto& [material, matTag] : std::views::zip(materials, materialTags))
    {
        RebindMaterial(matTag, material);
    }
    const Section sceneItemsSection("SceneItems");
    auto sceneItemsTags = vts.GetTags(sceneItemsSection);
    mSceneItemsStorage.Preload(sceneItemsTags);

    platform::Sleep(1, time::kSecondUnit);

    SetTickEnabled(true);
}


//-------------------------------------------------------------------------------------------------
void defensor::render::RenderSystem::RebindMaterial(const io::Tag& matTag, const yaget::render::MaterialPropertyTags& material)
{
    auto& vts = mApp.VTS();

    auto vsTag = material.mVertexShader;
    auto psTag = material.mPixelShader;
    auto sigTag = material.mSignature;
    auto psoTag = material.mPSO;
    auto shaderBufferTag = material.mShaderBuffer;
    if (!vsTag.IsValid() || !psTag.IsValid() || !sigTag.IsValid() || !psoTag.IsValid() || !shaderBufferTag.IsValid())
    {
        YLOG_ERROR("REND", std::format("Material '{}' has invalid material tags. {}'",
                       conv::ToString(matTag),
                       conv::ToString(material)).c_str());
        return;
    }

    ID3D12RootSignature* signature = nullptr;
    mRenderShaders.CreateSignatureDescription(vsTag, psTag, [this, &sigTag, &signature, &shaderBufferTag](const auto& descResult)
    {
        signature = mRenderSignatures.GetSignature(sigTag, descResult);
        mShaderBuffers.MakeBuffers(shaderBufferTag, descResult.mIndexMap);
    });
    AttachTransientAsset(sigTag, vts);
    AttachTransientAsset(shaderBufferTag, vts);

    auto vsBlob = mRenderShaders.GetShader(vsTag, yaget::render::RenderShaders::ShaderType::Vertex);
    auto psBlob = mRenderShaders.GetShader(psTag, yaget::render::RenderShaders::ShaderType::Pixel);

    auto vertexPins = mRenderShaders.GetShaderPins(vsTag);
    auto pixelPins = mRenderShaders.GetShaderPins(psTag);

    /*ID3D12PipelineState* pipeline =*/
    mRenderPipelines.GetPipeline(psoTag, signature, vsBlob, vertexPins, psBlob, pixelPins);
    AttachTransientAsset(psoTag, vts);

    //NOTE(eg) now we need to add this dependencies data:
    //        Material
    //           |
    //        Pipeline
    //           |
    //       Signature --> IndexMap
    //           |
    //      ShaderBuffer
    //        |     |
    //     Vertex Pixel
    //     Shader Shader
    mDependencyGraph.Add(matTag.mGuid, psoTag.mGuid);
    mDependencyGraph.Add(psoTag.mGuid, sigTag.mGuid);
    mDependencyGraph.Add(sigTag.mGuid, shaderBufferTag.mGuid);
    mDependencyGraph.Add(shaderBufferTag.mGuid, vsTag.mGuid);
    mDependencyGraph.Add(shaderBufferTag.mGuid, psTag.mGuid);

    //mDependencyGraph.ClearDirty(matTag.mGuid);
}


//-------------------------------------------------------------------------------------------------
void defensor::render::RenderSystem::HotRebindMaterial(const Guid& guid)
{
    Guid matGuid;
    DependencyNode* matNode = nullptr;
    std::vector<DependencyNode*> pathTo;

    /*DependencyNode *node =*/
    mDependencyGraph.Find(guid, &pathTo);
    if (!pathTo.empty())
    {
        matNode = *pathTo.begin();
        matGuid = matNode->mGuid;
    }
    else
    {
        YLOG_ERROR("REND", std::format("Material '{}' is not found in dependency graph, ignoring material rebind.", conv::ToString(guid)).c_str());
        return;
    }

    auto& vts = mApp.VTS();
    auto matTag = vts.FindTag(matGuid);
    auto oldMaterial = mRenderMaterials.GetMaterial(matTag);

    mRenderMaterials.ClearCache(matTag);

    auto material = mRenderMaterials.GetMaterial(matTag);

    auto oldMaterialText = conv::ToString(oldMaterial);
    auto newMaterialText = conv::ToString(material);
    YLOG_INFO("REND", std::format("Rebinding material '{}':\n\t== Old '{}'\n\n\t== New '{}'", conv::ToString(matTag), oldMaterialText, newMaterialText).c_str());

    auto vsTag = material.mVertexShader;
    mRenderShaders.ClearCache(vsTag);
    auto psTag = material.mPixelShader;
    mRenderShaders.ClearCache(psTag);

    auto sigTag = material.mSignature;
    mRenderSignatures.ClearCache(sigTag);

    auto psoTag = material.mPSO;
    mRenderPipelines.ClearCache(psoTag);

    RebindMaterial(matTag, material);

    matNode->Dirty() = true;
}


//-------------------------------------------------------------------------------------------------
void defensor::render::RenderSystem::OnResetDevice(const app::WindowFrame& windowFrame, yaget::render::DeviceB::ResizeState resizeState)
{
    if (resizeState == yaget::render::DeviceB::ResizeState::Reset)
    {
        mRenderTargetStorage.ResetAll(windowFrame);
        mSceneItemsStorage.ResetAll(windowFrame);
    }
    else if (resizeState == yaget::render::DeviceB::ResizeState::Set)
    {
        const Section renderTargetsSection("RenderTargets");
        auto renderTargetsTags = mApp.VTS().GetTags(renderTargetsSection);
        mRenderTargetStorage.Preload(renderTargetsTags);

        const Section sceneItemsSection("SceneItems");
        auto sceneItemsTags = mApp.VTS().GetTags(sceneItemsSection);
        mSceneItemsStorage.Preload(sceneItemsTags);
    }
}
