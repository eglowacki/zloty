#include "Render/Pipeline/PipelineContext.h"
#include "GameSystem/Messaging.h"
#include "Render/Platform/Adapter.h"
#include "VTS/ToolVirtualTransportSystem.h"


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

    yaget::io::tool::VirtualTransportSystemReset::Section GetAssetSection(const std::string& sectionName, const std::string& sectionSuffix)
    {
        yaget::io::tool::VirtualTransportSystemReset::Section result{};

        if (!sectionSuffix.empty())
        {
            auto parameter = std::format("{}@{}", sectionName, sectionSuffix);
            result = { parameter };
        }
        else
        {
            result = { sectionName };
        }

        return result;
    }

}


//-------------------------------------------------------------------------------------------------
yaget::render::PipelineContext::PipelineContext(DeviceB& device, io::VirtualTransportSystem& vts, ProgressCallback progressCallback)
    : mDependencyGraph(vts, Section("Manifest@RenderDependencies"), [this](auto guid) { HotRebindItemProperties(guid); })
    , mRenderSignatures{ device.GetAdapter().GetDevice(), vts, GetSection("Signatures") }
    , mRenderPipelines{ device.GetAdapter().GetDevice(), vts, GetSection("Pipelines"), device.GetSelectedAdapter().GetSelectedResolution().mDepthStencilFormat }
    , mRenderShaders{ vts, GetSection("Shaders") }
    , mPipelineTags{ vts }
    , mRenderMaterials{ mPipelineTags, vts }
    , mRenderTextures{ vts, GetSection("Textures") }
    , mTextureResources{ device, mRenderTextures }
    , mShaderBuffers{ device.GetWindowFrame().GetSurface().NumBackBuffers(), device.GetAdapter(), vts, GetSection("Constants"), device.GetQueueFenceValues() }
    , mRenderGeometries{ device.GetAdapter().GetDevice(), vts, GetSection("Geometries") }
    , mGeometryResources{ device, mRenderGeometries }
    , mRenderTargetStorage{ device.GetAdapter().GetDevice(), device.GetSwapChain(), mTextureResources, vts }
    , mSceneItemsStorage{ mRenderMaterials,
                         mRenderSignatures,
                         mRenderPipelines,
                         mShaderBuffers,
                         mTextureResources,
                         mGeometryResources,
                         vts, GetSection("SceneItems") }
    , mFontStorage{ mRenderGeometries, mGeometryResources, mSceneItemsStorage, vts }
    , mRenderPasses{ vts, device.GetWindowFrame() }
    , mVTS{ vts }
    , mProgressCallback{ progressCallback }
{
}


//-------------------------------------------------------------------------------------------------
void yaget::render::PipelineContext::PreloadAssets(const std::string& sectionSuffix)
{
    // we need to have some kind of manifest file which will enumerate all the files that need to be post process and saved into a cache
    auto& vts = mVTS;

    comp::gs::mt::InitCounter counter{ 0 };

    const Section renderTargetsSection = GetAssetSection("RenderTargets", sectionSuffix);
    auto renderTargetsTags = vts.GetTags(renderTargetsSection);

    const Section vertexShaderSection = GetAssetSection("VertexShaders", sectionSuffix);
    auto vertexShaderTags = vts.GetTags(vertexShaderSection);

    const Section pixelShaderSection = GetAssetSection("PixelShaders", sectionSuffix);
    auto pixelShaderTags = vts.GetTags(pixelShaderSection);

    const Section geometrySection = GetAssetSection("Geometry", sectionSuffix);
    auto geometryTags = vts.GetTags(geometrySection);

    const Section textureSection = GetAssetSection("Images", sectionSuffix);
    auto textureTags = vts.GetTags(textureSection);

    const Section materialSection = GetAssetSection("Materials", sectionSuffix);
    auto materialTags = vts.GetTags(materialSection);

    const Section sceneItemsSection = GetAssetSection("SceneItems", sectionSuffix);
    auto sceneItemsTags = vts.GetTags(sceneItemsSection);

    auto numAssetsToLoad = renderTargetsTags.size() + vertexShaderTags.size() + pixelShaderTags.size() + (geometryTags.size() * 2) + (textureTags.size() * 2) + materialTags.size() + sceneItemsTags.size();

    constexpr auto sleepTime = 0;
    const std::string formatedSuffix = sectionSuffix.empty() ? "" : std::format(" {}", sectionSuffix);
    //---------------------------------------------------------------------------------
    mProgressCallback(comp::gs::InitEvent{ .mNumItems = static_cast<int32_t>(numAssetsToLoad), .mItemsProcessed = &counter, .mText = std::format("Preloading {}{} Render Targets...", renderTargetsTags.size(), formatedSuffix) });
    mRenderTargetStorage.Preload(renderTargetsTags, counter);
    yaget::platform::Sleep(sleepTime, time::kSecondUnit);

    //if (mApplicationQuiting)
    //{
    //    return;
    //}
    mProgressCallback(comp::gs::InitEvent{ .mNumItems = static_cast<int32_t>(numAssetsToLoad), .mItemsProcessed = &counter, .mText = std::format("Preloading {}{} Shaders...", vertexShaderTags.size() + pixelShaderTags.size(), formatedSuffix) });
    mRenderShaders.Preload(vertexShaderTags, yaget::render::RenderShaders::ShaderType::Vertex, counter);
    mRenderShaders.Preload(pixelShaderTags, yaget::render::RenderShaders::ShaderType::Pixel, counter);
    yaget::platform::Sleep(sleepTime, time::kSecondUnit);

    //if (mApplicationQuiting)
    //{
    //    return;
    //}
    mProgressCallback(comp::gs::InitEvent{ .mNumItems = static_cast<int32_t>(numAssetsToLoad), .mItemsProcessed = &counter, .mText = std::format("Preloading {}{} Geometries...", geometryTags.size() * 2, formatedSuffix) });
    mRenderGeometries.Preload(geometryTags, counter);
    mGeometryResources.Preload(geometryTags, counter);
    yaget::platform::Sleep(sleepTime, time::kSecondUnit);

    //if (mApplicationQuiting)
    //{
    //    return;
    //}
    mProgressCallback(comp::gs::InitEvent{ .mNumItems = static_cast<int32_t>(numAssetsToLoad), .mItemsProcessed = &counter, .mText = std::format("Preloading {}{} Textures...", textureTags.size() * 2, formatedSuffix) });
    mRenderTextures.Preload(textureTags, counter);
    mTextureResources.Preload(textureTags, counter);
    yaget::platform::Sleep(sleepTime, time::kSecondUnit);

    //if (mApplicationQuiting)
    //{
    //    return;
    //}
    mProgressCallback(comp::gs::InitEvent{ .mNumItems = static_cast<int32_t>(numAssetsToLoad), .mItemsProcessed = &counter, .mText = std::format("Preloading {}{} Materials...", materialTags.size(), formatedSuffix) });
    auto materials = mRenderMaterials.GetMaterials(materialTags);
    for (const auto& [material, matTag] : std::views::zip(materials, materialTags))
    {
        RebindMaterial(matTag, material);
        ++counter;
    }
    yaget::platform::Sleep(sleepTime, time::kSecondUnit);

    //if (mApplicationQuiting)
    //{
    //    return;
    //}
    mProgressCallback(comp::gs::InitEvent{ .mNumItems = static_cast<int32_t>(numAssetsToLoad), .mItemsProcessed = &counter, .mText = std::format("Preloading {}{} Items...", sceneItemsTags.size(), formatedSuffix) });
    mSceneItemsStorage.Preload(sceneItemsTags, counter);
    yaget::platform::Sleep(sleepTime, time::kSecondUnit);

    //if (mApplicationQuiting)
    //{
    //    return;
    //}
    //---------------------------------------------------------------------------------
    yaget::platform::Sleep(sleepTime, time::kSecondUnit);

    //mMessaging.Dispatch(items::StageEvent{ "Main Menu", items::db_stage::BlendOp::Replace }, Messaging::DispatcherType::Logic);
    mProgressCallback(comp::gs::InitEvent{ .mNumItems = static_cast<int32_t>(numAssetsToLoad), .mItemsProcessed = nullptr, .mText = std::format("Finished{} Preloading", formatedSuffix) });

    //SetTickEnabled(true);
}


//-------------------------------------------------------------------------------------------------
void yaget::render::PipelineContext::OnResetDevice(const app::WindowFrame& windowFrame, yaget::render::DeviceB::ResizeState resizeState)
{
    using Section = yaget::io::VirtualTransportSystem::Section;

    if (resizeState == DeviceB::ResizeState::Reset)
    {
        mRenderTargetStorage.ResetAll(windowFrame);
        mSceneItemsStorage.ResetAll(windowFrame);
    }
    else if (resizeState == DeviceB::ResizeState::Set)
    {
        comp::gs::mt::InitCounter counter{ 0 };

        const Section renderTargetsSection("RenderTargets");
        auto renderTargetsTags = mVTS.GetTags(renderTargetsSection);
        mRenderTargetStorage.Preload(renderTargetsTags, counter);

        const Section sceneItemsSection("SceneItems");
        auto sceneItemsTags = mVTS.GetTags(sceneItemsSection);
        mSceneItemsStorage.Preload(sceneItemsTags, counter);
    }
}


//-------------------------------------------------------------------------------------------------
yaget::render::commands::RenderTarget* yaget::render::PipelineContext::FindRenderTarget(const io::Tag& tag) const
{
    auto renderTarget = mRenderTargetStorage.FindRenderTarget(tag);
    return renderTarget;
}


//-------------------------------------------------------------------------------------------------
yaget::render::scene::SceneItem* yaget::render::PipelineContext::GetSceneItem(const io::Tag& tag)
{
    auto sceneItem = mSceneItemsStorage.GetSceneItem(tag);
    return sceneItem;
}


//-------------------------------------------------------------------------------------------------
void yaget::render::PipelineContext::UpdateText(const io::Tag& tag, const ui::TextPrinters& textPrinters, commands::Type commandType)
{
    mFontStorage.UpdateText(tag, textPrinters, commandType);
}


//-------------------------------------------------------------------------------------------------
void yaget::render::PipelineContext::RebindMaterial(const io::Tag& matTag, const yaget::render::MaterialPropertyTags& material)
{
    auto& vts = mVTS;

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
void yaget::render::PipelineContext::HotRebindItemProperties(const Guid& /*guid*/)
{
#if 0 // This is driven by DependencyGraph class
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
#endif
}
