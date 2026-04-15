#include "Render/Pipeline/PipelineTags.h"
#include "ThreadModel/Condition.h"
#include "VTS/ResolvedAssets.h"
#include "VTS/VirtualTransportSystem.h"


//-------------------------------------------------------------------------------------------------
yaget::render::PipelineTags::PipelineTags(io::VirtualTransportSystem& vts)
    : mVTS(vts)
{
}


//-------------------------------------------------------------------------------------------------
yaget::render::PipelineTags::~PipelineTags() = default;


//-------------------------------------------------------------------------------------------------
yaget::io::Tag yaget::render::PipelineTags::ResolveTag(uint64_t hashValue, const std::string& tagName)
{
    {
        mt::ReadLock locker(mMutex);
        if (auto it = mResolveTags.find(hashValue); it != mResolveTags.end())
        {
            return it->second;
        }
    }
    
    mt::WriteLock locker(mMutex);
    if (auto it = mResolveTags.find(hashValue); it != mResolveTags.end())
    {
        return it->second;
    }

    io::VirtualTransportSystem::Section section("Transient@" + tagName);
    auto tag = mVTS.AssureTag(section);
    std::shared_ptr<io::Asset> newAsset = io::ResolveAsset<io::BinAsset>({}, tag, mVTS);
    mVTS.AttachTransientBlob(newAsset);

    mResolveTags[hashValue] = tag;

    return tag;
}


//-------------------------------------------------------------------------------------------------
yaget::io::Tag yaget::render::PipelineTags::GetTag(uint64_t hashValue) const
{
    mt::ReadLock locker(mMutex);
    if (auto it = mResolveTags.find(hashValue); it != mResolveTags.end())
    {
        return it->second;
    }

    return {};
}
