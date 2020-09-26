//////////////////////////////////////////////////////////////////////
// RenderStateCache.h
//
//  Copyright 8/12/2019 Edgar Glowacki
//
//  Maintained by: Edgar
//
//  NOTES:
//      Provides management for render states, like setting active one, 
//      preventing Activate on already active state, reseting to default ones.
//
//
//  #include "Resources/RenderStateCache.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "YagetCore.h"
#include "Resources/ResourceView.h"
#include <typeindex>
#include <stack>


namespace yaget
{
    namespace render { class Device; }

    namespace render::state
    {
        using ResourcePtr = std::shared_ptr<ResourceView>;

        // -----------------------------------------------------------------------------------------------------------
        // For each device render frame, BeginFrame(...) and EndFrame() is called, where BeginFrame takes list of default states for this frame.
        // The default state list should have all possible types of ResourceView.
        // Then for each description/geometry/shader, call Begin, Set and then End
        // TODO: Allow call Begin/Set recursevly
        class RenderStateCache : public Noncopyable<RenderStateCache>
        {
        public:
            // Default - whatever makes sense to for this state in how to reset. In most cases it will be treaded as AutoReset.
            // AutoReset - revert to previous setting in this slot when End() is called.
            // Preserve - keep this state until another call to Set(...) for this same spot.
            enum class SetPolicy { Default, AutoReset, Preserve };

            // Set stateResource for it's slot to be the current one.
            // policy param dicates how to restore states if any when calling End()
            //void Set(ResourceView* stateResource, SetPolicy policy);
            void Set(const ResourcePtr& stateResource, SetPolicy policy);
            bool Activate();

            // Called in pairs for every geometry we want to render, which should encompass all render states
            void Begin();
            void End();

            // Call at the start of full render frame. This sets default render states for the duration of this frame
            // EndFrame() must be called in pairs
            bool BeginFrame(const std::vector<ResourcePtr>& defaultStates);
            void EndFrame();

        private:
            struct StateResourceData
            {
                std::shared_ptr<ResourceView> mState;
                SetPolicy mSetPolicy = SetPolicy::Default;
            };

            // we are keying on Hash value for ResourceView
            using StateBlocks = std::unordered_map<std::type_index, StateResourceData>;
            using PreviousStates = std::stack<StateBlocks>;

            StateBlocks mCurrentBlocks;
            StateBlocks mDefaultBlocks;

            PreviousStates mPreviousStates;
            bool mBeginStarted = false;
            bool mFrameStarted = false;
        };

        // -----------------------------------------------------------------------------------------------------------
        // Provides RAII for calling Begin and End methods. This is used around one geometry/shaders resources
        class DefaultStateCache
        {
        public:
            DefaultStateCache(RenderStateCache& renderStateCache) : mRenderStateCache(renderStateCache) 
            { mRenderStateCache.Begin(); }

            ~DefaultStateCache() { mRenderStateCache.End(); }

        private:
            RenderStateCache& mRenderStateCache;
        };

    } // namespace render::state
} // namespace yaget
