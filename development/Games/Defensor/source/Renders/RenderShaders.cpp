#include "RenderShaders.h"
#include "Core/ErrorHandlers.h"
#include "Render/Platform/ResourceCompiler.h"
#include "Streams/Guid.h"
#include "VTS/ResolvedAssets.h"
#include "magic_enum/magic_enum.hpp"
#include "Parsers/DependencyGraph.h"
#include "Json/JsonHelpers.h"
#include <magic_enum/magic_enum.hpp>


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
        std::string mEntryPoint;
        std::string mTarget;
    };

    inline bool operator==(const ShaderMapping& lhs, const ShaderMapping& rhs)
    {
        return lhs.mEntryPoint == rhs.mEntryPoint && lhs.mTarget == rhs.mTarget;
    }

    inline bool operator!=(const ShaderMapping& lhs, const ShaderMapping& rhs)
    {
        return !(lhs == rhs);
    }

    inline void to_json(nlohmann::json& j, const ShaderMapping shaderMapping)
    {
        j["EntryPoint"] = shaderMapping.mEntryPoint;
        j["Target"] = shaderMapping.mTarget;

        //const auto enumName = magic_enum::enum_name(shaderType);
        //j = enumName;
    }

    inline void from_json(const nlohmann::json& j, ShaderMapping& shaderMapping)
    {
        shaderMapping.mEntryPoint = yaget::json::GetValue(j, "EntryPoint", shaderMapping.mEntryPoint);
        shaderMapping.mTarget = yaget::json::GetValue(j, "Target", shaderMapping.mTarget);
    }

    using ShaderMappings = std::map<defensor::render::RenderShaders::ShaderType, ShaderMapping>;
    ShaderMappings ShaderOptionsMappings = {
        { defensor::render::RenderShaders::ShaderType::Vertex, { "VSMain", "vs_6_5" } },
        { defensor::render::RenderShaders::ShaderType::Pixel, { "PSMain", "ps_6_5" } },
        { defensor::render::RenderShaders::ShaderType::Geometry, { "GSMain", "gs_6_5" } },
        { defensor::render::RenderShaders::ShaderType::Compute, { "CSMain", "cs_6_5" } },
        { defensor::render::RenderShaders::ShaderType::Hull, { "HSMain", "hs_6_5" } },
        { defensor::render::RenderShaders::ShaderType::Domain, { "DSMain", "ds_6_5" } }
    };


    yaget::io::Buffer CompileShader(const yaget::io::Buffer& sourceBuffer, defensor::render::RenderShaders::ShaderType shaderType, bool debugShaders)
    {
        using namespace yaget;
        io::Buffer result;
        if (io::size_data(sourceBuffer))
        {
            const char* entryName = ShaderOptionsMappings[shaderType].mEntryPoint.c_str();
            const char* target = ShaderOptionsMappings[shaderType].mTarget.c_str();
            render::ResourceCompiler compiler(io::cast_to_view(sourceBuffer), entryName, target, false, debugShaders);
            result = compiler.GetCompiled();
        }
        return result;
    }

}


//-------------------------------------------------------------------------------------------------
namespace defensor::render
{
    inline void to_json(nlohmann::json& j, const RenderShaders::ShaderType shaderType)
    {
        const auto enumName = magic_enum::enum_name(shaderType);
        j = enumName;
    }

    inline void from_json(const nlohmann::json& j, RenderShaders::ShaderType& cacheType)
    {
        std::string source;
        j.get_to(source);
        auto enumValue = magic_enum::enum_cast<RenderShaders::ShaderType>(source);
        if (enumValue.has_value())
        {
            cacheType = enumValue.value();
        }
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
void defensor::render::RenderShaders::PopulateShaderMappings(io::VirtualTransportSystem::Section fileName, io::VirtualTransportSystem& vts)
{
    yaget::render::PopulateMap<ShaderMappings>(fileName, vts, ShaderOptionsMappings);
}


//-------------------------------------------------------------------------------------------------
void defensor::render::RenderShaders::SaveShaderMappings(io::VirtualTransportSystem::Section fileName, io::VirtualTransportSystem& vts)
{
    yaget::render::SaveMap(fileName, vts, ShaderOptionsMappings);
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
    io::SingleBLobLoader<io::StringsAsset> shaderLoader(mVTS, tag);
    if (auto asset = shaderLoader.GetAsset())
    {
        shaderBuffer = asset->mBuffer;
    }

#if YAGET_DEBUG_RENDER == 1
    bool debugShaders = true;
#else
    bool debugShaders = false;
#endif

    io::Buffer result = CompileShader(shaderBuffer, shaderType, debugShaders);
    if (!io::size_data(result))
    {
        YLOG_ERROR("COMP",
                   fmt::format("Could not get compiled {} shader for tag: '{}\n{}:'. Using built-in shader as s fallback.", magic_enum::enum_name(shaderType),
                       yaget::conv::Convertor<yaget::io::Tag>::ToString(tag), tag.ResolveVTS()).c_str());
        result = CompileShader(io::CreateBuffer(buildInShaderSource, std::strlen(buildInShaderSource)), shaderType, false);
        error_handlers::ThrowOnError(io::size_data(result) > 0,
                                     fmt::format("Could not compile built-in shader type: '%s'. Source:\n'%s'", magic_enum::enum_name(shaderType),
                                                 buildInShaderSource));
    }

    mAssets.insert({ tag, result });
    mCache.SaveCachedAsset(tag, result);
    return result;
}
