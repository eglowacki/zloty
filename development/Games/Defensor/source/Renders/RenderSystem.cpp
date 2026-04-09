#include "MemoryManager/NewAllocator.h"
#include "Render/DesktopApplication.h"
#include "Render/Device.h"
#include "Render/Pipeline/ConstantBuffer.h"
#include "Render/Pipeline/ShaderBuffers.h"
#include "Render/Platform/Adapter.h"
#include "Renders/RenderSystem.h"
#include "Render/Cache/AssetCache.h"

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
    , mRenderMaterials(app.VTS(), GetSection("Materials"))
    , mRenderTextures(app.VTS(), GetSection("Textures"))
    , mTextureResources(GetDevice(), mRenderTextures)
    , mShaderBuffers(GetDevice().GetAdapter(), app.VTS(), GetSection("Constants"))
    , mRenderGeometries(GetDevice().GetAdapter().GetDevice(), app.VTS(), GetSection("Geometries"))
    , mGeometryResources(GetDevice(), mRenderGeometries)
    , mRenderTargetStorage(GetDevice().GetAdapter().GetDevice())
    , mSceneItemsStorage(mRenderMaterials,
                         mRenderSignatures,
                         mRenderPipelines,
                         mShaderBuffers,
                         mTextureResources,
                         mGeometryResources,
                         app.VTS(), GetSection("SceneItems"))
    , mResizeCallbackId{ GetDevice().RegisterResizeCallback([this](auto&&... params) { OnResetDevice(params...); }) }
{
    //io::AttachTransientAsset(mSceneRenderTargetTag, app.VTS());

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
        int passId = 0;
        passId;

        auto& vts = mApp.VTS();
        const colors::Color color = mColorInterpolator.GetValue(gameClock);
        auto& device = GetDevice();

        {
            auto renderTarget = commands::CreateRenderTargetFrom(mSceneRenderTargetTag, device.GetSwapChain(), mRenderTargetStorage);
            // NOTE(eg) Before uncommenting this, we need to make sure that it's cleared from mTextureResources when we resize window!
            //mTextureResources.AttachRenderTarget(mSceneRenderTargetTag, renderTarget);

            auto frameCommands = device.GetFrameCommands(*renderTarget, gameClock, channel);
            auto commandList = frameCommands.BeginFrame(&color);

            coordinator.ForEach<RenderEntity>([commandList, &vts, &gameClock, this](comp::Id_t /*id*/, const auto& row)
            {
                auto renderComponent = std::get<RenderComponent*>(row);
                renderComponent;

                auto sceneItemTag = vts.GetTag(Section{ "SceneItems@CheckerRectangle" });
                auto sceneItem = mSceneItemsStorage.GetSceneItem(sceneItemTag);

                auto matrixInterpolateValue = mMatrixInterpolator.GetValue(gameClock);
                //// this is just test to see if matrix updates get propagated to shader.
                //float matrix[16];
                //std::ranges::fill(matrix, matrixInterpolateValue);
                //auto adjustedMatrix = math3d::Matrix(matrix);
                //sceneItem->UpdateData(constant_shader_types::ConstantTypes::WorldViewProjection, adjustedMatrix);

                float timeData = matrixInterpolateValue;//[4] = { 0.5f, 0.5f, 0.5f, 0.5f };
                sceneItem->UpdateData(constant_shader_types::ConstantTypes::Time, timeData);

                sceneItem->Render(commandList);

                return true;
            });

            frameCommands.EndFrame();
        }

        auto quadScreenMaterialTag = vts.GetTag(Section{ "Materials@ScreenQuadMaterial" });
        auto quadScreenMaterialProperties = mRenderMaterials.GetMaterial(quadScreenMaterialTag);
        if (DependencyNode* quadScreenMaterialNode = mDependencyGraph.Find(quadScreenMaterialTag.mGuid, nullptr))
        {
            auto sceneTexture = mRenderTargetStorage.FindRenderTarget(mSceneRenderTargetTag);

            auto renderTarget = mRenderTargetStorage.AliasRenderTarget(mSwapChainRenderTargetTag, device.GetSwapChain());
            auto frameCommands = device.GetFrameCommands(*renderTarget, gameClock, channel);
            auto commandList = frameCommands.BeginFrame(&color)->GetDeviceCommandList();

            auto signatureTag = TypeToTag(quadScreenMaterialProperties.mSignature, vts);
            auto rootSig = mRenderSignatures.GetSignature(signatureTag);
            commandList->SetGraphicsRootSignature(rootSig);

            auto psoTag = TypeToTag(quadScreenMaterialProperties.mPSO, vts);
            auto pso = mRenderPipelines.GetPipeline(psoTag);
            commandList->SetPipelineState(pso);

            auto constantBufferTag = TypeToTag(quadScreenMaterialProperties.mShaderBuffer, vts);
            auto constantBuffer = mShaderBuffers.GetBuffer(constantBufferTag);

            math3d::Matrix adjustedMatrix = math3d::Matrix::Identity;
            constantBuffer->UpdateData(constant_shader_types::ConstantTypes::WorldViewProjection, adjustedMatrix);
            float timeData = 1.0f;
            constantBuffer->UpdateData(constant_shader_types::ConstantTypes::Time, timeData);
            constantBuffer->UpdateData(constant_shader_types::ConstantTypes::Texture2d, sceneTexture->SRVDescriptorHeap());
            constantBuffer->Bind(commandList);

            auto geometryTag = vts.GetTag(Section{ "Geometry@ScreenQuad" });
            auto geometryResource = mGeometryResources.GetResource(geometryTag);
            RenderShape renderShape{};
            renderShape.Bind(geometryResource);
            renderShape.Render(commandList);

            frameCommands.EndFrame();
        }

        memory::StopRecordAllocations();
    }
    else
    {
        const auto& newFrameRenderIds = sceneComponent->GetIds();

        coordinator.ForEach<RenderEntity>(newFrameRenderIds, [sceneComponent, &vts = mApp.VTS(), this](comp::Id_t id, const auto& row)
        {
            if (auto data = sceneComponent->FindState(id))
            {
                auto renderComponent = std::get<RenderComponent*>(row);
                renderComponent->mMatrix = math3d::Matrix(data->mMatrix);

                if (renderComponent->mRenderMaterial.mAssetTag.mGuid != Guid(data->mAssetGuid))
                {
                    renderComponent->mRenderMaterial.mAssetTag.mGuid = Guid(data->mAssetGuid);
                }
            }

            return true;
        });
    }
}


//-------------------------------------------------------------------------------------------------
void defensor::render::RenderSystem::PreloadAssets()
{
    //{
    //    std::string testMessage = "This is some test data to be compressed and decompressed using zip library.";
    //    auto testBuffer = io::CreateBuffer(testMessage);
    //    auto compressedBuffer = compression::ZipBuffer(io::cast_to_view(testBuffer));

    //    auto decompressedBuffer = compression::UnzipBuffer(io::cast_to_view(compressedBuffer));
    //    std::string resultMessage(reinterpret_cast<const char*>(io::cast_data<uint8_t>(decompressedBuffer)), io::size_data(decompressedBuffer));

    //    int z = 0;
    //    z;
    //}


    // we need to have some kind of manifest file which will enumerate all the files that need to be post process and saved into a cache
    auto& vts = mApp.VTS();

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

    // let's try to load some textures here as a test

    //io::SingleBLobLoader<io::TextureAsset> loader(vts, Section("Images@Red"));
    //auto textureAsset = loader.GetAsset();

    //auto savedImage = image::SaveImage(textureAsset->mBuffer, image::ImageType::PNG);
    //auto result = io::file::SaveFile("c:/Development/zloty/development/Games/Defensor/data/Images/RedSaved.png", savedImage);
    //result;

    //auto imageBuffer = image::GetImage(Section("Images@Red"), vts);
    //auto header = io::cast_data<image::Header>(imageBuffer);
    //io::BufferView pixelData = io::cast_to_view(imageBuffer, sizeof(header));
    //imageBuffer = image::GetImage(Section("Images@Green"), vts);
    //imageBuffer = image::GetImage(Section("Images@Blue"), vts);
    //imageBuffer = image::GetImage(Section("Images@White"), vts);

    platform::Sleep(1, time::kSecondUnit);

    SetTickEnabled(true);
}


//-------------------------------------------------------------------------------------------------
void defensor::render::RenderSystem::RebindMaterial(const io::Tag& matTag, yaget::render::MaterialProperties material)
{
    auto& vts = mApp.VTS();

    auto vsTag = TypeToTag(material.mVertexShader, vts);
    auto psTag = TypeToTag(material.mPixelShader, vts);
    auto sigTag = TypeToTag(material.mSignature, vts);
    auto psoTag = TypeToTag(material.mPSO, vts);
    auto shaderBufferTag = TypeToTag(material.mShaderBuffer, vts);
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

    auto vsTag = TypeToTag(material.mVertexShader, vts);
    mRenderShaders.ClearCache(vsTag);
    auto psTag = TypeToTag(material.mPixelShader, vts);
    mRenderShaders.ClearCache(psTag);

    auto sigTag = TypeToTag(material.mSignature, vts);
    mRenderSignatures.ClearCache(sigTag);

    auto psoTag = TypeToTag(material.mPSO, vts);
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
    }
    else if (resizeState == yaget::render::DeviceB::ResizeState::Set)
    {
        // NOTE(eg) possibly recreate render targets
    }
}
