/////////////////////////////////////////////////////////////////////////
// bind_log.cpp
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
#include "App/AppUtilities.h"
#include "Logger/Log.h"
#include "Script/luacpp.h"
#pragma warning(push)
#pragma warning (disable : 4100)  // '' : unreferenced formal parameter
    #include <luabind/luabind.hpp>
#pragma warning(pop)


namespace
{

}


namespace eg { namespace script {

void init_app(lua_State *L)
{
    using namespace luabind;

    module(L, "yaget")
    [
        def("sleep", (void(*)(double))&util::sleep),
        def("appPath", &util::getAppPath),
        def("appName", &util::getAppName),
        def("appFullPath", &util::getAppFullPath)
    ];

}

}} // namespace script // namespace eg



