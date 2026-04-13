#include "Render/Pipeline/RenderMaterialProperties.h"
#include "Render/Pipeline/PipelineTags.h"
#include "HashUtilities.h"


namespace
{
    //-------------------------------------------------------------------------------------------------
    yaget::render::MaterialPropertyTags ResolveAssetTag(const yaget::io::Tag& assetTag, yaget::render::PipelineTags& pipelineTags, yaget::io::VirtualTransportSystem& vts)
    {
        using namespace yaget;
        using Section = io::VirtualTransportSystem::Section;

        render::MaterialPropertyTags materialTags = {};

        if (auto jsonAsset = io::LoadJson(vts, assetTag); jsonAsset && jsonAsset->IsValid())
        {
            auto& jasonBlock = jsonAsset->root;

            auto vsSection = json::GetValue(jasonBlock, "VertexShader", Section{});
            materialTags.mVertexShader = vts.GetTag(vsSection);

            auto psSection = json::GetValue(jasonBlock, "PixelShader", Section{});
            materialTags.mPixelShader = vts.GetTag(psSection);

            auto rasterizerState = json::GetValue(jasonBlock, "RasterizerState", render::AssetCacheType::RasterizerStateCounterClockwise);
            auto blendMode = json::GetValue(jasonBlock, "BlendMode", render::AssetCacheType::BlendModeOpaque);
            auto depthState = json::GetValue(jasonBlock, "DepthState", render::AssetCacheType::DepthStateNone);
            auto topologyState = render::AssetCacheType::TopologyStateTriangle;
            auto rtFormat = render::AssetCacheType::RTVFormatRGBA8;

            std::string sigCreated = " ";
            size_t sigHash = 0;
            conv::hash_combine(sigHash, materialTags.mVertexShader.mGuid, materialTags.mPixelShader.mGuid);
            materialTags.mSignature = pipelineTags.GetTag(sigHash);
            if (!materialTags.mSignature.IsValid())
            {
                const std::string sigName = std::format("Signature=vertex{}-pixel{}", vsSection.mFilter, psSection.mFilter);
                materialTags.mSignature = pipelineTags.ResolveTag(sigHash, sigName);
                sigCreated = "*";
            }

            std::string psoCreated = " ";
            size_t psoHash = 0;
            auto psoProperties = rasterizerState | blendMode | depthState | topologyState | rtFormat;
            conv::hash_combine(psoHash, materialTags.mSignature.mGuid, psoProperties);
            materialTags.mPSO = pipelineTags.GetTag(psoHash);
            if (!materialTags.mPSO.IsValid())
            {
                std::string psoName = std::format("PSO={}={}", materialTags.mSignature.mName, conv::ToString(psoProperties));
                conv::ReplaceAll(psoName, "|", "-");
                conv::ReplaceAll(psoName, " ", "");
                materialTags.mPSO = pipelineTags.ResolveTag(psoHash, psoName);
                psoCreated = "*";

                render::AssetCache::AddTagToType(materialTags.mPSO, psoProperties);
            }

            std::string constantCreated = " ";
            size_t constantHash = 0;
            auto constantProperties = render::AssetCacheType::ShaderBuffer;
            conv::hash_combine(constantHash, materialTags.mSignature.mGuid, constantProperties);
            materialTags.mShaderBuffer = pipelineTags.GetTag(constantHash);
            if (!materialTags.mShaderBuffer.IsValid())
            {
                std::string constantName = std::format("Constant=vertex{}-pixel{}={}", vsSection.mFilter, psSection.mFilter, conv::ToString(constantProperties));
                conv::ReplaceAll(constantName, "|", "-");
                conv::ReplaceAll(constantName, " ", "");
                materialTags.mShaderBuffer = pipelineTags.ResolveTag(constantHash, constantName);
                constantCreated = "*";
            }

            YLOG_INFO("REND", std::format("Generated transient pipelines for Material: {}\n\tSignature: {}{}\n\tPSO:       {}{}\n\tConstant:  {}{}.",
                          conv::ToString(assetTag),
                          sigCreated, conv::ToString(materialTags.mSignature),
                          psoCreated, conv::ToString(materialTags.mPSO), 
                          constantCreated, conv::ToString(materialTags.mShaderBuffer)).c_str());
        }

        else
        {
            YLOG_ERROR("REND", "Did not load material '%s'.", assetTag.ResolveVTS().c_str());
        }

        return materialTags;
    }
}

//-------------------------------------------------------------------------------------------------
yaget::render::RenderMaterialProperties::RenderMaterialProperties(PipelineTags& pipelineTags, io::VirtualTransportSystem& vts)
    : mPipelineTags(pipelineTags)
    , mVTS(vts)
{
}


//-------------------------------------------------------------------------------------------------
yaget::render::RenderMaterialProperties::~RenderMaterialProperties() = default;


//-------------------------------------------------------------------------------------------------
yaget::render::MaterialPropertyTags yaget::render::RenderMaterialProperties::GetMaterial(const io::Tag& tag)
{
    auto results = GetMaterials({ tag });

    return !results.empty() ? *results.begin() : MaterialPropertyTags{};
}


//-------------------------------------------------------------------------------------------------
std::vector<yaget::render::MaterialPropertyTags> yaget::render::RenderMaterialProperties::GetMaterials(const io::Tags& tags)
{
    std::vector<MaterialPropertyTags> results;

    for (auto tag : tags)
    {
        {
            mt::ReadLock locker(mMutexTags);
            if (auto it = mMaterialTags.find(tag); it != mMaterialTags.end())
            {
                results.push_back(it->second);
                continue;
            }
        }

        mt::WriteLock locker(mMutexTags);
        if (auto it = mMaterialTags.find(tag); it != mMaterialTags.end())
        {
            results.push_back(it->second);
            continue;
        }

        MaterialPropertyTags materialTags = ResolveAssetTag(tag, mPipelineTags, mVTS);
        results.push_back(materialTags);

        mMaterialTags[tag] = materialTags;
    }

    return results;
}


//-------------------------------------------------------------------------------------------------
void yaget::render::RenderMaterialProperties::ClearCache(const io::Tag& tag)
{
    mt::WriteLock locker(mMutexTags);
    mMaterialTags.erase(tag);
}


//-------------------------------------------------------------------------------------------------
void yaget::render::RenderMaterialProperties::PopulateMappings(io::VirtualTransportSystem::Section /*fileName*/, io::VirtualTransportSystem& /*vts*/)
{
}


//-------------------------------------------------------------------------------------------------
void yaget::render::RenderMaterialProperties::SaveMappings(io::VirtualTransportSystem::Section /*fileName*/, io::VirtualTransportSystem& /*vts*/)
{
}
