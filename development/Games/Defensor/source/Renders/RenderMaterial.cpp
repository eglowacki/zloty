#include "Renders/RenderMaterial.h"
#include "Streams/Buffers.h"
#include "VTS/ResolvedAssets.h"

//-------------------------------------------------------------------------------------------------
defensor::render::RenderMaterial::RenderMaterial(const io::Tag& assetTag, io::VirtualTransportSystem& vts)
    : mVTS(vts)
{
    ResolveAssetTag(assetTag);
}


//-------------------------------------------------------------------------------------------------
defensor::render::RenderMaterial::~RenderMaterial() = default;


//-------------------------------------------------------------------------------------------------
void defensor::render::RenderMaterial::ResolveAssetTag(const io::Tag& assetTag)
{
    if (assetTag == mAssetTag)
    {
        YLOG_WARNING("REND", "New tag '%s' is the same as current one, ignoring.", conv::Convertor<io::VirtualTransportSystem::Section>::ToString(assetTag).c_str());
        return;
    }

    mAssetTag = assetTag;
    mAssetTypes = {};

    if (auto jsonAsset = LoadJson(mVTS, mAssetTag); jsonAsset && jsonAsset->IsValid())
    {
        auto& jasonBlock = jsonAsset->root;

        mAssetTypes.mVertexShader = json::GetValue(jasonBlock, "VertexShader", AssetCacheType::Empty);
        mAssetTypes.mPixelShader = json::GetValue(jasonBlock, "PixelShader", AssetCacheType::Empty);
        mAssetTypes.mRasterizerState = json::GetValue(jasonBlock, "RasterizerState", AssetCacheType::RasterizerStateCounterClockwise);
        mAssetTypes.mBlendMode = json::GetValue(jasonBlock, "BlendMode", AssetCacheType::BlendModeOpaque);
        mAssetTypes.mDepthState = json::GetValue(jasonBlock, "DepthState", AssetCacheType::DepthStateNone);

        mAssetTypes.mSignature = mAssetTypes.mVertexShader | mAssetTypes.mPixelShader;
        mAssetTypes.mPSO = mAssetTypes.mSignature | mAssetTypes.mRasterizerState | mAssetTypes.mDepthState | mAssetTypes.mBlendMode | AssetCacheType::TopologyStateTriangle | AssetCacheType::RTVFormatRGBA8;

        //auto vertexSection = yaget::render::AssetCache::operator[](mVertexShader);
        //auto pixelSection = yaget::render::AssetCache::operator[](mPixelShader);
        //nlohmann::json jsonBlock;
        //jsonBlock["VertexShader"] = mVertexShader;
        //jsonBlock["PixelShader"] = mPixelShader;
        //jsonBlock["RasterizerState"] = mRasterizerState;
        //jsonBlock["BlendMode"] = mBlendMode;
        //jsonBlock["DepthState"] = mDepthState;

        //auto textBlock = json::PrettyPrint(jsonBlock);
    }
    else
    {
        YLOG_ERROR("REND", "Did not load material '%s'.", assetTag.ResolveVTS().c_str());
    }
}
