#include "Resources/RenderStateCache.h"
#include <typeindex>


bool yaget::render::state::RenderStateCache::BeginFrame(const std::vector<ResourcePtr>& defaultStates)
{
    YAGET_ASSERT(!mFrameStarted, "Missing call to EndFrame() before calling BeginFrame() again.");
    YAGET_ASSERT(mPreviousStates.empty(), "Previous blocks left when calling BeginFrame()");

    mCurrentBlocks = {};
    mDefaultBlocks = {};
        
    std::transform(std::begin(defaultStates), std::end(defaultStates), std::inserter(mDefaultBlocks, std::end(mDefaultBlocks)), [](const ResourcePtr& state)
    {
        return std::make_pair(state->GetType(), StateResourceData{ state, SetPolicy::Preserve });
    });

    std::size_t failed = mDefaultBlocks.size();
    std::for_each(std::begin(mDefaultBlocks), std::end(mDefaultBlocks), [&failed](const auto& block)
    { 
        failed -= block.second.mState->Activate() ? 1 : 0;
    });

    mFrameStarted = failed == 0;
    return mFrameStarted;
}


void yaget::render::state::RenderStateCache::EndFrame()
{
    YAGET_ASSERT(mFrameStarted, "Missing call to BeginFrame() before calling EndFrame().");
    mFrameStarted = false;
}


void yaget::render::state::RenderStateCache::Set(const ResourcePtr& stateResource, SetPolicy policy)
{
    YAGET_ASSERT(stateResource, "Incoming resource is null.");
    YAGET_ASSERT(mFrameStarted, "Missing call to Begin() before calling Set() for resource type: '%d'.", stateResource->GetType());
    YAGET_ASSERT(mDefaultBlocks.find(stateResource->GetType()) != mDefaultBlocks.end(), "Incoming resource: '%d' does not exist in default set.", stateResource->GetType());

    // let's see if incoming stateResource type is already in mCurrentBlocks, and if there is one check state hash for differences
    if (const auto& it = mCurrentBlocks.find(stateResource->GetType()); it != mCurrentBlocks.end())
    {
        if (it->second.mState->GetStateHash() != stateResource->GetStateHash())
        {
            mCurrentBlocks[stateResource->GetType()] = { stateResource, policy };
        }
    }
    else
    {
        // there is no entry for this type in mCurrentBlocks, simply set incoming into the slot
        mCurrentBlocks[stateResource->GetType()] = { stateResource, policy };
    }
}


bool yaget::render::state::RenderStateCache::Activate()
{
    for (const auto&[blockType, block] : mCurrentBlocks)
    {
        if (!block.mState->Activate())
        {
            return false;
        }
    }

    return true;
}


void yaget::render::state::RenderStateCache::Begin()
{
    YAGET_ASSERT(mFrameStarted, "Missing call to BeginFrame() before calling Begin().");
    YAGET_ASSERT(!mBeginStarted, "Missing call to End() before calling Begin() again.");

    mBeginStarted = true;
    mPreviousStates.push(mCurrentBlocks);
    mCurrentBlocks = {};
}


void yaget::render::state::RenderStateCache::End()
{
    YAGET_ASSERT(mFrameStarted, "Missing call to BeginFrame() before calling End().");
    YAGET_ASSERT(mBeginStarted, "Missing call to Begin() before calling End().");
    YAGET_ASSERT(!mPreviousStates.empty(), "Missing previous states in call to End().");

    StateBlocks newBlocks = mPreviousStates.top();
    mPreviousStates.pop();

    for (const auto& [blockType, block] : mCurrentBlocks)
    {
        // Activate state in newBlocks. If there is no state in newBlock, look in default ones, activate it and then assign to newBlocks.
        if (block.mSetPolicy == SetPolicy::AutoReset)
        {
            if (const auto& it = newBlocks.find(blockType); it != std::end(newBlocks))
            {
                it->second.mState->Activate();
            }
            else
            {
                mDefaultBlocks[blockType].mState->Activate();
            }
        }
        else if (block.mSetPolicy == SetPolicy::Preserve || block.mSetPolicy == SetPolicy::Default)
        {
            // we want to keep this state, so just assign to newBlocks
            newBlocks[blockType] = block;
        }
    }

    mCurrentBlocks = newBlocks;
    mBeginStarted = false;
}
