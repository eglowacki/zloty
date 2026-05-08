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
        StagerSystem(const char* niceName, M& messaging, Application& app, CS& coordinatorSet, bool tickEnabled)
            : comp::gs::GameSystem<CS, comp::gs::NoEndMarker, M, StageComponent*>(niceName, messaging, app, [this](auto&&... params) {OnUpdate(params...); }, coordinatorSet, tickEnabled)
            , mDirector(app.Director())
        {
        }

    private:
        void OnUpdate(comp::Id_t id, const time::GameClock& gameClock, metrics::Channel& channel, const StageComponent* stageComponent);

        class Stager
        {
        public:
            Stager(const db_stage::Name::Types& stageName, const db_stage::Blend::Types& blend, const comp::ItemIds& itemIds, StagerSystem& stagerSystem)
                : mCurrentStage(stageName)
                , mBlend(blend)
                , mItemIds(itemIds)
                , mStagerSystem(stagerSystem)
            {

                for (const auto& id : mItemIds)
                {
                    mStagerSystem.GetCS().LoadItem(id);
                }
            }

            ~Stager()
            {
                for (const auto& id : mItemIds)
                {
                    mStagerSystem.GetCS().RemoveItem(id);
                }
            }

            db_stage::Name::Types mCurrentStage;
            db_stage::Blend::Types mBlend;
            comp::ItemIds mItemIds;
            StagerSystem& mStagerSystem;
        };

        items::Director& mDirector;

        using StagersStack = std::stack<std::unique_ptr<Stager>>;
        StagersStack mStagersStack;
    };

    // --------------------------------------------------------------------
    // impl methods
    template <typename CS, typename M>
    void StagerSystem<CS, M>::OnUpdate(comp::Id_t /*id*/, const time::GameClock& gameClock, metrics::Channel& /*channel*/, const StageComponent* stageComponent)
    {
        gameClock;
        const auto requestedStageName = stageComponent->GetValue<db_stage::Name>();

        const auto currentStage = mStagersStack.empty() ? "" : mStagersStack.top()->mCurrentStage;

        bool isNameSet = !requestedStageName.empty();
        bool isNameDifferent = requestedStageName != currentStage;

        isNameSet;
        isNameDifferent;

        if (!requestedStageName.empty() && requestedStageName != currentStage)
        {
            // ok at this point we need to know if any current items will be removed or will they stay to be merged with incoming (new) ones.
            // we could go even further and try to support updating component of existing ones.

            const auto items = mDirector.GetStageItems(requestedStageName);
            if (!items.empty())
            {
                for (const auto& id : items)
                {
                    auto item = comp::gs::GameSystem<CS, comp::gs::NoEndMarker, M, StageComponent*>::GetCS().LoadItem(id);
                    item;
                    int z = 0;
                    z;


                }

                const auto blend = stageComponent->GetValue<db_stage::Blend>();
                if (blend == db_stage::BlendOp::Replace)
                {
                    if (!mStagersStack.empty())
                    {
                        mStagersStack.pop();
                    }

                    mStagersStack.push(std::make_unique<Stager>(requestedStageName, blend, items, *this));
                }

                // here we have choices on how to load requested stage
                //  leave current items loaded
                //  and load requested ones
                //mStagersStack.push(Stager{ requestedStageName, stageComponent->GetValue<db_stage::Blend>(), *this });
            }
        }
    }
} // namespace yaget::items


