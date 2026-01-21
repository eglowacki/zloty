#include "Render/Cache/CacheWatcher.h"


//-------------------------------------------------------------------------------------------------
yaget::render::CacheWatcher::CacheWatcher(io::VirtualTransportSystem& vts, io::VirtualTransportSystem::Section fileName)
    : mVTS(vts)
    , mCache(mVTS, fileName)
{
}


//-------------------------------------------------------------------------------------------------
yaget::render::CacheWatcher::~CacheWatcher() = default;


//-------------------------------------------------------------------------------------------------
void yaget::render::CacheWatcher::AssureTagWatch(const yaget::io::Tag& tag, std::function<void(const yaget::io::Tag&)> callback)
{
    if (!mWatchedTags.contains(tag))
    {
        if (auto shaderFilePath = tag.ResolveVTS(); !shaderFilePath.empty())
        {
            mWatchedTags.insert(tag);

            mWatcher.Add(tag.Hash(), shaderFilePath, [this, tag, callback]()
            {
                callback(tag);
            });
        }
    }
}

void yaget::render::CacheWatcher::ClearTagWatch(const yaget::io::Tag& tag)
{
    if (mWatchedTags.contains(tag))
    {
        mWatchedTags.erase(tag);
        mWatcher.Remove(tag.Hash());
    }
}
