#include "Render/Pipeline/PipelineContext.h"
#include "GameSystem/Messaging.h"
#include "Render/Platform/Adapter.h"
#include "VTS/ToolVirtualTransportSystem.h"


namespace
{
    //-------------------------------------------------------------------------------------------------
    yaget::io::VirtualTransportSystem::Section GetSection(const char* cacheName, const std::string& stagePrefix)
    {
        using Section = yaget::io::VirtualTransportSystem::Section;

#ifdef YAGET_DEBUG
        const auto debugPrefix = "_Debug";
#else
        const auto debugPrefix = "";
#endif

        const auto stageName = stagePrefix + (stagePrefix.empty() ? "" : "/");
        Section section("Caches@" + stageName + std::string(cacheName) + debugPrefix);
        section.mMatch = Section::FilterMatch::Exact;
        return section;
    }


    //-------------------------------------------------------------------------------------------------
    yaget::io::tool::VirtualTransportSystemReset::Section GetAssetSection(const std::string& sectionName, const std::string& stagePrefix)
    {
        yaget::io::tool::VirtualTransportSystemReset::Section result{};

        if (!stagePrefix.empty())
        {
            auto parameter = std::format("{}@{}", sectionName, stagePrefix);
            result = { parameter };
        }
        else
        {
            result = { sectionName };
        }

        return result;
    }


    //-------------------------------------------------------------------------------------------------
    yaget::io::Tags GetAssetTags(const std::string& sectionName, const yaget::io::VirtualTransportSystem& vts, const std::string& stagePrefix)
    {
        const auto renderTargetsSection = GetAssetSection(sectionName, stagePrefix);
        return vts.GetTags(renderTargetsSection);
    }


    //-------------------------------------------------------------------------------------------------
    ID3D12Device* GetDevice(const yaget::render::DeviceB& device)
    {
        return device.GetAdapter().GetDevice();
    }


    //-------------------------------------------------------------------------------------------------
    yaget::io::Buffer GetUserData(const yaget::render::DeviceB& device)
    {
        return yaget::io::CreateBuffer(device.GetSelectedAdapter().mDriverVersion.Compact());
    }

}


//-------------------------------------------------------------------------------------------------
yaget::render::PipelineContext::PipelineContext(DeviceB& device, io::VirtualTransportSystem& vts, const std::string& stagePrefix, const items::Director& /*director*/, ProgressCallback progressCallback)
    : mDependencyGraph(vts, Section("Manifest@RenderDependencies"), [this](auto guid) { HotRebindItemProperties(guid); })
    , mRenderSignatures{ GetDevice(device), vts, GetSection("Signatures", stagePrefix), GetUserData(device) }
    , mRenderPipelines{ GetDevice(device), vts, GetSection("Pipelines", stagePrefix), GetUserData(device), device.GetSelectedAdapter().GetSelectedResolution().mDepthStencilFormat }
    , mRenderShaders{ vts, GetSection("Shaders", stagePrefix), GetUserData(device) }
    , mPipelineTags{ vts }
    , mRenderMaterials{ mPipelineTags, vts }
    , mRenderTextures{ vts, GetSection("Textures", stagePrefix), GetUserData(device) }
    , mTextureResources{ device, mRenderTextures }
    , mShaderBuffers{ device.GetWindowFrame().GetSurface().NumBackBuffers(), device.GetAdapter(), vts, GetSection("Constants", stagePrefix), device.GetQueueFenceValues() }
    , mRenderGeometries{ GetDevice(device), vts, GetSection("Geometries", stagePrefix), GetUserData(device) }
    , mGeometryResources{ device, mRenderGeometries }
    , mRenderTargetStorage{ GetDevice(device), device.GetSwapChain(), mTextureResources, vts }
    , mSceneItemsStorage{ mRenderMaterials,
                         mRenderSignatures,
                         mRenderPipelines,
                         mShaderBuffers,
                         mTextureResources,
                         mGeometryResources,
                         vts, GetSection("SceneItems", stagePrefix) }
    , mFontStorage{ mRenderGeometries, mGeometryResources, mSceneItemsStorage, vts }
    , mRenderPasses{ vts, device.GetWindowFrame() }
    , mVTS{ vts }
    , mProgressCallback{ progressCallback }
{
}


//-------------------------------------------------------------------------------------------------
void yaget::render::PipelineContext::PreloadAssets(const std::string& stagePrefix)
{
    // we need to have some kind of manifest file which will enumerate all the files that need to be post process and saved into a cache
    auto& vts = mVTS;

    comp::gs::mt::InitCounter counter{ 0 };

    auto renderTargetsTags = GetAssetTags("RenderTargets", vts, stagePrefix);
    auto vertexShaderTags = GetAssetTags("VertexShaders", vts, stagePrefix);
    auto pixelShaderTags = GetAssetTags("PixelShaders", vts, stagePrefix);
    auto geometryTags = GetAssetTags("Geometry", vts, stagePrefix);
    auto textureTags = GetAssetTags("Images", vts, stagePrefix);
    auto materialTags = GetAssetTags("Materials", vts, stagePrefix);
    auto sceneItemsTags = GetAssetTags("SceneItems", vts, stagePrefix);

    auto numAssetsToLoad = renderTargetsTags.size() + vertexShaderTags.size() + pixelShaderTags.size() + (geometryTags.size() * 2) + (textureTags.size() * 2) + materialTags.size() + sceneItemsTags.size();

    constexpr auto sleepTime = 0;
    const std::string formatedSuffix = stagePrefix.empty() ? "" : std::format(" {}", stagePrefix);
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
