#include "Render/Cache/AssetCache.h"
#include "Render/Commands/RenderCommandList.h"
#include "Render/Pipeline/ConstantBuffer.h"
#include "Render/Pipeline/RenderMaterialProperties.h"
#include "Render/Pipeline/RenderPipelines.h"
#include "Render/Pipeline/RenderSignatures.h"
#include "Render/Pipeline/RenderTextures.h"
#include "Render/Pipeline/ShaderBuffers.h"
#include "Render/Scene/RenderSceneItems.h"

namespace
{
    //-------------------------------------------------------------------------------------------------
    struct ItemProperties
    {
        using Section = yaget::io::VirtualTransportSystem::Section;
        using Sections = yaget::io::VirtualTransportSystem::Sections;

        Section mMaterial;
        Sections mGeometries;
        Sections mTextures;

        uint32_t mRenderOrder{ yaget::render::scene::SceneItem::PassOrderIndependent};
    };


    //-------------------------------------------------------------------------------------------------
    void to_json(nlohmann::json& j, const ItemProperties& itemProperties)
    {
        j["Material"] = itemProperties.mMaterial;
        j["Geometries"] = itemProperties.mGeometries;
        j["Textures"] =  itemProperties.mTextures;
        j["RenderOrder"] =  itemProperties.mRenderOrder;
    }


    //-------------------------------------------------------------------------------------------------
    void from_json(const nlohmann::json& j, ItemProperties& itemProperties)
    {
        itemProperties.mMaterial = yaget::json::GetValue(j, "Material", itemProperties.mMaterial);
        itemProperties.mGeometries = yaget::json::GetValue(j, "Geometries", itemProperties.mGeometries);
        itemProperties.mTextures = yaget::json::GetValue(j, "Textures", itemProperties.mTextures);
        itemProperties.mRenderOrder = yaget::json::GetValue(j, "RenderOrder", itemProperties.mRenderOrder);
    }


    D3D_PRIMITIVE_TOPOLOGY GetTopologyType(yaget::render::AssetCacheType psoType)
    {
        using namespace yaget::render;

        D3D_PRIMITIVE_TOPOLOGY topology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
        if (has(psoType, AssetCacheType::TopologyStateTriangle))
        {
            topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        }
        else if (has(psoType, AssetCacheType::TopologyStatePoint))
        {
            topology = D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
        }
        else if (has(psoType, AssetCacheType::TopologyStateLine))
        {
            topology = D3D_PRIMITIVE_TOPOLOGY_LINELIST;
        }

        return topology;
    }


}

//-------------------------------------------------------------------------------------------------
yaget::render::scene::SceneItem::SceneItem() = default;


//-------------------------------------------------------------------------------------------------
yaget::render::scene::SceneItem::~SceneItem() = default;


//-------------------------------------------------------------------------------------------------
void yaget::render::scene::SceneItem::Render(uint32_t bufferIndex, const commands::CommandList* commandList, commands::RenderPassState& currentRenderPassState)
{
    auto deviceCommandList = commandList->GetDeviceCommandList();

    if (currentRenderPassState.CheckNewHash(mRootSignature, commands::RenderPassState::HashType::RootSignature))
    {
        deviceCommandList->SetGraphicsRootSignature(mRootSignature);
    }

    if (currentRenderPassState.CheckNewHash(mPipelineState, commands::RenderPassState::HashType::PipelineState))
    {
        deviceCommandList->SetPipelineState(mPipelineState);
    }

    auto topology = GetTopologyType(mTags.mPsoCacheType);
    if (currentRenderPassState.CheckNewHash(topology, commands::RenderPassState::HashType::Topology))
    {
        deviceCommandList->IASetPrimitiveTopology(topology);
    }

    constexpr constant_shader_types::ConstantTypes textureTypes[4] =
    {
        constant_shader_types::ConstantTypes::Texture2d,
        constant_shader_types::ConstantTypes::Texture2dSecond,
        constant_shader_types::ConstantTypes::Texture2dThird,
        constant_shader_types::ConstantTypes::Texture2dFourth
    };

    auto commandType = commandList->GetType();
    for (size_t i = 0; i < mTextureResources.size(); ++i)
    {
        mConstantBuffer->UpdateData(bufferIndex, textureTypes[i], mTextureResources[i], commandType);
    }

    mConstantBuffer->Bind(deviceCommandList);

    std::vector<RenderShape> renderShapes{ mGeometriesData.size() };

    for (const auto& [geom, shape] : std::views::zip(mGeometriesData, renderShapes))
    {
        shape.Bind(geom);
        shape.Render(deviceCommandList);
    }
}


//-------------------------------------------------------------------------------------------------
uint64_t yaget::render::scene::SceneItem::GetRenderOrder() const
{
    uint64_t order{};
    uint32_t propertiesOrder = 0;

    order = (static_cast<uint64_t>(mTags.mRenderPassOrder) << 32) | propertiesOrder;

    return order;
}


//-------------------------------------------------------------------------------------------------
const yaget::render::scene::SceneItem::AssetTags& yaget::render::scene::SceneItem::GetTags() const
{
    return mTags;
}


//-------------------------------------------------------------------------------------------------
bool yaget::render::scene::SceneItem::UpdateData(uint32_t bufferIndex, constant_shader_types::ConstantTypes constantTypes, const uint8_t* data, size_t dataSize, commands::Type commandType)
{
    if (constantTypes == constant_shader_types::ConstantTypes::GeometryData)
    {
        const auto geomData = reinterpret_cast<const GeometriesResources::GeometryData*>(data);
        const auto& passIndex = geomData->mPassIndex;

        auto minSizeNeeded = std::max<size_t>(mGeometriesData.size(), passIndex + 1);
        mGeometriesData.resize(minSizeNeeded);

        mGeometriesData[passIndex] = *geomData;

        return true;
    }

    return mConstantBuffer->UpdateData(bufferIndex, constantTypes, data, dataSize, commandType);
}


//-------------------------------------------------------------------------------------------------
yaget::render::scene::SceneItemsStorage::SceneItemsStorage(RenderMaterialProperties& renderMaterials,
                                                           RenderSignatures& renderSignatures,
                                                           RenderPipelines& renderPipelines,
                                                           ShaderBuffers& shaderBuffers,
                                                           TextureResources& textureResources,
                                                           GeometriesResources& geometriesResources,
                                                           io::VirtualTransportSystem& vts, io::VirtualTransportSystem::Section /*fileName*/)
    : mRenderMaterials{ renderMaterials }
    , mSignatures{ renderSignatures }
    , mPipelines{ renderPipelines }
    , mShaderBuffers{ shaderBuffers }
    , mTextures{ textureResources }
    , mGeometries{ geometriesResources }
    , mVTS{ vts }
{
}


//-------------------------------------------------------------------------------------------------
yaget::render::scene::SceneItemsStorage::~SceneItemsStorage() = default;


//-------------------------------------------------------------------------------------------------
yaget::render::scene::SceneItem* yaget::render::scene::SceneItemsStorage::GetSceneItem(const io::Tag& tag)
{
    auto resources = GetSceneItems(io::Tags{ tag }, nullptr);
    return !resources.empty() ? *resources.begin() : nullptr;
}


//-------------------------------------------------------------------------------------------------
std::vector<yaget::render::scene::SceneItem*> yaget::render::scene::SceneItemsStorage::GetSceneItems(const io::Tags& tags, comp::gs::mt::InitCounter* counter)
{
    std::vector<SceneItem*> results;

    for (const auto& tag : tags)
    {
        YAGET_ASSERT(tag.IsValid(), "Tag: '%s' is not valid.", yaget::conv::ToString(tag).c_str());
        {
            mt::ReadLock locker(mMutex);
            if (auto it = mItems.find(tag); it != mItems.end())
            {
                results.push_back(&it->second);
                if (counter)
                {
                    ++(*counter);
                }
                continue;
            }
        }

        mt::WriteLock locker(mMutex);

        if (auto it = mItems.find(tag); it != mItems.end())
        {
            results.push_back(&it->second);
            if (counter)
            {
                ++(*counter);
            }
            continue;
        }

        auto itemProperties = LoadBlob<ItemProperties>(mVTS, tag);

        auto materialTag = mVTS.GetTag(itemProperties.mMaterial);
        auto geometriesTags = mVTS.GetTags(itemProperties.mGeometries);
        auto texturesTags = mVTS.GetTags(itemProperties.mTextures);

        MaterialPropertyTags materialProperties = mRenderMaterials.GetMaterial(materialTag);
        auto geometriesData = mGeometries.GetResources(geometriesTags, nullptr);
        auto textures = mTextures.GetResourceViews(texturesTags);

        auto rootSig = mSignatures.GetSignature(materialProperties.mSignature);

        auto pso = mPipelines.GetPipeline(materialProperties.mPSO);

        auto constantBuffer = mShaderBuffers.GetBuffer(materialProperties.mShaderBuffer);

        auto& sceneItem = mItems[tag];
        sceneItem.mRootSignature = rootSig;
        sceneItem.mPipelineState = pso;
        sceneItem.mConstantBuffer = constantBuffer;
        sceneItem.mGeometriesData = std::move(geometriesData);
        sceneItem.mTextureResources = std::move(textures);

        sceneItem.mTags = 
        {
            .mMaterialTag = materialTag,
            .mGeometriesTags = std::move(geometriesTags),
            .mTexturesTags = std::move(texturesTags),
            .mRenderPassOrder =  itemProperties.mRenderOrder,
            .mPsoCacheType = AssetCache::TagToType(materialProperties.mPSO)
        };

        results.push_back(&sceneItem);
        if (counter)
        {
            ++(*counter);
        }
    }

    return results;
}


//-------------------------------------------------------------------------------------------------
void yaget::render::scene::SceneItemsStorage::SortSceneItems(std::vector<SceneItem*>& sceneItems)
{
    std::ranges::sort(sceneItems, [](const SceneItem* item1, const SceneItem* item2)
    {
        return item1->GetRenderOrder() < item2->GetRenderOrder();
    });
}


//-------------------------------------------------------------------------------------------------
void yaget::render::scene::SceneItemsStorage::Preload(const io::Tags& tags, comp::gs::mt::InitCounter& counter)
{
    GetSceneItems(tags, &counter);
}


//-------------------------------------------------------------------------------------------------
void yaget::render::scene::SceneItemsStorage::ResetAll(const app::WindowFrame& /*windowFrame*/)
{
    mt::WriteLock locker(mMutex);
    mItems.clear();
}


//-------------------------------------------------------------------------------------------------
void yaget::render::scene::SceneItemsStorage::ClearItem(const io::Tag& tag)
{
    mt::WriteLock locker(mMutex);
    mItems.erase(tag);
}


//-------------------------------------------------------------------------------------------------
void yaget::render::scene::SceneItemsStorage::PopulateMappings(io::VirtualTransportSystem::Section /*fileName*/, io::VirtualTransportSystem& /*vts*/)
{
    //PopulateMap(fileName, vts, ShaderOptionsMappings);
}


//-------------------------------------------------------------------------------------------------
void yaget::render::scene::SceneItemsStorage::SaveMappings(io::VirtualTransportSystem::Section /*fileName*/, io::VirtualTransportSystem& /*vts*/)
{
    //SaveMap(fileName, vts, ShaderOptionsMappings);
}
