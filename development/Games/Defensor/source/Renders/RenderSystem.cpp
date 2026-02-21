#include "Renders/RenderSystem.h"
#include "Render/DesktopApplication.h"
#include "Render/Device.h"
#include "Render/Platform/Adapter.h"

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

        auto tag = vts.AssureTag(section);
        return tag;
    }
}


//-------------------------------------------------------------------------------------------------
defensor::render::RenderSystem::RenderSystem(Messaging& messaging, Application& app, RenderCoordinatorSet& coordinatorSet)
    : RenderSystemApp("RenderSystem", messaging, app, [this](auto&&... params) { OnUpdate(params...); }, coordinatorSet)
    , mAssetPoolThread("PreloadRenderAssets", 1)
    , mColorInterpolator({ 0.4f, 0.6f, 0.9f, 1.0f }, { 0.6f, 0.9f, 0.4f, 1.0f })
    , mMatrixInterpolator(0.0f, 1.0f)
    , mDependencyGraph(app.VTS(), yaget::io::VirtualTransportSystem::Section("Manifest@RenderDependencies"), [this](auto guid) {HotRebindMaterial(guid);})
    , mRenderSignatures(GetDevice().GetAdapter().GetDevice(), app.VTS())
    , mRenderPipelines(GetDevice().GetAdapter().GetDevice(), app.VTS())
    , mRenderShaders(app.VTS())
    , mRenderMaterials(app.VTS())
{
    mAssetPoolThread.AddTask([this]()
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
    if (!mAssetsPreloaded)
    {
        return;
    }

    using RenderEntity = comp::RowPolicy<RenderComponent*>;
    auto& coordinator = GetCS().GetCoordinator<RenderEntity>();

    if (id == comp::END_ID_MARKER)
    {
        auto& vts = mApp.VTS();

        const colors::Color color = mColorInterpolator.GetValue(gameClock);
        auto& device = GetDevice();
        auto framerHandle = device.GetFramerHandle(gameClock, channel, &color);
        auto commandList = framerHandle.GetCommandList();

        coordinator.ForEach<RenderEntity>([commandList, &vts, &gameClock, color, this](comp::Id_t /*id*/, const auto& row)
        {
            auto renderComponent = std::get<RenderComponent*>(row);
            const auto location = renderComponent->mMatrix;

            auto& material = renderComponent->mRenderMaterial;

            if (DependencyNode* materialNode = mDependencyGraph.Find(material.mAssetTag.mGuid, nullptr))
            {
                if (materialNode->IsBranchDirty())
                {
                    YLOG_ERROR("REND", "material is dirty, we are not handling!!!");
                    return true;
                }
                else
                {
                    auto signatureTag = TypeToTag(material.mAssetTypes.mSignature, vts);
                    auto rootSig = mRenderSignatures.GetSignature(signatureTag);
                    commandList->SetGraphicsRootSignature(rootSig);

                    auto psoTag = TypeToTag(material.mAssetTypes.mPSO, vts);
                    auto pso = mRenderPipelines.GetPipeline(psoTag);
                    commandList->SetPipelineState(pso);
                }

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

    void AttachTransientAsset(const yaget::io::Tag& tag, yaget::io::VirtualTransportSystem& vts)
    {
        using namespace yaget;

        if (!vts.FindTag(tag.mGuid).IsValid())
        {
            std::shared_ptr<io::Asset> newAsset = io::ResolveAsset<io::BinAsset>({}, tag, vts);
            vts.AttachTransientBlob(newAsset);
        }
    }

//-------------------------------------------------------------------------------------------------
void defensor::render::RenderSystem::PreloadAssets()
{
    // we need to have some kind of manifest file which will enumerate all the files that need to be post process and saved into a cache
    auto& vts = mApp.VTS();
    const io::VirtualTransportSystem::Section vertexShaderSection("VeretexShaders");
    auto vertexShaderTags = vts.GetTags(vertexShaderSection);

    const io::VirtualTransportSystem::Section pixelShaderSection("PixelShaders");
    auto pixelShaderTags = vts.GetTags(pixelShaderSection);

    mRenderShaders.GetShaders(vertexShaderTags, yaget::render::RenderShaders::ShaderType::Vertex);
    mRenderShaders.GetShaders(pixelShaderTags, yaget::render::RenderShaders::ShaderType::Pixel);

    const io::VirtualTransportSystem::Section materialSection("Materials");
    auto materialTags = vts.GetTags(materialSection);

    auto materials = mRenderMaterials.GetMaterials(materialTags);
    for (const auto& [material, matTag] : std::views::zip(materials, materialTags))
    {
        RebindMaterial(matTag, material);
    }

    platform::Sleep(1, time::kSecondUnit);

    mAssetsPreloaded = true;
}


//-------------------------------------------------------------------------------------------------
void defensor::render::RenderSystem::RebindMaterial(const io::Tag& matTag, yaget::render::AssetTypes material)
{
    auto& vts = mApp.VTS();

    auto vsTag = TypeToTag(material.mVertexShader, vts);
    auto psTag = TypeToTag(material.mPixelShader, vts);
    auto sigTag = TypeToTag(material.mSignature, vts);
    auto psoTag = TypeToTag(material.mPSO, vts);
    if (!vsTag.IsValid() || !psTag.IsValid() || !sigTag.IsValid() || !psoTag.IsValid())
    {
        YLOG_ERROR("REND", std::format("Material '{}' has invalid material tags. vsTag: '{}', psTag: '{}', sigTag: '{}', psoTag: '{}'", 
            conv::Convertor<io::Tag>::ToString(matTag), 
            conv::Convertor<io::Tag>::ToString(vsTag), 
            conv::Convertor<io::Tag>::ToString(psTag), 
            conv::Convertor<io::Tag>::ToString(sigTag), 
            conv::Convertor<io::Tag>::ToString(psoTag)).c_str());
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

    mDependencyGraph.ClearDirty(matTag.mGuid);
}


//-------------------------------------------------------------------------------------------------
void defensor::render::RenderSystem::HotRebindMaterial(const Guid& guid)
{
    Guid matGuid;

    std::vector<DependencyNode*> pathTo;
    DependencyNode *node = mDependencyGraph.Find(guid, &pathTo);
    node;
    if (!pathTo.empty())
    {
        DependencyNode* matNode = *pathTo.begin();
        matGuid = matNode->mGuid;
    }
    
    auto& vts = mApp.VTS();
    auto tag = vts.FindTag(matGuid);
    mRenderMaterials.ClearCache(tag);

    auto material = mRenderMaterials.GetMaterial(tag);

    auto vsTag = TypeToTag(material.mVertexShader, vts);
    mRenderShaders.ClearCache(vsTag);
    auto psTag = TypeToTag(material.mPixelShader, vts);
    mRenderShaders.ClearCache(psTag);

    auto sigTag = TypeToTag(material.mSignature, vts);
    mRenderSignatures.ClearCache(sigTag);
    
    auto psoTag = TypeToTag(material.mPSO, vts);
    mRenderPipelines.ClearCache(psoTag);

    RebindMaterial(tag, material);
}
