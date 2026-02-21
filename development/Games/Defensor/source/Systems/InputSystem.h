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
    // collects all actions triggered while matching set Input Context
    // it will iterate over InputComponents and try to match those triggered actions with component list.
    // It is up to other 'systems/components' to use this info for game play logic
    // It should be one of the first if not first Systems in SystemsCoordinator
    class ProcessInputSystem : public comp::gs::GameSystem<GameCoordinatorSet, comp::gs::GenerateEndMarker, defensor::game::Messaging, comp::InputComponent*>
    {
    public:
        ProcessInputSystem(Messaging& messaging, Application& app, GameCoordinatorSet& coordinatorSet, bool tickEnabled = true);

        // Constrain all incoming input to be associated with this specific context
        // If contextName empty, then it simply disables any input processing
        void SetContext(const std::string& contextName);

    private:
        void OnUpdate(comp::Id_t id, const time::GameClock& gameClock, metrics::Channel& channel, comp::InputComponent* inputComponent);

        void SaveAction(const std::string& actionName, uint64_t timeStamp, int32_t mouseX, int32_t mouseY, uint32_t flags);

        struct ActionInput
        {
            std::string mName;
            uint64_t mTimeStamp;
            int32_t mMouseX;
            int32_t mMouseY;
            uint32_t mFlags;
        };

        std::map<std::string, ActionInput> mActionInputs;

        std::string mActiveContextName;
    };


    // Clear all InputComponents acquired Actions this frame.
    // It should be one of the last if not last Systems in SystemsCoordinator
    class ClearInputSystem : public comp::gs::GameSystem<GameCoordinatorSet, comp::gs::NoEndMarker, defensor::game::Messaging, comp::InputComponent*>
    {
    public:
        ClearInputSystem(Messaging& messaging, Application& app, GameCoordinatorSet& coordinatorSet, bool tickEnabled = true);

    private:
        void OnUpdate(comp::Id_t id, const time::GameClock& gameClock, metrics::Channel& channel, comp::InputComponent* inputComponent);
    };
}
