///////////////////////////////////////////////////////////////////////
// StagerSystem.h
//
//  Copyright 02/12/2025 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      Provides games specific loading, unloading and lifetime management of Components/Items
//
//
//  #include "StagerSystem.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "DefensorGameTypes.h"
#include "Items/StagerSystem.h"

namespace defensor::game
{
    class DefensorStagerSystem : public yaget::items::StagerSystem<GameCoordinatorSet, Messaging>
    {
    public:
        DefensorStagerSystem(Messaging& messaging, Application& app, GameCoordinatorSet& coordinatorSet)
            : StagerSystem<GameCoordinatorSet, Messaging>("DefensorStagerSystem", messaging, app, coordinatorSet, true)
        {
            mMessaging.Listen<items::StageEvent>([this](const auto& event)
            {
                UpdateStageComponent(event.mName, event.mBlend);

            }, Messaging::DispatcherType::Logic);
        }

    private:
        void UpdateStageComponent(const items::db_stage::Name::Types& stageName, items::db_stage::Blend::Types stageBlend);
    };

}
