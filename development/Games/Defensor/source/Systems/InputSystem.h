///////////////////////////////////////////////////////////////////////
// InputSystem.h
//
//  Copyright 6/23/2025 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      System which iterates and tick over inputs
//          InputProcess... does fill in all InputComponents with current action inputs
//          InputClear... will clear all InputComponents from last frame
//
//
//  #include "Systems/InputSystem.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once


#include "Components/GameSystem.h"
#include "DefensorGameTypes.h"

namespace defensor::game
{
    class ProcessInputSystem : public comp::gs::GameSystem<GameCoordinatorSet, comp::gs::GenerateEndMarker, defensor::game::Messaging, comp::InputComponent*>
    {
    public:
        ProcessInputSystem(Messaging& messaging, Application& app, GameCoordinatorSet& coordinatorSet);

    private:
        void OnUpdate(comp::Id_t id, const time::GameClock& gameClock, metrics::Channel& channel, comp::InputComponent* inputComponent);

        void SaveAction(const std::string& actionName, uint64_t timeStamp, int32_t mouseX, int32_t mouseY, uint32_t flags);
    };


    class ClearInputSystem : public comp::gs::GameSystem<GameCoordinatorSet, comp::gs::NoEndMarker, defensor::game::Messaging, comp::InputComponent*>
    {
    public:
        ClearInputSystem(Messaging& messaging, Application& app, GameCoordinatorSet& coordinatorSet);

    private:
        void OnUpdate(comp::Id_t id, const time::GameClock& gameClock, metrics::Channel& channel, comp::InputComponent* inputComponent);
    };
}
