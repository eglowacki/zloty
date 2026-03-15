#include "Renders/RenderSystem.h"
#include "Render/DesktopApplication.h"
#include "Render/Device.h"
#include "Render/Platform/Adapter.h"
#include "ImageLoaders/ImageProcessor.h"
#include "App/FileUtilities.h"

#include <ranges>


namespace
{
    yaget::io::Tag TypeToTag(yaget::render::AssetCacheType assetCacheType, yaget::io::VirtualTransportSystem& vts)
    {
        auto section = yaget::render::AssetCache::operator[](assetCacheType);
        if (section.mName.empty())
        {
            return {};
        }

        yaget::io::VirtualTransportSystem::Section querySection = section;
        querySection.mMatch = yaget::io::VirtualTransportSystem::Section::FilterMatch::Exact;
        auto tag = vts.AssureTag(querySection);
        return tag;
    }

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
    , mDependencyGraph(app.VTS(), Section("Manifest@RenderDependencies"), [this](auto guid) {HotRebindMaterial(guid); })
    , mRenderSignatures(GetDevice().GetAdapter().GetDevice(), app.VTS(), GetSection("Signatures"))
    , mRenderPipelines(GetDevice().GetAdapter().GetDevice(), app.VTS(), GetSection("Pipelines"))
    , mRenderShaders(app.VTS(), GetSection("Shaders"))
    , mRenderMaterials(app.VTS(), GetSection("Materials"))
    , mRenderTextures(app.VTS(), GetSection("Textures"))
{
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
    using RenderEntity = comp::RowPolicy<RenderComponent*>;
    auto& coordinator = GetCS().GetCoordinator<RenderEntity>();

    if (id == comp::END_ID_MARKER)
    {
        auto& vts = mApp.VTS();

        const colors::Color color = mColorInterpolator.GetValue(gameClock);
        auto& device = GetDevice();
        auto framerHandle = device.GetFramerHandle(gameClock, channel, &color);
        auto commandList = framerHandle.GetCommandList();

        coordinator.ForEach<RenderEntity>([commandList, &vts, &gameClock, this](comp::Id_t /*id*/, const auto& row)
            {
                auto renderComponent = std::get<RenderComponent*>(row);
                const auto location = renderComponent->mMatrix;

                auto& material = renderComponent->mRenderMaterial;

                if (DependencyNode* materialNode = mDependencyGraph.Find(material.mAssetTag.mGuid, nullptr))
                {
                    if (materialNode->IsBranchDirty())
                    {
                        material.ResolveAssetTag(material.mAssetTag);
                        materialNode->ClearDirty();
                    }

                    auto signatureTag = TypeToTag(material.mMaterialProperties.mSignature, vts);
                    auto rootSig = mRenderSignatures.GetSignature(signatureTag);
                    commandList->SetGraphicsRootSignature(rootSig);

                    auto psoTag = TypeToTag(material.mMaterialProperties.mPSO, vts);
                    auto pso = mRenderPipelines.GetPipeline(psoTag);
                    commandList->SetPipelineState(pso);

                    float matrix[16];
                    math3d::GetMatrixAsFloats(location, matrix);
                    std::ranges::fill(matrix, mMatrixInterpolator.GetValue(gameClock));

                    commandList->SetGraphicsRoot32BitConstants(0, 16, matrix, 0);
                    renderComponent->Render(commandList);
                }

                return true;
            });
    }
    else
    {
        const auto& newFrameRenderIds = sceneComponent->GetIds();

        coordinator.ForEach<RenderEntity>(newFrameRenderIds, [sceneComponent, &vts = mApp.VTS()](comp::Id_t id, const auto& row)
            {
                if (auto data = sceneComponent->FindState(id))
                {
                    auto renderComponent = std::get<RenderComponent*>(row);
                    renderComponent->mMatrix = math3d::Matrix(data->mMatrix);

                    if (renderComponent->mRenderMaterial.mAssetTag.mGuid != Guid(data->mAssetGuid))
                    {
                        // we need to update material for this render component
                        renderComponent->mRenderMaterial.ResolveAssetTag(vts.FindTag(Guid(data->mAssetGuid)));
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
    const Section vertexShaderSection("VertexShaders");
    auto vertexShaderTags = vts.GetTags(vertexShaderSection);

    const Section pixelShaderSection("PixelShaders");
    auto pixelShaderTags = vts.GetTags(pixelShaderSection);

    mRenderShaders.GetShaders(vertexShaderTags, yaget::render::RenderShaders::ShaderType::Vertex);
    mRenderShaders.GetShaders(pixelShaderTags, yaget::render::RenderShaders::ShaderType::Pixel);

    const Section textureSection("Images");
    auto textureTags = vts.GetTags(textureSection);
    mRenderTextures.GetTextures(textureTags);

    const Section materialSection("Materials");
    auto materialTags = vts.GetTags(materialSection);

    auto materials = mRenderMaterials.GetMaterials(materialTags);
    for (const auto& [material, matTag] : std::views::zip(materials, materialTags))
    {
        RebindMaterial(matTag, material);
    }

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
    if (!vsTag.IsValid() || !psTag.IsValid() || !sigTag.IsValid() || !psoTag.IsValid())
    {
        YLOG_ERROR("REND", std::format("Material '{}' has invalid material tags. {}'",
            conv::ToString(matTag),
            conv::ToString(material)).c_str());
        return;
    }

    ID3D12RootSignature* signature = nullptr;
    mRenderShaders.CreateSignatureDescription(vsTag, psTag, [this, &sigTag, &signature](const auto& descResult)
        {
            signature = mRenderSignatures.GetSignature(sigTag, descResult);
        });
    AttachTransientAsset(sigTag, vts);

    auto vsBlob = mRenderShaders.GetShader(vsTag, yaget::render::RenderShaders::ShaderType::Vertex);
    auto psBlob = mRenderShaders.GetShader(psTag, yaget::render::RenderShaders::ShaderType::Pixel);

    /*ID3D12PipelineState* pipeline =*/ mRenderPipelines.GetPipeline(psoTag, signature, vsBlob, psBlob);
    AttachTransientAsset(psoTag, vts);

    //NOTE(eg) now we need to add this dependencies data:
    //        Material
    //           |
    //        Pipeline
    //           |
    //       Signature
    //        |     |
    //     Vertex Pixel
    //     Shader Shader
    mDependencyGraph.Add(matTag.mGuid, psoTag.mGuid);
    mDependencyGraph.Add(psoTag.mGuid, sigTag.mGuid);
    mDependencyGraph.Add(sigTag.mGuid, vsTag.mGuid);
    mDependencyGraph.Add(sigTag.mGuid, psTag.mGuid);

    //mDependencyGraph.ClearDirty(matTag.mGuid);
}


//-------------------------------------------------------------------------------------------------
void defensor::render::RenderSystem::HotRebindMaterial(const Guid& guid)
{
    Guid matGuid;
    DependencyNode* matNode = nullptr;
    std::vector<DependencyNode*> pathTo;

    /*DependencyNode *node =*/ mDependencyGraph.Find(guid, &pathTo);
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
