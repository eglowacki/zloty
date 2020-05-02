///////////////////////////////////////////////////////////////////////
// FSMState.h
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
//  #include <FSM/FSMState.h>
//
//////////////////////////////////////////////////////////////////////
//! \file

#ifndef FSM_FSMSTATE_H
#define FSM_FSMSTATE_H
#pragma once

#include <Base.h>


namespace eg
{
    class FSMState
    {
    public:
        // Destructor
        virtual ~FSMState() {}

        //@{{
        //! State Functions
        virtual void ExecuteBeginState() = 0;
        virtual void ExecuteState() = 0;
        virtual void ExecuteEndState() = 0;
        //@}}
    };


    template <typename T>
    class FSMStateTemplate : public FSMState
    {
    public:
        typedef void (T::*PFNSTATE)(void);

        FSMStateTemplate();

        //! Initialize Functions
        void Set(T *pInstance, PFNSTATE pfnBeginState, PFNSTATE pfnState, PFNSTATE pfnEndState);

        // from FSMState
        virtual void ExecuteBeginState()
        {
            // Begin State
            (mpInstance->*mpfnBeginState)();
        }

        virtual void ExecuteState()
        {
            // State
            (mpInstance->*mpfnState)();
        }

        virtual void ExecuteEndState()
        {
            // End State
            (mpInstance->*mpfnEndState)();
        }

    private:
        //! Instance Pointer
        T *mpInstance;
        //! State Function Pointer
        PFNSTATE mpfnBeginState;
        //! State Function Pointer
        PFNSTATE mpfnState;
        //! State Function Pointer
        PFNSTATE mpfnEndState;
    };


    // inline implementation
    template <typename T>
    inline FSMStateTemplate<T>::FSMStateTemplate() :
        mpInstance(0),
        mpfnBeginState(0),
        mpfnState(0),
        mpfnEndState(0)
    {}


    template <typename T>
    inline void FSMStateTemplate<T>::Set(T *pInstance, typename FSMStateTemplate<T>::PFNSTATE pfnBeginState, typename FSMStateTemplate<T>::PFNSTATE pfnState, typename FSMStateTemplate<T>::PFNSTATE pfnEndState)
    {
        // set instance
        mpInstance = pInstance;

        // Set Function Pointers
        mpfnBeginState = pfnBeginState;

        mpfnState = pfnState;

        mpfnEndState = pfnEndState;
    }

} // namespace eg

#endif // FSM_FSMSTATE_H


