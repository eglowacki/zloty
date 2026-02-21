#include "Render/Pipeline/RenderShaders.h"
#include "Core/ErrorHandlers.h"
#include "Render/Platform/ResourceCompiler.h"
#include "Streams/Guid.h"
#include "VTS/ResolvedAssets.h"
#include "magic_enum/magic_enum.hpp"
#include "Parsers/DependencyGraph.h"
#include "Json/JsonHelpers.h"
#include "Exception/Exception.h"
#include <cstddef>


namespace
{
    const char* buildInShaderSource = R"( 
            struct PSInput
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


    constexpr size_t length(std::string_view sv)
    {
        return sv.size();
    }
    const std::size_t buildInShaderSourceLen = length(buildInShaderSource);;

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
    }

    inline void from_json(const nlohmann::json& j, ShaderMapping& shaderMapping)
    {
        shaderMapping.mEntryPoint = yaget::json::GetValue(j, "EntryPoint", shaderMapping.mEntryPoint);
        shaderMapping.mTarget = yaget::json::GetValue(j, "Target", shaderMapping.mTarget);
    }

    using ShaderMappings = std::map<yaget::render::RenderShaders::ShaderType, ShaderMapping>;
    ShaderMappings ShaderOptionsMappings = {
        { yaget::render::RenderShaders::ShaderType::Vertex, { "VSMain", "vs_6_5" } },
        { yaget::render::RenderShaders::ShaderType::Pixel, { "PSMain", "ps_6_5" } },
        { yaget::render::RenderShaders::ShaderType::Geometry, { "GSMain", "gs_6_5" } },
        { yaget::render::RenderShaders::ShaderType::Compute, { "CSMain", "cs_6_5" } },
        { yaget::render::RenderShaders::ShaderType::Hull, { "HSMain", "hs_6_5" } },
        { yaget::render::RenderShaders::ShaderType::Domain, { "DSMain", "ds_6_5" } }
    };


    // we pass tag only for reporting/logging features
    yaget::render::ResourceCompiler::CompileResult CompileShader(const yaget::io::Tag& tag, yaget::render::ResourceCompiler* resourceCompiler, yaget::io::BufferView data, const yaget::Strings& parameters)
    {
        yaget::render::ResourceCompiler::CompileResult compiledResult;
        try
        {
            compiledResult = resourceCompiler->Compile(data, parameters);
            YLOG_INFO("COMP", "Compiled shader for: '%s'", yaget::conv::Convertor<yaget::io::Tag>::ToString(tag).c_str());
        }
        catch (const yaget::ex::bad_init& ex)
        {
            YLOG_ERROR("COMP", "Did not compiled shader: '%s'. Error: %s", yaget::conv::Convertor<yaget::io::Tag>::ToString(tag).c_str(), ex.what());
        }

        return compiledResult;
    }


    yaget::Strings GetCommandParameters(yaget::render::RenderShaders::ShaderType shaderType, bool debugShader)
    {
        yaget::Strings parameters;

        const char* entryName = ShaderOptionsMappings[shaderType].mEntryPoint.c_str();
        const char* target = ShaderOptionsMappings[shaderType].mTarget.c_str();

        parameters.push_back("-E");
        parameters.push_back(entryName);

        // -T for the target profile (eg. 'ps_6_6')
        parameters.push_back("-T");
        parameters.push_back(target);

        parameters.push_back("-encoding");
        parameters.push_back("utf8");
        parameters.push_back("-fdiagnostics-format=msvc");

        if (debugShader)
        {
            parameters.push_back("-Zi");
            parameters.push_back("-Qembed_debug");
            parameters.push_back("-Od");
        }

        return parameters;
    }


}


//-------------------------------------------------------------------------------------------------
namespace yaget::render
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
yaget::render::RenderShaders::RenderShaders(yaget::io::VirtualTransportSystem& vts)
    : CacheWatcher(vts, yaget::io::VirtualTransportSystem::Section("Caches@Shaders"))
    , mResourceCompiler(std::make_shared<ResourceCompiler>())
{
}


//-------------------------------------------------------------------------------------------------
yaget::render::RenderShaders::~RenderShaders() = default;


//-------------------------------------------------------------------------------------------------
yaget::io::Buffer yaget::render::RenderShaders::GetShader(const yaget::io::Tag& tag, ShaderType shaderType)
{
    YAGET_ASSERT(tag.IsValid(), "Tag: '%s:%s' is not valid.", yaget::conv::Convertor<yaget::Guid>::ToString(tag.mGuid).c_str(),
        yaget::conv::Convertor<yaget::io::Tag>::ToString(tag).c_str());
    auto result = GetShaders(io::Tags{ tag }, shaderType);
    return !result.empty() ? *result.begin() : io::Buffer{};
}


//-------------------------------------------------------------------------------------------------
std::vector<yaget::io::Buffer> yaget::render::RenderShaders::GetShaders(const yaget::io::Tags& tags, ShaderType shaderType)
{
    std::lock_guard mutexLocker(mMutex);
    std::vector<yaget::io::Buffer> results = tags | std::views::transform([this, shaderType](const auto& tag)
    {
        return AssureShaderNonMT(tag, shaderType);
    }) | std::ranges::to<std::vector>();
    return results;
}


//-------------------------------------------------------------------------------------------------
void yaget::render::RenderShaders::ClearCache(const io::Tag& tag)
{
    {
        std::lock_guard mutexLocker(mMutex);
        mReflections.erase(tag);
    }

    CacheWatcher<io::Buffer>::ClearCache(tag);
}


//-------------------------------------------------------------------------------------------------
void yaget::render::RenderShaders::CreateSignatureDescription(const io::Tag& vertexTag, const io::Tag& pixelTag, DescriptionCallback callback)
{
    auto getReflection = [this](io::Tag tag, ShaderType shaderType)
    {
        ResourceReflector::Ptr reflector;

        if (!mReflections.contains(tag))
        {
            auto asset = AssureShaderNonMT(tag, shaderType);
            reflector = mResourceCompiler->Decompile(io::cast_to_view(asset));
            mReflections.insert({ tag, reflector });
        }
        else
        {
            reflector = mReflections.find(tag)->second;
        }

        return reflector;
    };

    std::lock_guard mutexLocker(mMutex);
    ResourceReflector::Ptr vertexReflector = getReflection(vertexTag, ShaderType::Vertex);
    ResourceReflector::Ptr pixelReflector = getReflection(pixelTag, ShaderType::Pixel);

    YAGET_ASSERT(vertexReflector && pixelReflector, "There is no reflection record for vertex '%s' and/or pixel '%s' shaders.",
        conv::Convertor<io::Tag>::ToString(vertexTag).c_str(),
        conv::Convertor<io::Tag>::ToString(pixelTag).c_str());

    vertexReflector->MakeRootSignature(pixelReflector.get(), callback);
}


//-------------------------------------------------------------------------------------------------
void yaget::render::RenderShaders::PopulateShaderMappings(io::VirtualTransportSystem::Section fileName, io::VirtualTransportSystem& vts)
{
    PopulateMap(fileName, vts, ShaderOptionsMappings);
}


//-------------------------------------------------------------------------------------------------
void yaget::render::RenderShaders::SaveShaderMappings(io::VirtualTransportSystem::Section fileName, io::VirtualTransportSystem& vts)
{
    SaveMap(fileName, vts, ShaderOptionsMappings);
}


//-------------------------------------------------------------------------------------------------
yaget::io::Buffer yaget::render::RenderShaders::AssureShaderNonMT(const yaget::io::Tag& tag, yaget::render::RenderShaders::ShaderType shaderType)
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

    Strings arguments = GetCommandParameters(shaderType, true);;

    auto [buffer, reflection] = CompileShader(tag, mResourceCompiler.get(), io::cast_to_view(shaderBuffer), arguments);
    if (!io::size_data(buffer))
    {
        YLOG_ERROR("COMP",
            std::format("Could not get compiled {} shader for tag: '{}\n{}:'. Using built-in shader as s fallback.", magic_enum::enum_name(shaderType),
                yaget::conv::Convertor<yaget::io::Tag>::ToString(tag), tag.ResolveVTS()).c_str());

        arguments = GetCommandParameters(shaderType, false);;

        auto binaryCode = CompileShader(tag, mResourceCompiler.get(), io::BufferView(buildInShaderSource, buildInShaderSourceLen), arguments);
        buffer = binaryCode.first;
        reflection = binaryCode.second;

        error_handlers::ThrowOnError(io::size_data(buffer) > 0,
            std::format("Could not compile built-in shader type: '%s'. Source:\n'%s'", magic_enum::enum_name(shaderType),
                buildInShaderSource));
    }

    mAssets.insert({ tag, buffer });
    mCache.SaveCachedAsset(tag, buffer);
    mReflections.insert({ tag , reflection });
    return buffer;
}
