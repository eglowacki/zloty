#include "Render/Pipeline/RenderMaterialProperties.h"


namespace
{
    //-------------------------------------------------------------------------------------------------
    yaget::render::MaterialProperties ResolveAssetTag(const yaget::io::Tag& assetTag, yaget::io::VirtualTransportSystem& vts)
    {
        using namespace yaget;

        render::MaterialProperties materialProperties = {};

        if (auto jsonAsset = io::LoadJson(vts, assetTag); jsonAsset && jsonAsset->IsValid())
        {
            auto& jasonBlock = jsonAsset->root;

            materialProperties.mVertexShader = json::GetValue(jasonBlock, "VertexShader", render::AssetCacheType::Empty);
            materialProperties.mPixelShader = json::GetValue(jasonBlock, "PixelShader", render::AssetCacheType::Empty);
            materialProperties.mRasterizerState = json::GetValue(jasonBlock, "RasterizerState", render::AssetCacheType::RasterizerStateCounterClockwise);
            materialProperties.mBlendMode = json::GetValue(jasonBlock, "BlendMode", render::AssetCacheType::BlendModeOpaque);
            materialProperties.mDepthState = json::GetValue(jasonBlock, "DepthState", render::AssetCacheType::DepthStateNone);

            materialProperties.mSignature = materialProperties.mVertexShader | materialProperties.mPixelShader;
            materialProperties.mPSO = materialProperties.mSignature | materialProperties.mRasterizerState | materialProperties.mDepthState | materialProperties.mBlendMode | render::AssetCacheType::TopologyStateTriangle | render::AssetCacheType::RTVFormatRGBA8;
            materialProperties.mShaderBuffer = render::AssetCacheType::ShaderBuffer | materialProperties.mVertexShader | materialProperties.mPixelShader;
        }
        else
        {
            YLOG_ERROR("REND", "Did not load material '%s'.", assetTag.ResolveVTS().c_str());
        }

        return materialProperties;
    }

}

//-------------------------------------------------------------------------------------------------
yaget::render::RenderMaterials::RenderMaterials(io::VirtualTransportSystem& vts, io::VirtualTransportSystem::Section fileName)
    : CacheWatcher(vts, fileName)
{
}


//-------------------------------------------------------------------------------------------------
yaget::render::RenderMaterials::~RenderMaterials() = default;



//-------------------------------------------------------------------------------------------------
yaget::render::MaterialProperties yaget::render::RenderMaterials::GetMaterial(const io::Tag& tag)
{
    auto results = GetMaterials({ tag });

    return !results.empty() ? *results.begin() : MaterialProperties{};
}


//-------------------------------------------------------------------------------------------------
std::vector<yaget::render::MaterialProperties> yaget::render::RenderMaterials::GetMaterials(const io::Tags& tags)
{
    std::vector<MaterialProperties> results;

    std::lock_guard mutexLocker(mMutex);

    for (auto tag : tags)
    {
        auto result = GetAsset(tag, [this, &results](auto tag, auto& cachedData)
        {
            MaterialProperties* materialProperties = nullptr;
            if (io::size_data(cachedData))
            {
                materialProperties = io::cast_data<MaterialProperties>(cachedData);
            }
            else
            {
                MaterialProperties materialData = ResolveAssetTag(tag, mVTS);
                auto matBuffer = io::CreateBuffer(reinterpret_cast<const uint8_t*>(&materialData), sizeof(materialData));
                cachedData = io::ResizeBuffer(cachedData, sizeof(materialData));
                io::CopyBuffer(matBuffer, cachedData, 0);

                materialProperties = io::cast_data<MaterialProperties>(cachedData);
            }

            return *materialProperties;
        });
        
        results.push_back(result);
    }

    return results;
}


//-------------------------------------------------------------------------------------------------
void yaget::render::RenderMaterials::PopulateMappings(io::VirtualTransportSystem::Section /*fileName*/, io::VirtualTransportSystem& /*vts*/)
{
}


//-------------------------------------------------------------------------------------------------
void yaget::render::RenderMaterials::SaveMappings(io::VirtualTransportSystem::Section /*fileName*/, io::VirtualTransportSystem& /*vts*/)
{
}
