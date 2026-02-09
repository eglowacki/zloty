#include "Renders/RenderMaterial.h"
#include "Streams/Buffers.h"
#include "VTS/ResolvedAssets.h"


//-------------------------------------------------------------------------------------------------
defensor::render::RenderMaterial::RenderMaterial(const io::Tag& assetTag, io::VirtualTransportSystem& vts)
    : mAssetTag(assetTag)
    , mVTS(vts)
{
    ResolveAssetTag(assetTag);
}


//-------------------------------------------------------------------------------------------------
defensor::render::RenderMaterial::~RenderMaterial() = default;


//-------------------------------------------------------------------------------------------------
void defensor::render::RenderMaterial::ResolveAssetTag(const io::Tag& assetTag)
{
    mAssetTag = assetTag;
    if (auto jsonAsset = LoadJson(mVTS, mAssetTag); jsonAsset && jsonAsset->IsValid())
    {
        auto& jasonBlock = jsonAsset->root;

        mVertexShader = json::GetValue(jasonBlock, "VertexShader", yaget::render::AssetCacheType::DSVFormatBlah7);
        mPixelShader = json::GetValue(jasonBlock, "PixelShader", yaget::render::AssetCacheType::DSVFormatBlah7);
        mRasterizerState = json::GetValue(jasonBlock, "RasterizerState", yaget::render::AssetCacheType::DSVFormatBlah7);
        mBlendMode = json::GetValue(jasonBlock, "BlendMode", yaget::render::AssetCacheType::DSVFormatBlah7);
        mDepthState = json::GetValue(jasonBlock, "DepthState", yaget::render::AssetCacheType::DSVFormatBlah7);

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
