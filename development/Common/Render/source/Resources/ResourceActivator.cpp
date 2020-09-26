#include "Resources/ResourceActivator.h"


void yaget::render::state::ResourceActivator::Set(const ResourcePtr& resource)
{
    auto& block = mResourceBlocks[resource->GetType()];
    if (!block.mResource || (block.mResource->GetStateHash() != resource->GetStateHash()))
    {
        block.mResource = resource;
        block.mDirty = true;
    }

    //auto rType = resource->GetType();
    //auto& blocks = mResourceBlocks[rType];
    //auto& dirtyBit = mDirtyBlocks[rType];

    //if (blocks.empty() || (!blocks.empty() && blocks.top()->GetStateHash() != resource->GetStateHash()))
    //{
    //    blocks.push(resource);
    //    dirtyBit = true;
    //}
}

bool yaget::render::state::ResourceActivator::Activate()
{
    for (auto& [key, block] : mResourceBlocks)
    {
        if (block.mDirty)
        {
            if (!block.mResource->Activate())
            {
                return false;
            }

            block.mDirty = false;
        }
    }

    return true;
}

void yaget::render::state::ResourceActivator::PushBlocks()
{
}

void yaget::render::state::ResourceActivator::PopBlocks()
{
}
