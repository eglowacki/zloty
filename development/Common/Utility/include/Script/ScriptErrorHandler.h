/////////////////////////////////////////////////////////////////////////
// ScriptErrorHandler.h
//
//  Copyright 4/6/2009 Edgar Glowacki.
//
// NOTES: This file does not include any headers and it's made to be
//        included by other source files where required header files
//        are included before it.
//
//
// #include "Script/ScriptErrorHandler.h"
//
/////////////////////////////////////////////////////////////////////////
//! \file

#ifndef SCRIPT_ERROR_HANDLER_H
#define SCRIPT_ERROR_HANDLER_H
#pragma once

#include "Logger/Log.h"
#include "luacpp.h"
#include <string>
#include <sstream>
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#pragma warning(push)
#pragma warning (disable : 4702)  //    : unreachable code
#pragma warning (disable : 4100)  // '' : unreferenced formal parameter
#include <luabind/luabind.hpp>
#include <luabind/object.hpp>
#pragma warning(pop)

namespace eg
{
    namespace script
    {
        inline std::string getStackType(lua_State *L, int index)
        {
            std::string textMsg;
            int t = lua_type(L, index);
            int top = lua_gettop(L);
            if (top == index)
            {
                textMsg += "  ";
            }
            else
            {
                textMsg += "\n   ";
            }

            int neg_index = -1 - (top - index);

            textMsg += boost::lexical_cast<std::string>(neg_index) + "|" + boost::lexical_cast<std::string>(index) + ": ";
            switch (t)
            {
            case LUA_TSTRING:  // strings
                textMsg += "<" + std::string(lua_typename(L, t)) + "> " + lua_tostring(L, index);
                break;

            case LUA_TBOOLEAN:  // booleans
                textMsg += "<" + std::string(lua_typename(L, t)) + "> " + (lua_toboolean(L, index) ? "true" : "false");
                break;

            case LUA_TNUMBER:  // numbers
                textMsg += "<" + std::string(lua_typename(L, t)) + "> " + boost::lexical_cast<std::string>(lua_tonumber(L, index));
                break;

            default:  // other values
                textMsg +=  "<" + std::string(lua_typename(L, t)) + "> " + lua_typename(L, t);
                break;

            }
            //textMsg += "\n";
            return textMsg;
        }

        inline void stackDump(lua_State *L)
        {
            std::string textMsg;
            int top = lua_gettop(L);
            //for (int i = 1; i <= top; i++)

            for (int i = top; i > 0; i--)
            {
                textMsg += getStackType(L, i);
            }
            log_trace("main") << "StackTrace:\n[" << textMsg << "]";
        }

        inline std::string stackdump(lua_State *L)
        {
            std::string textMsg;
            int top = lua_gettop(L);

            for (int i = top; i > 0; i--)
            {
                textMsg += getStackType(L, i);
            }

            textMsg = "StackTrace:\n[" + textMsg + "]";
            return textMsg;
            //log_trace("main") << "StackTrace:\n[" << textMsg << "]";
        }

        #define lua_stackdump(L) log_trace("main") << eg::script::stackdump(L)

        inline void printTable(lua_State *L, std::string& textMsg)
        {
            lua_pushnil(L);

            while(lua_next(L, -2) != 0)
            {
                if(lua_isstring(L, -1))
                {
                    textMsg += boost::str(boost::format("\n      %1% = %2%;") % lua_tostring(L, -2) % lua_tostring(L, -1));
                }
                else if(lua_isnumber(L, -1))
                {
                    textMsg += boost::str(boost::format("\n      %1% = %2%;") % lua_tostring(L, -2) % lua_tonumber(L, -1));
                }
                else if(lua_istable(L, -1))
                {
                    textMsg += boost::str(boost::format("\n      %1% = %2%;") % lua_tostring(L, -2) % "table");
                    //printTable(L, textMsg);
                }
                else
                {
                    textMsg += "unknown;";
                }

                lua_pop(L, 1);
            }
        }

        inline void tableDump(lua_State *L, const std::string& name = "table")
        {
            std::string textMsg;
            int t = lua_type(L, -1);
            if (t == LUA_TTABLE)
            {
                textMsg = "[  {" + name + "}";
                printTable(L, textMsg);
            }

            log_trace("main") << "TableDump:\n" << textMsg << "]";
        }

        inline void printTable(const luabind::object& table, std::string& textMsg)
        {
            for (luabind::iterator it(table), e; it != e; ++it)
            {
                luabind::object key = it.key();
                if (luabind::type(*it) == LUA_TSTRING)
                {
                    textMsg += boost::str(boost::format("\n      %1% = %2%;") % luabind::object_cast<std::string>(key) % luabind::object_cast<std::string>(*it));
                }
                else if (luabind::type(*it) == LUA_TNUMBER)
                {
                    textMsg += boost::str(boost::format("\n      %1% = %2%;") % luabind::object_cast<std::string>(key) % luabind::object_cast<double>(*it));
                }
                else if (luabind::type(*it) == LUA_TFUNCTION)
                {
                    textMsg += boost::str(boost::format("\n      %1% = %2%;") % luabind::object_cast<std::string>(key) % "function");
                }
                else if (luabind::type(*it) == LUA_TLIGHTUSERDATA)
                {
                    textMsg += boost::str(boost::format("\n      %1% = %2%;") % luabind::object_cast<std::string>(key) % "light_user_data");
                }
                else if (luabind::type(*it) == LUA_TUSERDATA)
                {
                    textMsg += boost::str(boost::format("\n      %1% = %2%;") % luabind::object_cast<std::string>(key) % "user_data");
                }
                else if (luabind::type(*it) == LUA_TTABLE)
                {
                    //textMsg += boost::str(boost::format("\n      %1% = %2%;") % luabind::object_cast<std::string>(key) % "table begin -->");
                    textMsg += boost::str(boost::format("\n      %1% = %2%;") % "key" % "table begin -->");
                    //textMsg += boost::str(boost::format("\n      %1% = %2%;") % lua_tostring(L, -2) % "table begin -->");
                    printTable(*it, textMsg);
                    //textMsg += boost::str(boost::format("\n      %1% = %2%;") % luabind::object_cast<std::string>(key) % "table end -->");
                    textMsg += boost::str(boost::format("\n      %1% = %2%;") % "key" % "table end -->");
                }
                else
                {
                    textMsg += boost::str(boost::format("\n      %1% = %2%;") % luabind::object_cast<std::string>(key) % "unknown");
                }
            }
        }


        inline void tableDump(const luabind::object& table, const std::string& name = "table")
        {
            std::string textMsg;
            textMsg = "[  {" + name + "}";
            printTable(table, textMsg);

            log_trace("main") << "TableDump:\n" << textMsg << "]";
        }

        //! called when executing lua chunk and any error occurred
        //! This will format error string full_path_file_name(line_number) : error blah blah
        //!       traceback
        inline int luaError(lua_State *L, bool fulldebug, std::string *sourceLine = 0)
        {
            lua_Debug d = {0};
            if (fulldebug)
            {
                lua_getstack(L, 1, &d);
                lua_getinfo(L, "Sln", &d);
            }
            std::string err = lua_tostring(L, -1);

            size_t secondMarker = err.find_first_of(':');
            if (secondMarker != std::string::npos)
            {
                if (err[secondMarker+1] == '/')
                {
                    secondMarker = err.find_first_of(':', secondMarker+1);
                }
                secondMarker = err.find_first_of(':', secondMarker+1);
            }

            std::string file_line;
            if (secondMarker != std::string::npos)
            {
                file_line = err.substr(0, secondMarker+1);
                err.erase(0, secondMarker+1);
            }
            boost::trim(err);

            lua_pop(L, 1);

            std::string file_name("nothing");
            int line_number = -1;

            if (!fulldebug)
            {
                size_t file_beg = file_line.find_first_of('"');
                size_t file_end = file_line.find_first_of('"', file_beg+1);
                if (file_beg != std::string::npos && file_end != std::string::npos)
                {
                    file_name = file_line.substr(file_beg+1, file_end-file_beg-1);
                    boost::trim(file_name);

                    size_t line_beg = file_line.find_first_of(':', file_end+1);
                    size_t line_end = file_line.find_first_of(':', line_beg+1);
                    std::string number = file_line.substr(line_beg+1, line_end-line_beg-1);
                    line_number = boost::lexical_cast<int>(number);
                }
            }

            if (d.source != 0)
            {
                if (std::string(d.source) == "=[C]" && !file_line.empty())
                {
                    // this happened in C code, so we need to extract source and
                    // line number from file_line we saved earlier.
                    size_t file_beg = file_line.find_first_of('"');
                    size_t file_end = file_line.find_first_of('"', file_beg+1);
                    if (file_beg != std::string::npos && file_end != std::string::npos)
                    {
                        file_name = file_line.substr(file_beg+1, file_end-file_beg-1);
                        boost::trim(file_name);

                        size_t line_beg = file_line.find_first_of(':', file_end+1);
                        size_t line_end = file_line.find_first_of(':', line_beg+1);
                        std::string number = file_line.substr(line_beg+1, line_end-line_beg-1);
                        line_number = boost::lexical_cast<int>(number);
                    }
                }
                else
                {
                    //msg << d.source << "(" << d.currentline << ") : error";
                    file_name = d.source;
                    line_number = d.currentline;
                }
            }

            std::stringstream msg;
            if (sourceLine)
            {
                msg << file_name << "(" << line_number << ")";
                *sourceLine = msg.str();
                return 1;
            }

            msg << file_name << "(" << line_number << ") : error";
            if (d.name != 0)
            {
                msg << "(" << d.namewhat << " " << d.name << ")";
            }

            msg << " " << err;
            err = msg.str();
            lua_pushstring(L, err.c_str());

            //-------------------------------------------------------
            // now, let's get the trace back
            lua_getfield(L, LUA_GLOBALSINDEX, "debug");
            if (!lua_istable(L, -1))
            {
                lua_pop(L, 2);
                err += "\n-- missing debug --";
                lua_pushstring(L, err.c_str());
                return 1;
            }
            lua_getfield(L, -1, "traceback");
            if (!lua_isfunction(L, -1))
            {
                lua_pop(L, 3);
                err += "\n-- missing traceback --";
                lua_pushstring(L, err.c_str());
                return 1;
            }
            lua_pushvalue(L, 1);
            lua_pushinteger(L, 2);
            lua_call(L, 2, 1);
            const char *stackTraceMsg = lua_tostring(L, -1);
            err = stackTraceMsg ? stackTraceMsg : err;
            lua_pop(L, 3);
            //-------------------------------------------------------

            lua_pushstring(L, err.c_str());
            return 1;
        }

        inline int luaError(lua_State *L)
        {
            return luaError(L, true, 0);
        }

        inline std::string luaGetSourceLine(lua_State *L)
        {
            std::string sourceLine;
            luaError(L, true, &sourceLine);
            return sourceLine;
        }

    } // namespace script
} // namespace eg

#endif // SCRIPT_ERROR_HANDLER_H

