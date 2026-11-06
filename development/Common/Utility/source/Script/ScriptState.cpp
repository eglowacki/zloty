#include "Script/ScriptState.h"


//--------------------------------------------------------------------------------------------------
sol::state yaget::script::GetState()
{
    sol::state lua;

#if 0
	// print, assert, and other base functions
	base,
	// require and other package functions
	package,
	// coroutine functions and utilities
	coroutine,
	// string library
	string,
	// functionality from the OS
	os,
	// all things math
	math,
	// the table manipulator and observer functions
	table,
	// the debug library
	debug,
	// input/output library
	io,
	// library for handling utf8: new to Lua
	utf8,
#endif

    lua.open_libraries(sol::lib::base, sol::lib::package, sol::lib::coroutine, sol::lib::string, sol::lib::os, sol::lib::math, sol::lib::table, sol::lib::debug, sol::lib::io);
    return lua;
}
