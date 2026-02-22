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
    mMaterialProperties = {};

    if (auto jsonAsset = LoadJson(mVTS, mAssetTag); jsonAsset && jsonAsset->IsValid())
    {
        auto& jasonBlock = jsonAsset->root;

        mMaterialProperties.mVertexShader = json::GetValue(jasonBlock, "VertexShader", AssetCacheType::Empty);
        mMaterialProperties.mPixelShader = json::GetValue(jasonBlock, "PixelShader", AssetCacheType::Empty);
        mMaterialProperties.mRasterizerState = json::GetValue(jasonBlock, "RasterizerState", AssetCacheType::RasterizerStateCounterClockwise);
        mMaterialProperties.mBlendMode = json::GetValue(jasonBlock, "BlendMode", AssetCacheType::BlendModeOpaque);
        mMaterialProperties.mDepthState = json::GetValue(jasonBlock, "DepthState", AssetCacheType::DepthStateNone);

        mMaterialProperties.mSignature = mMaterialProperties.mVertexShader | mMaterialProperties.mPixelShader;
        mMaterialProperties.mPSO = mMaterialProperties.mSignature | mMaterialProperties.mRasterizerState | mMaterialProperties.mDepthState | mMaterialProperties.mBlendMode | AssetCacheType::TopologyStateTriangle | AssetCacheType::RTVFormatRGBA8;

        YLOG_INFO("REND", "Created material for: '%s'", yaget::conv::Convertor<yaget::io::Tag>::ToString(assetTag).c_str());
    }
    else
    {
        YLOG_ERROR("REND", "Did not load material '%s'.", assetTag.ResolveVTS().c_str());
    }
}
