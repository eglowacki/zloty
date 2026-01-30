#include "RenderShaders.h"
#include "Core/ErrorHandlers.h"
#include "Render/Platform/ResourceCompiler.h"
#include "Streams/Guid.h"
#include "VTS/ResolvedAssets.h"
#include "magic_enum/magic_enum.hpp"
#include "Parsers/DependencyGraph.h"


namespace
{
    const char* buildInShaderSource = R"( struct PSInput
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

    std::map<defensor::render::RenderShaders::ShaderType, ShaderMapping> ShaderMappings = {
        { defensor::render::RenderShaders::ShaderType::Vertex, { "VSMain", "vs_6_5" } },
        { defensor::render::RenderShaders::ShaderType::Pixel, { "PSMain", "ps_6_5" } },
        { defensor::render::RenderShaders::ShaderType::Geometry, { "GSMain", "gs_6_5" } },
        { defensor::render::RenderShaders::ShaderType::Compute, { "CSMain", "cs_6_5" } },
        { defensor::render::RenderShaders::ShaderType::Hull, { "HSMain", "hs_6_5" } },
        { defensor::render::RenderShaders::ShaderType::Domain, { "DSMain", "ds_6_5" } }
    };


    yaget::io::Buffer CompileShader(const yaget::io::Buffer& sourceBuffer, defensor::render::RenderShaders::ShaderType shaderType)
    {
        using namespace yaget;
        io::Buffer result;
        if (io::size_data(sourceBuffer))
        {
            const char* entryName = ShaderMappings[shaderType].mEntryPoint;
            const char* target = ShaderMappings[shaderType].mTarget;
            render::ResourceCompiler compiler(io::cast_to_view(sourceBuffer), entryName, target, false /*useOldCompiler*/);
            result = compiler.GetCompiled();
        }
        return result;
    }
}


//-------------------------------------------------------------------------------------------------
defensor::render::RenderShaders::RenderShaders(yaget::io::VirtualTransportSystem& vts, yaget::DependencyGraph& dependencyGraph, io::Watcher& watcher)
    : CacheWatcher(vts, yaget::io::VirtualTransportSystem::Section("Caches@Shaders"), dependencyGraph, watcher)
{
}


//-------------------------------------------------------------------------------------------------
defensor::render::RenderShaders::~RenderShaders() = default;


//-------------------------------------------------------------------------------------------------
yaget::io::Buffer defensor::render::RenderShaders::GetShader(const yaget::io::Tag& tag, ShaderType shaderType)
{
    YAGET_ASSERT(tag.IsValid(), "Tag: '%s:%s' is not valid.", yaget::conv::Convertor<yaget::Guid>::ToString(tag.mGuid).c_str(),
                 yaget::conv::Convertor<yaget::io::Tag>::ToString(tag).c_str());
    auto result = GetShaders(io::Tags{ tag }, shaderType);
    return !result.empty() ? *result.begin() : yaget::io::Buffer{};
}


//-------------------------------------------------------------------------------------------------
std::vector<yaget::io::Buffer> defensor::render::RenderShaders::GetShaders(const yaget::io::Tags& tags, ShaderType shaderType)
{
    std::lock_guard mutexLocker(mMutex);
    std::vector<yaget::io::Buffer> results = tags | std::views::transform([this, shaderType](const auto& tag)
    {
        return AssureShaderNonMT(tag, shaderType);
    }) | std::ranges::to<std::vector>();
    return results;
}


//-------------------------------------------------------------------------------------------------
yaget::io::Buffer defensor::render::RenderShaders::AssureShaderNonMT(const yaget::io::Tag& tag, defensor::render::RenderShaders::ShaderType shaderType)
{
    if (auto asset = GetAsset(tag); io::size_data(asset))
    {
        return asset;
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
        YLOG_ERROR("COMP",
                   fmt::format("Could not get compiled {} shader for tag: '{}\n{}:'. Using built-in shader as s fallback.", magic_enum::enum_name(shaderType),
                       yaget::conv::Convertor<yaget::io::Tag>::ToString(tag), tag.ResolveVTS()).c_str());
        result = CompileShader(io::CreateBuffer(buildInShaderSource, std::strlen(buildInShaderSource)), shaderType);
        error_handlers::ThrowOnError(io::size_data(result) > 0,
                                     fmt::format("Could not compile built-in shader type: '%s'. Source:\n'%s'", magic_enum::enum_name(shaderType),
                                                 buildInShaderSource));
    }
    mAssets.insert({ tag, result });
    mCache.SaveCachedAsset(tag, result);
    return result;
}
