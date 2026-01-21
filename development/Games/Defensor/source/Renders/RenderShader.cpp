#include "RenderShader.h"
#include "Render/Platform/ResourceCompiler.h"
#include "Streams/Guid.h"
#include "VTS/ResolvedAssets.h"


namespace
{
    const char* shaderSource = 
        R"( struct PSInput
            {
                float4 position : SV_POSITION;
                float4 color : COLOR;
            };

            PSInput VSMain(float4 position : SV_POSITION, float4 color : COLOR)
            {
                PSInput result;

                result.position = position;
                result.color = color;

                return result;
            }

            float4 PSMain(PSInput input) : SV_TARGET
            {
                return input.color;
            }
        )";

    // let's have a table of mapping between shader type to (entry point and target)
    struct ShaderMapping
    {
        const char* mEntryPoint;
        const char* mTarget;
    };

    std::map<defensor::render::RenderShader::ShaderType, ShaderMapping> ShaderMappings =
    {
        { defensor::render::RenderShader::ShaderType::Vertex, {"VSMain", "vs_5_1"} },
        { defensor::render::RenderShader::ShaderType::Pixel, {"PSMain", "ps_5_1"} },
        { defensor::render::RenderShader::ShaderType::Geometry, {"GSMain", "gs_5_1"} },
        { defensor::render::RenderShader::ShaderType::Compute, {"CSMain", "cs_5_1"} },
        { defensor::render::RenderShader::ShaderType::Hull, {"HSMain", "hs_5_1"} },
        { defensor::render::RenderShader::ShaderType::Domain, {"DSMain", "ds_5_1"} }
    };
  
}


//-------------------------------------------------------------------------------------------------
defensor::render::RenderShader::RenderShader(yaget::io::VirtualTransportSystem& vts)
    : CacheWatcher(vts, yaget::io::VirtualTransportSystem::Section("Caches@Shaders"))
{
}


//-------------------------------------------------------------------------------------------------
defensor::render::RenderShader::~RenderShader() = default;


//-------------------------------------------------------------------------------------------------
yaget::io::Buffer defensor::render::RenderShader::GetShader(const yaget::io::Tag& tag, ShaderType shaderType)
{
    YAGET_ASSERT(tag.IsValid(), "Tag: '%s:%s' is not valid.", yaget::conv::Convertor<yaget::Guid>::ToString(tag.mGuid).c_str(), yaget::conv::Convertor<yaget::io::Tag>::ToString(tag).c_str());

    AssureTagWatch(tag, [this](auto tag) { CachedAssetChanged(tag); });

    if (auto it = mShaders.find(tag); it != mShaders.end())
    {
        return it->second;
    }

    if (auto shader = mCache.GetCachedAsset(tag); yaget::io::size_data(shader))
    {
        mShaders.insert({ tag, shader });
        return shader;
    }

    const char* entryName = ShaderMappings[shaderType].mEntryPoint;
    const char* target = ShaderMappings[shaderType].mTarget;

    yaget::io::Buffer shaderBuffer;
    if (tag.mName == "EmbeddedVertexShader" || tag.mName == "EmbeddedPixelShader")
    {
        shaderBuffer = io::CreateBuffer(shaderSource, std::strlen(shaderSource));
    }
    else
    {
        io::SingleBLobLoader<io::StringsAsset> shaderLoader(mVTS, tag);
        if (auto asset = shaderLoader.GetAsset())
        {
            shaderBuffer = asset->mBuffer;
        }
    }
    
    yaget::io::Buffer result;
    if (io::size_data(shaderBuffer))
    {
        yaget::render::ResourceCompiler compiler(io::cast_to_view(shaderBuffer), entryName, target, false /*useNewestCompiler*/);
        result = compiler.GetCompiled();
        mShaders.insert({ tag, result });

        mCache.SaveCachedAsset(tag, result);
    }
    else
    {
        YLOG_ERROR("REND", "There is no data associated with shader: '%s'.", conv::Convertor<io::Tag>::ToString(tag).c_str());
    }

    return result;
}


//-------------------------------------------------------------------------------------------------
std::vector<yaget::io::Buffer> defensor::render::RenderShader::GetShaders(const yaget::io::Tags& tags, ShaderType shaderType)
{
    std::vector<yaget::io::Buffer> results = 
        tags | 
        std::views::transform([this, shaderType](const auto& tag)
        {
            return GetShader(tag, shaderType);
        }) | 
        std::ranges::to<std::vector>();

    return results;
}


//-------------------------------------------------------------------------------------------------
void defensor::render::RenderShader::CachedAssetChanged(const io::Tag& tag)
{
    tag;
}
