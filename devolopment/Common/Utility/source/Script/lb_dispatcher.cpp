/////////////////////////////////////////////////////////////////////////
// lb_dispatcher.cpp
//
//  Copyright 4/11/2009 Edgar Glowacki.
//
// NOTES:
//      Provides bindings for functions in app namespace
//
//
//
/////////////////////////////////////////////////////////////////////////
//! \file
#include "Message/Dispatcher.h"
#include "Input/InputManager.h"
#include "MessageInterface.h"
#include "Script/ScriptErrorHandler.h"
//#include <new>
//#include <luabind/adopt_policy.hpp>


using namespace eg;

namespace
{
    const int kNumTries = 5;





} // namespace


namespace eg { namespace script {

void init_dispatcher(lua_State * /*L*/)
{
    using namespace luabind;
}


}} // namespace script // namespace eg



