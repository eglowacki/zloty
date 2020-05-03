///////////////////////////////////////////////////////////////////////
// FSM.h
//
//  Copyright 5/18/2006 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      Finite State Machine using function pointers
//      based on GPG 3 by Charles Farris
//
//
//  #include <FSM\FSM.h>
//
//////////////////////////////////////////////////////////////////////
//! \file

#ifndef FSM_FSM_H
#define FSM_FSM_H
#pragma once


#include "FSMState.h"

namespace eg
{
    // FSM Class
    class FSM
    {
    public:
        FSM();
        virtual ~FSM() {}

        // Global Functions
        virtual void Update();

        //! Return TRUE if state is the current one
        bool IsState(const FSMState& state) const;
        //! Go to newState, and do all the transitions
        bool GotoState(FSMState& newState);

    protected:
        //! Helper method to see if we are in initial state.
        bool IsStateInitial() const;

    private:
        virtual void BeginStateInitial() {}
        virtual void StateInitial() {}
        virtual void EndStateInitial() {}

        // Current State
        FSMState *mpCurrentState;
        // New State
        FSMState *mpNewState;
        // Initial State
        FSMStateTemplate<FSM> mStateInitial;
    };

} // namespace eg

#endif // FSM_FSM_H

