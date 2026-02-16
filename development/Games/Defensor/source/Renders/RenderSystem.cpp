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
        auto tag = vts.AssureTag(section);
        return tag;
    }
}


//-------------------------------------------------------------------------------------------------
defensor::render::RenderSystem::RenderSystem(Messaging& messaging, Application& app, RenderCoordinatorSet& coordinatorSet)
    : RenderSystemApp("RenderSystem", messaging, app, [this](auto&&... params) { OnUpdate(params...); }, coordinatorSet)
    , mAssetPoolThread("PreloadRenderAssets", 1)
    , mColorInterpolator({ 0.4f, 0.6f, 0.9f, 1.0f }, { 0.6f, 0.9f, 0.4f, 1.0f })
    , mDependencyGraph(app.VTS(), io::VirtualTransportSystem::Section("Manifest@RenderDependencies"))
    , mRenderSignatures(GetDevice().GetAdapter().GetDevice(), app.VTS(), mDependencyGraph, mWatcher)
    , mRenderPipelines(GetDevice().GetAdapter().GetDevice(), app.VTS(), mDependencyGraph, mWatcher)
    , mRenderShaders(app.VTS(), mDependencyGraph, mWatcher)
    , mRenderMaterials(app.VTS(), mDependencyGraph, mWatcher)
{
    mAssetPoolThread.AddTask([this]()
    {
        PreloadAssets();
    });
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

        const colors::Color color = mColorInterpolator.GetColor(gameClock);
        auto& device = GetDevice();
        auto framerHandle = device.GetFramerHandle(gameClock, channel, &color);
        auto commandList = framerHandle.GetCommandList();

        coordinator.ForEach<RenderEntity>([commandList, &vts, color, this](comp::Id_t /*id*/, const auto& row)
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
            }

            float matrix[16];
            math3d::GetMatrixAsFloats(location, matrix);
            std::ranges::fill(matrix, color.G());

            commandList->SetGraphicsRoot32BitConstants(0, 16, matrix, 0);
            renderComponent->Render(commandList);

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

        std::shared_ptr<io::Asset> newAsset = io::ResolveAsset<io::BinAsset>({}, tag, vts);
        vts.AttachTransientBlob(newAsset);
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
        ID3D12RootSignature* signature = nullptr;
        auto vsTag = TypeToTag(material.mVertexShader, vts);
        auto psTag = TypeToTag(material.mPixelShader, vts);

        auto sigTag = TypeToTag(material.mSignature, vts);
        mRenderShaders.CreateSignatureDescription(vsTag, psTag, [this, &sigTag, &signature](const auto& descResult)
        {
            signature = mRenderSignatures.GetSignature(sigTag, descResult);
        });
        AttachTransientAsset(sigTag, vts);

        auto psoTag = TypeToTag(material.mPSO, vts);
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

        //DependencyNode* matNode = mDependencyGraph.Find(matTag.mGuid, nullptr);
    }


    //for (const auto& materialTag : materialTags)
    //{
    //    RenderMaterial renderMaterial(materialTag, vts);

    //    //auto vsTag = TypeToTag(renderMaterial.mVertexShader, vts);
    //    ////auto vsBlob = mRenderShaders.GetShader(vsTag, yaget::render::RenderShaders::ShaderType::Vertex);
    //    //auto psTag = TypeToTag(renderMaterial.mPixelShader, vts);
    //    ////auto psBlob = mRenderShaders.GetShader(psTag, yaget::render::RenderShaders::ShaderType::Pixel);

    //    //// having both blobs for vertex and shader allows us to extract 
    //    //// D3D12_VERSIONED_ROOT_SIGNATURE_DESC and use that to find or create
    //    //// root signature
    //    auto vsTag = TypeToTag(renderMaterial.mAssetTypes.mVertexShader, vts);
    //    auto psTag = TypeToTag(renderMaterial.mAssetTypes.mPixelShader, vts);
    //    mRenderShaders.CreateSignatureDescription(vsTag, psTag, [this, &renderMaterial, &vts](const auto& descResult)
    //    {
    //        auto sigTag = TypeToTag(renderMaterial.mAssetTypes.mSignature, vts);

    //        ID3D12RootSignature* sig = mRenderSignatures.GetSignature(sigTag, descResult);


    //        sig;

    //        int z = 0;
    //        z;
    //    });

    //    int z = 0;
    //    z;
    //}

    platform::Sleep(1, time::kSecondUnit);

    mAssetsPreloaded = true;
}
