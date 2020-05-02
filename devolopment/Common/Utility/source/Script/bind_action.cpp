// bind_action.cpp
#include "Message/Dispatcher.h"
#include "MessageInterface.h"
#include "Input/InputManager.h"
#include "Script/ScriptErrorHandler.h"

using namespace eg;

namespace
{
    const int kNumTries = 5;


    //! Functor used to connect to any input action
    class LuaActionFunctor
    {
        // no copy semantics
        LuaActionFunctor(const LuaActionFunctor&);
        LuaActionFunctor& operator=(const LuaActionFunctor&);

    public:
        LuaActionFunctor(InputManager& input, const std::string& name)
        : mErrorCounter(kNumTries)
        {
            mConnection = input.ListenForAction(name, boost::ref(*this));
        }

        ~LuaActionFunctor() {}

        void attach(const luabind::object& callback)
        {
            mCallback = callback;
        }

        void operator()(const std::string& actionName, uint32_t timeStamp, int32_t mouseX, int32_t mouseY)
        {
            if (!mCallback || !mErrorCounter)
            {
                return;
            }

            try
            {
                luabind::call_function<void>(mCallback, actionName, timeStamp, mouseX, mouseY);
                mErrorCounter = kNumTries;
            }
            catch (luabind::error& e)
            {
                mErrorCounter--;
                lua_State* L = e.state();
                script::luaError(L, false);
                luabind::object error_msg(luabind::from_stack(L, -1));
                log_error << "Script runtime error, tries left(" << mErrorCounter << "):\n" << error_msg << ">";
                lua_pop(L, 1);
            }
        }

    private:
        luabind::object mCallback;
        // we only want to log error on first exception
        // since last good run
        int mErrorCounter;
        InputManager::ActionListener_t mConnection;
    };

    // function __index(object, key)
    // value = object.key
    int lua_index(lua_State * /*L*/)     // object, key (value = object.key)
    {
        // return one value from the top of the stack
        return 0;
    }


    // object.key = value
    int lua_newindex(lua_State* L) // object, key, value (object.key = value)
    {
        // get object's metatable
        if (LuaActionFunctor *functor = (LuaActionFunctor *)luaL_checkudata(L, 1, "yaget.dispatcher.input"))
        {
            std::string key = luaL_checkstring(L, 2);
            if (key == "connect")
            {
                if (lua_isnil(L, 3))
                {
                    functor->attach(luabind::object());
                    return 1;
                }
                else if (lua_isfunction(L, 3))
                {
                    functor->attach(luabind::object(luabind::from_stack(L, 3)));
                    return 1;
                }
                else
                {
                    return luaL_error(L, "Invalid value set on connect [function|nil]");
                }
            }
            return luaL_error(L, "Invalid '%s' property called [connect]", key.c_str());
        }

        return luaL_error(L, "Input object nil.");
    }


    // collect garbage
    int lua_gc(lua_State* L) // object
    {
        // get object's metatable
        if (LuaActionFunctor *functor = (LuaActionFunctor *)luaL_checkudata(L, 1, "yaget.dispatcher.input"))
        {
            functor->~LuaActionFunctor();
        }

        // return one value from the top of the stack
        return 1;
    }

    // will return connection action (input connection) which then can be manipulated
    // and used
    int action(lua_State* L)                                            // name
    {
        //lua_stackdump(L);
        std::string name = luaL_checkstring(L, 1);

        luabind::object g = luabind::globals(L);
        luabind::object lua_yaget = luabind::gettable(g, "yaget");
        InputManager *input = luabind::object_cast<InputManager *>(lua_yaget["input"]);

        void *conUserData = lua_newuserdata(L, sizeof(LuaActionFunctor));  // name, udata
        /*LuaActionFunctor *act =*/ new(conUserData)LuaActionFunctor(*input, name);

        luaL_getmetatable(L, "yaget.dispatcher.input");                 // name, udata, mt
        lua_setmetatable(L, -2);                                        // name, udata

        // return the user data
        return 1;
    }

} // namespace



namespace eg { namespace script {

void init_action(lua_State *L)
{
    using namespace luabind;

    // Create and fill the metatable
    luaL_newmetatable(L, "yaget.dispatcher.input");     // mt

    lua_pushstring(L, "__index");                       // mt, "__index"
    lua_pushcfunction(L, &lua_index);                   // mt, "__index", lua_index
    lua_rawset(L, -3);                                  // mt

    lua_pushstring(L, "__newindex");                    // mt, "__newindex"
    lua_pushcfunction(L, &lua_newindex);                // mt, "__newindex", lua_newindex
    lua_rawset(L, -3);                                  // mt

    lua_pushstring(L, "__gc");                          // mt, "__gc"
    lua_pushcfunction(L, &lua_gc);                      // mt, "__gc", lua_gc
    lua_rawset(L, -3);                                  // mt

    lua_pop(L, 1);                                      //

    lua_pushstring(L, "action");                        // "action"
    lua_pushcfunction(L, &action);                      // "action", action
    lua_rawset(L, LUA_GLOBALSINDEX);                    //
}


}} // namespace script // namespace eg

