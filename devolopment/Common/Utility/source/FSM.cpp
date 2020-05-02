#include "FSM/FSM.h"


namespace eg {


FSM::FSM()
{
    // Initialize States
    mStateInitial.Set(this, &FSM::BeginStateInitial, &FSM::StateInitial, &FSM::EndStateInitial);
    // Initialize State Machine
    mpCurrentState=static_cast<FSMState *>(&mStateInitial);
    mpNewState = 0;
}


void FSM::Update()
{
    // Check New State
    if (mpNewState)
    {
        // Execute End State
        mpCurrentState->ExecuteEndState();
        // Set New State
        mpCurrentState = mpNewState;
        mpNewState = 0;
        // Execute Begin State
        mpCurrentState->ExecuteBeginState();
    }

    // Execute State
    mpCurrentState->ExecuteState();
}


bool FSM::IsStateInitial() const
{
    return IsState(mStateInitial);
}


bool FSM::IsState(const FSMState& state) const
{
    return (mpCurrentState == &state);
}


bool FSM::GotoState(FSMState& newState)
{
    // Set New State
    mpNewState = &newState;
    return true;
}


} // namespace eg



