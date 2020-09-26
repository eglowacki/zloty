//////////////////////////////////////////////////////////////////////
// ResourceActivator.h
//
//  Copyright 9/15/2019 Edgar Glowacki
//
//  Maintained by: Edgar
//
//  NOTES:
//      Provides management of resources and render states to minimize
//      state changes.
//
//
//  #include "Resources/ResourceActivator.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "YagetCore.h"
#include "Resources/ResourceView.h"


namespace yaget
{
    namespace render::state
    {

        class ResourceActivator : public Noncopyable<ResourceActivator>
        {
        public:
            using ResourcePtr = std::shared_ptr<ResourceView>;

            void Set(const ResourcePtr& resource);
            bool Activate();

            void PushBlocks();
            void PopBlocks();

        private:
            struct ResourceBlock
            {
                ResourcePtr mResource;
                bool mDirty = false;
            };

            using ResourceBlocks = std::unordered_map<std::type_index, ResourceBlock>;
            ResourceBlocks mResourceBlocks;

            //using ResourceStack = std::stack<ResourcePtr>;

            //using ResourceBlocks = std::unordered_map<std::type_index, ResourceStack>;
            //using DirtyBlocks = std::unordered_map<std::type_index, bool>;

            //ResourceBlocks mResourceBlocks;
            //DirtyBlocks mDirtyBlocks;
        };

    } // namespace render::state
} // namespace yaget
