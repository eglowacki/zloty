/////////////////////////////////////////////////////////////////////////
// lb_log.cpp
//
//  Copyright 4/8/2009 Edgar Glowacki.
//
// NOTES:
//      Provides bindings for log output from lua
//
//
//
/////////////////////////////////////////////////////////////////////////
//! \file
//#include "Logger/Log.h"
#include "Script/ScriptErrorHandler.h"
#include "Script/luacpp.h"
#pragma warning(push)
#pragma warning (disable : 4100)  // '' : unreferenced formal parameter
    #include <luabind/luabind.hpp>
#pragma warning(pop)


namespace eg {namespace script
{
    std::string luaGetSourceLine2(lua_State *L)
    {
        std::string line("empty(-1)");
        lua_Debug d = {0};
        lua_getstack(L, 1, &d);
        lua_getinfo(L, "Sln", &d);
        std::string file_name("empty");
        int line_number = -1;

        if (d.source != 0)
        {
            file_name = d.source;
            line_number = d.currentline;

            std::stringstream msg;
            msg << file_name << "(" << line_number << ")";
            line = msg.str();
        }

        return line;
    }
}} // namespace script } // namespace eg

namespace
{


    //void lb_log_trace(const char *msg)
    void lb_log_trace(const luabind::object& msg)
    {
        if (luabind::type(msg) == LUA_TSTRING)
        {
            lua_State *L = msg.interpreter();
            std::string lua_file_number = eg::script::luaGetSourceLine2(L);
            log_trace(tr_script) << "[lua:]" << "\n" << lua_file_number << " : " << luabind::object_cast<const char *>(msg) << "";
        }
    }

    void lb_log_debug(const char *msg)
    {
        if (msg && *msg)
        {
            log_debug << msg;
        }
    }

    void lb_log_warning(const char *msg)
    {
        if (msg && *msg)
        {
            log_warning << msg;
        }
    }

    void lb_log_error(const char *msg)
    {
        if (msg && *msg)
        {
            log_error << msg;
        }
    }
}


namespace eg { namespace script {

void init_log(lua_State *L)
{
    using namespace luabind;

    module(L, "log")
    [
        def("trace", &lb_log_trace),
        def("debug", &lb_log_debug),
        def("warning", &lb_log_warning),
        def("error", &lb_log_error)
    ];
}

}} // namespace script // namespace eg


