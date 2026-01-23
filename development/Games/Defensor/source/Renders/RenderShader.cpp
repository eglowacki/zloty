#include "RenderShader.h"

#include "Core/ErrorHandlers.h"
#include "Render/Platform/ResourceCompiler.h"
#include "Streams/Guid.h"
#include "VTS/ResolvedAssets.h"
#include "magic_enum/magic_enum.hpp"


namespace
{
    const char* buildInShaderSource = 
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

    yaget::io::Buffer CompileShader(const yaget::io::Buffer& sourceBuffer, defensor::render::RenderShader::ShaderType shaderType)
    {
        using namespace yaget;

        io::Buffer result;
        if (io::size_data(sourceBuffer))
        {
            const char* entryName = ShaderMappings[shaderType].mEntryPoint;
            const char* target = ShaderMappings[shaderType].mTarget;

            render::ResourceCompiler compiler(io::cast_to_view(sourceBuffer), entryName, target, false /*useNewestCompiler*/);
            result = compiler.GetCompiled();
        }

        return result;
    }

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

    auto result = GetShaders( io::Tags{ tag }, shaderType);

    return !result.empty() ? *result.begin() : yaget::io::Buffer{};
}


//-------------------------------------------------------------------------------------------------
std::vector<yaget::io::Buffer> defensor::render::RenderShader::GetShaders(const yaget::io::Tags& tags, ShaderType shaderType)
{
    std::lock_guard mutexLocker(mMutex);

    std::vector<yaget::io::Buffer> results = 
        tags | 
        std::views::transform([this, shaderType](const auto& tag)
        {
            return AssureShaderNonMT(tag, shaderType);
        }) | 
        std::ranges::to<std::vector>();

    return results;
}


//-------------------------------------------------------------------------------------------------
void defensor::render::RenderShader::CachedAssetChanged(const io::Tag& tag)
{
    std::lock_guard mutexLocker(mMutex);

    mAssets.erase(tag);
    mCache.ClearCachedAsset(tag);
    mVTS.ClearAsset(tag);
}


//-------------------------------------------------------------------------------------------------
yaget::io::Buffer defensor::render::RenderShader::AssureShaderNonMT(const yaget::io::Tag& tag, defensor::render::RenderShader::ShaderType shaderType)
{
    AssureTagWatch(tag, [this](auto tag) { CachedAssetChanged(tag); });

    if (auto it = mAssets.find(tag); it != mAssets.end())
    {
        return it->second;
    }

    if (auto shader = mCache.GetCachedAsset(tag); yaget::io::size_data(shader))
    {
        mAssets.insert({ tag, shader });
        return shader;
    }

    yaget::io::Buffer shaderBuffer;
    if (tag.mName == "EmbeddedVertexShader" || tag.mName == "EmbeddedPixelShader")
    {
        shaderBuffer = io::CreateBuffer(buildInShaderSource, std::strlen(buildInShaderSource));
    }
    else
    {
        io::SingleBLobLoader<io::StringsAsset> shaderLoader(mVTS, tag);
        if (auto asset = shaderLoader.GetAsset())
        {
            shaderBuffer = asset->mBuffer;
        }
    }

    io::Buffer result = CompileShader(shaderBuffer, shaderType);
    if (!io::size_data(result))
    {
        YLOG_ERROR("COMP", fmt::format("Could not get compiled {} shader for tag: '{}\n{}:'. Using built-in shader as s fallback.", magic_enum::enum_name(shaderType), yaget::conv::Convertor<yaget::io::Tag>::ToString(tag), tag.ResolveVTS()).c_str());
        result = CompileShader(io::CreateBuffer(buildInShaderSource, std::strlen(buildInShaderSource)), shaderType);
        error_handlers::ThrowOnError(io::size_data(result) > 0, fmt::format("Could not compile built-in shader type: '%s'. Source:\n'%s'", magic_enum::enum_name(shaderType), buildInShaderSource));
    }

    mAssets.insert({ tag, result });
    mCache.SaveCachedAsset(tag, result);

    return result;
}
