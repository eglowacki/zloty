#include "Render/Pipeline/RenderTextures.h"
#include "Core/ErrorHandlers.h"
#include "Streams/Guid.h"
#include "VTS/ResolvedAssets.h"
#include "magic_enum/magic_enum.hpp"
#include "Json/JsonHelpers.h"
#include "Exception/Exception.h"
#include <cstddef>


//-------------------------------------------------------------------------------------------------
yaget::render::RenderTextures::RenderTextures(yaget::io::VirtualTransportSystem& vts)
    : CacheWatcher(vts, yaget::io::VirtualTransportSystem::Section("Caches@Textures"))
{
}


//-------------------------------------------------------------------------------------------------
yaget::render::RenderTextures::~RenderTextures() = default;


//-------------------------------------------------------------------------------------------------
yaget::io::Buffer yaget::render::RenderTextures::GetTexture(const yaget::io::Tag& tag)
{
    YAGET_ASSERT(tag.IsValid(), "Tag: '%s:%s' is not valid.", 
        yaget::conv::ToString(tag.mGuid).c_str(),
        yaget::conv::ToString(tag).c_str());

    auto result = GetTextures(io::Tags{ tag });
    return !result.empty() ? *result.begin() : io::Buffer{};
}


//-------------------------------------------------------------------------------------------------
std::vector<yaget::io::Buffer> yaget::render::RenderTextures::GetTextures(const yaget::io::Tags& tags)
{
    std::vector<io::Buffer> results;

    std::lock_guard mutexLocker(mMutex);

    for (auto tag : tags)
    {
        auto result = GetAsset(tag, [this, &results](auto tag, auto& cachedData)
        {
            if (!io::size_data(cachedData))
            {
                io::SingleBLobLoader<io::TextureAsset> loader(mVTS, tag);
                auto textureAsset = loader.GetAsset();
                cachedData = textureAsset ? textureAsset->mBuffer : io::Buffer{};
            }

            return cachedData;
        });
        
        results.push_back(result);
    }

    return results;
}


//-------------------------------------------------------------------------------------------------
void yaget::render::RenderTextures::PopulateTextureMappings(io::VirtualTransportSystem::Section /*fileName*/, io::VirtualTransportSystem& /*vts*/)
{
    //PopulateMap(fileName, vts, ShaderOptionsMappings);
}


//-------------------------------------------------------------------------------------------------
void yaget::render::RenderTextures::SaveTextureMappings(io::VirtualTransportSystem::Section /*fileName*/, io::VirtualTransportSystem& /*vts*/)
{
    //SaveMap(fileName, vts, ShaderOptionsMappings);
}
