#include "Render/Pipeline/RenderMaterials.h"


namespace
{
    //-------------------------------------------------------------------------------------------------
    yaget::render::AssetTypes ResolveAssetTag(const yaget::io::Tag& assetTag, yaget::io::VirtualTransportSystem& vts)
    {
        using namespace yaget;

        render::AssetTypes assetTypes = {};

        if (auto jsonAsset = io::LoadJson(vts, assetTag); jsonAsset && jsonAsset->IsValid())
        {
            auto& jasonBlock = jsonAsset->root;

            assetTypes.mVertexShader = json::GetValue(jasonBlock, "VertexShader", render::AssetCacheType::Empty);
            assetTypes.mPixelShader = json::GetValue(jasonBlock, "PixelShader", render::AssetCacheType::Empty);
            assetTypes.mRasterizerState = json::GetValue(jasonBlock, "RasterizerState", render::AssetCacheType::RasterizerStateCounterClockwise);
            assetTypes.mBlendMode = json::GetValue(jasonBlock, "BlendMode", render::AssetCacheType::BlendModeOpaque);
            assetTypes.mDepthState = json::GetValue(jasonBlock, "DepthState", render::AssetCacheType::DepthStateNone);

            assetTypes.mSignature = assetTypes.mVertexShader | assetTypes.mPixelShader;
            assetTypes.mPSO = assetTypes.mSignature | assetTypes.mRasterizerState | assetTypes.mDepthState | assetTypes.mBlendMode | render::AssetCacheType::TopologyStateTriangle | render::AssetCacheType::RTVFormatRGBA8;
        }
        else
        {
            YLOG_ERROR("REND", "Did not load material '%s'.", assetTag.ResolveVTS().c_str());
        }

        return assetTypes;
    }

}

//-------------------------------------------------------------------------------------------------
yaget::render::RenderMaterials::RenderMaterials(io::VirtualTransportSystem& vts)
    : CacheWatcher(vts, yaget::io::VirtualTransportSystem::Section("Caches@Materials"))
{
}


//-------------------------------------------------------------------------------------------------
yaget::render::RenderMaterials::~RenderMaterials() = default;



//-------------------------------------------------------------------------------------------------
yaget::render::AssetTypes yaget::render::RenderMaterials::GetMaterial(const io::Tag& tag)
{
    auto results = GetMaterials({ tag });

    return !results.empty() ? *results.begin() : AssetTypes{};
}


//-------------------------------------------------------------------------------------------------
std::vector<yaget::render::AssetTypes> yaget::render::RenderMaterials::GetMaterials(const io::Tags& tags)
{
    std::vector<yaget::render::AssetTypes> results;

    std::lock_guard mutexLocker(mMutex);

    for (auto tag : tags)
    {
        auto result = GetAsset(tag, [this, &results](auto tag, auto& cachedData)
        {
            AssetTypes* assetTypes = nullptr;
            if (io::size_data(cachedData))
            {
                assetTypes = io::cast_data<AssetTypes>(cachedData);
            }
            else
            {
                AssetTypes materialData = ResolveAssetTag(tag, mVTS);
                auto matBuffer = io::CreateBuffer(reinterpret_cast<const uint8_t*>(&materialData), sizeof(materialData));
                cachedData = io::ResizeBuffer(cachedData, sizeof(materialData));
                io::CopyBuffer(matBuffer, cachedData, 0);

                assetTypes = io::cast_data<AssetTypes>(cachedData);
            }

            return *assetTypes;
        });
        
        results.push_back(result);
    }

    return results;
}
