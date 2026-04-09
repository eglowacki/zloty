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
    struct ItemProperties
    {
        yaget::io::VirtualTransportSystem::Section mMaterial;
        yaget::io::VirtualTransportSystem::Section mGeometry;
        yaget::io::VirtualTransportSystem::Sections mTextures;
    };


    //-------------------------------------------------------------------------------------------------
    void to_json(nlohmann::json& j, const ItemProperties& itemProperties)
    {
        j["Material"] = itemProperties.mMaterial;
        j["Geometry"] = itemProperties.mGeometry;
        j["Textures"] =  itemProperties.mTextures;
    }


    //-------------------------------------------------------------------------------------------------
    void from_json(const nlohmann::json& j, ItemProperties& itemProperties)
    {
        itemProperties.mMaterial = yaget::json::GetValue(j, "Material", itemProperties.mMaterial);
        itemProperties.mGeometry = yaget::json::GetValue(j, "Geometry", itemProperties.mGeometry);
        itemProperties.mTextures = yaget::json::GetValue(j, "Textures", itemProperties.mTextures);
    }
    
}

//-------------------------------------------------------------------------------------------------
yaget::render::scene::SceneItem::SceneItem()
{
}


//-------------------------------------------------------------------------------------------------
yaget::render::scene::SceneItem::~SceneItem() = default;


//-------------------------------------------------------------------------------------------------
void yaget::render::scene::SceneItem::Render(commands::CommandList* commandList)
{
    auto deviceCommandList = commandList->GetDeviceCommandList();

    deviceCommandList->SetGraphicsRootSignature(mRootSignature);
    deviceCommandList->SetPipelineState(mPipelineState);

    constexpr constant_shader_types::ConstantTypes textureTypes[4] =
    {
        constant_shader_types::ConstantTypes::Texture2d,
        constant_shader_types::ConstantTypes::Texture2dSecond,
        constant_shader_types::ConstantTypes::Texture2dThird,
        constant_shader_types::ConstantTypes::Texture2dFourth
    };

    for (size_t i = 0; i < mTextureResources.size(); ++i)
    {
        mConstantBuffer->UpdateData(textureTypes[i], mTextureResources[i]);
    }

    mConstantBuffer->Bind(deviceCommandList);

    mRenderShape.Bind(mGeometryData);
    mRenderShape.Render(deviceCommandList);
}


//-------------------------------------------------------------------------------------------------
bool yaget::render::scene::SceneItem::UpdateData(constant_shader_types::ConstantTypes constantTypes, const uint8_t* data, size_t dataSize)
{
    return mConstantBuffer->UpdateData(constantTypes, data, dataSize);
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
    //ItemProperties itemProperties{
    //    .mMaterial = "Materials@BasicTextureMaterial",
    //    .mGeometry = "Geometry@Rectangle",
    //    .mTextures = { Section("Images@Checker"), Section("Images@Red") }
    //};

    //nlohmann::json jsonBlock = itemProperties;
    //auto textBlock = json::PrettyPrint(jsonBlock);
    //textBlock;

    //int z = 0;
    //z;
}


//-------------------------------------------------------------------------------------------------
yaget::render::scene::SceneItemsStorage::~SceneItemsStorage() = default;


//-------------------------------------------------------------------------------------------------
yaget::render::scene::SceneItem* yaget::render::scene::SceneItemsStorage::GetSceneItem(const io::Tag& tag)
{
    auto resources = GetSceneItems(io::Tags{ tag });
    return !resources.empty() ? *resources.begin() : nullptr;
}


//-------------------------------------------------------------------------------------------------
std::vector<yaget::render::scene::SceneItem*> yaget::render::scene::SceneItemsStorage::GetSceneItems(const io::Tags& tags)
{
    std::vector<yaget::render::scene::SceneItem*> results;
    for (const auto& tag : tags)
    {
        YAGET_ASSERT(tag.IsValid(), "Tag: '%s' is not valid.", yaget::conv::ToString(tag).c_str());
        {
            mt::ReadLock locker(mMutex);
            if (auto it = mItems.find(tag); it != mItems.end())
            {
                results.push_back(&it->second);
                continue;
            }
        }

        mt::WriteLock locker(mMutex);

        if (auto it = mItems.find(tag); it != mItems.end())
        {
            results.push_back(&it->second);
            continue;
        }

        auto itemProperties = LoadBlob<ItemProperties>(mVTS, tag);

        auto materialTag = mVTS.GetTag(itemProperties.mMaterial);
        auto geometryTag = mVTS.GetTag(itemProperties.mGeometry);
        auto texturesTags = mVTS.GetTags(itemProperties.mTextures);

        auto materialProperties = mRenderMaterials.GetMaterial(materialTag);
        auto geometryData = mGeometries.GetResource(geometryTag);
        auto textures = mTextures.GetResourceViews(texturesTags);

        io::Tag signatureTag = TypeToTag(materialProperties.mSignature, mVTS);
        auto rootSig = mSignatures.GetSignature(signatureTag);

        io::Tag psoTag = TypeToTag(materialProperties.mPSO, mVTS);
        auto pso = mPipelines.GetPipeline(psoTag);

        io::Tag constantBufferTag = TypeToTag(materialProperties.mShaderBuffer, mVTS);
        auto constantBuffer = mShaderBuffers.GetBuffer(constantBufferTag);

        auto& sceneItem = mItems[tag];
        sceneItem.mRootSignature = rootSig;
        sceneItem.mPipelineState = pso;
        sceneItem.mConstantBuffer = constantBuffer;
        sceneItem.mGeometryData = geometryData;
        std::ranges::copy(textures.begin(), textures.end(), std::back_inserter(sceneItem.mTextureResources));

        results.push_back(&sceneItem);
    }

    return results;
}


//-------------------------------------------------------------------------------------------------
void yaget::render::scene::SceneItemsStorage::Preload(const io::Tags& tags)
{
    GetSceneItems(tags);
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
