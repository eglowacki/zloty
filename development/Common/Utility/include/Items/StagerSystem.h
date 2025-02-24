///////////////////////////////////////////////////////////////////////
// StagerSystem.h
//
//  Copyright 9/7/2024 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      Uses 'Stage' table from Director to manage and control
//		loading and unloading items/components.
//
//
//  #include "Items/StagerSystem.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "ItemsDirector.h"

#include "App/Application.h"

#include "Components/GameSystem.h"
#include "Items/StageComponent.h"

namespace yaget::items
{

    template <typename CS, typename M>
    class StagerSystem : public comp::gs::GameSystem<CS, comp::gs::NoEndMarker, M, StageComponent*>
    {
    public:
        StagerSystem(M& messaging, Application& app, CS& coordinatorSet)
            : comp::gs::GameSystem<CS, comp::gs::NoEndMarker, M, StageComponent*>("StagerSystem", messaging, app, [this](auto&&... params) {OnUpdate(params...); }, coordinatorSet)
            , mApp(app)
        {}

    private:
        void OnUpdate(comp::Id_t id, const time::GameClock& gameClock, metrics::Channel& channel, StageComponent* stageComponent);

        class Stager
        {
        public:
            Stager(const db_stage::StageName::Types& stageName, StagerSystem& stagerSystem)
                : mCurrentStage(stageName)
                , mStagerSystem(stagerSystem)
                , mItemIds(mStagerSystem.mApp.Director().GetStageItems(mCurrentStage))
            {
                for (const auto& id : mItemIds)
                {
                    //mStagerSystem.mApp.Director().LoadItemState<StagerSystem::Row>(id);
                }
            }

            ~Stager()
            {
                for (const auto& id : mItemIds)
                {
                    //mStagerSystem.RemoveComponents<StagerSystem::RowPolicy>(id);
                }
            }

            db_stage::StageName::Types mCurrentStage;
            StagerSystem& mStagerSystem;
            yaget::comp::ItemIds mItemIds;
        };

        Application& mApp;

        using StagersStack = std::stack<Stager>;
        StagersStack mStagersStack;
    };

    // --------------------------------------------------------------------
    // impl methods
    template <typename CS, typename M>
    void StagerSystem<CS, M>::OnUpdate(comp::Id_t id, const time::GameClock& gameClock, metrics::Channel& channel, StageComponent* stageComponent)
    {
        const auto& requestedStageName = stageComponent->GetValue<db_stage::StageName>();

        const auto currentStage = mStagersStack.empty() ? "" : mStagersStack.top().mCurrentStage;
        if (!requestedStageName.empty() && requestedStageName != currentStage)
        {
            // here we have choices on how to load requested stage
            //  leave current items loaded
            //  load requested ones
            mStagersStack.push({requestedStageName, *this});

            // ok at this point we need to know if any current items will be removed or will they stay to be merged with incoming (new) ones.
            // we could go even further and try to support updating component of existing ones.
            //const auto items = mApp.Director().GetStageItems(requestedStageName);
            //...

            //mCurrentStage = stageName;
        }
        int z = 0;
        z;
    }
} // namespace yaget::items


