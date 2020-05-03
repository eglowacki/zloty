// bind_timer.cpp
#include "Message/Dispatcher.h"
#include "MessageInterface.h"
#include "Script/ScriptErrorHandler.h"
#include "Profiler/Profiler.h"
#include <queue>

using namespace eg;

namespace
{
    const int kNumTries = 5;

    class LuaTimers
    {
        // no copy semantics
        LuaTimers(const LuaTimers&);
        LuaTimers& operator=(const LuaTimers&);

    public:
        LuaTimers(Dispatcher& dsp)
        {
            mConnection = dsp[mtype::kFrameTick].connect(boost::ref(*this));
        }

        ~LuaTimers() {mConnection.disconnect();}

        struct Functor
        {
            Functor() : rate(0), repeat(false), current(0), errorCounter(kNumTries)
            {}

            luabind::object callback;
            double rate;
            bool repeat;
            double current;
            int errorCounter;
        };

        size_t nextIndex()
        {
            if (!mFreeIndexes.empty())
            {
                size_t index = mFreeIndexes.front();
                mFreeIndexes.pop();
                return index;
            }

            mFunctors.push_back(Functor());
            return mFunctors.size()-1;
        }

        void freeIndex(size_t index)
        {
            // reset this index to null
            // and add it to free pool (queue)
            mFunctors[index] = Functor();
            mFreeIndexes.push(index);
        }

        Functor& operator[](size_t index)
        {
            if (index+1 > mFunctors.size())
            {
                mFunctors.resize(index+1);
            }

            return mFunctors[index];
        }

        void operator()(const Message& msg)
        {
            Prof(LuaTimer);
            if (!msg.Is<float>())
            {
                return;
            }

            float deltaTime = msg.GetValue<float>();
            for (std::vector<Functor>::iterator it = mFunctors.begin(); it != mFunctors.end(); ++it)
            {
                Functor& functor = *it;
                if (!functor.callback || !functor.errorCounter)
                {
                    continue;
                }

                functor.current += deltaTime;
                if (functor.current >= functor.rate)
                {
                    try
                    {
                        Prof(LuaFunction);
                        luabind::call_function<void>(functor.callback, functor.current);
                        functor.errorCounter = kNumTries;
                    }
                    catch (luabind::error& e)
                    {
                        functor.errorCounter--;
                        lua_State* L = e.state();
                        script::luaError(L, false);
                        luabind::object error_msg(luabind::from_stack(L, -1));
                        log_error << "Script runtime error, tries left(" << functor.errorCounter << "):\n" << error_msg << ">";
                        lua_pop(L, 1);
                    }

                    functor.current = 0;
                    if (!functor.repeat)
                    {
                        functor.callback = luabind::object();
                    }
                }
            }
        }

    private:
        boost::signals::connection mConnection;
        std::vector<Functor> mFunctors;
        std::queue<size_t> mFreeIndexes;
    };

    boost::shared_ptr<LuaTimers> Timers;


    // function __index(object, key)
    // value = object.key
    int lua_index(lua_State * /*L*/)     // object, key (value = object.key)
    {
        // return one value from the top of the stack
        return 0;
    }


    int lua_newindex(lua_State* L) // object, key, value (object.key = value)
    {
        // get object's metatable
        if (size_t *index = (size_t *)luaL_checkudata(L, 1, "yaget.dispatcher.timer"))
        {
            std::string key = luaL_checkstring(L, 2);
            if (key == "connect")
            {
                if (lua_isnil(L, 3))
                {
                    LuaTimers::Functor& functor = (*Timers)[*index];
                    functor.callback = luabind::object();
                    return 1;
                }
                else if (lua_isfunction(L, 3))
                {
                    LuaTimers::Functor& functor = (*Timers)[*index];
                    functor.callback = luabind::object(luabind::from_stack(L, 3));
                    return 1;
                }
                else
                {
                    return luaL_error(L, "Invalid value set on connect [function|nil]");
                }
            }
            else if (key == "rate") // when to fire next (in seconds)
            {
                if (lua_isnumber(L, 3))
                {
                    LuaTimers::Functor& functor = (*Timers)[*index];
                    functor.rate = lua_tonumber(L, 3);
                    return 1;
                }
                else
                {
                    return luaL_error(L, "Invalid value set on rate [number]");
                }
            }
            else if (key == "repeat")
            {
                if (lua_isboolean(L, 3))
                {
                    LuaTimers::Functor& functor = (*Timers)[*index];
                    functor.repeat = lua_toboolean(L, 3) == 1;
                    return 1;
                }
                else
                {
                    return luaL_error(L, "Invalid value set on repeat [boolean]");
                }
            }
            return luaL_error(L, "Invalid '%s' property called [connect|rate|repeat]", key.c_str());
        }

        return luaL_error(L, "Timer object nil.");
    }


    int lua_gc(lua_State* L) // object
    {
        // get object's metatable
        if (size_t *index = (size_t *)luaL_checkudata(L, 1, "yaget.dispatcher.timer"))
        {
            Timers->freeIndex(*index);
        }

        // return one value from the top of the stack
        return 1;
    }


    // will return timer object which then can be manipulated
    // and used
    int timer(lua_State* L)    // rate, repeat (or any combination of number, boolean)
    {
        float rate = 0;
        bool repeat = false;
        int numValues = lua_gettop(L);
        if (numValues)
        {
            if (lua_isboolean(L, -1))
            {
                repeat = lua_toboolean(L, -1) == 1;
            }
            else if (lua_isnumber(L, -1))
            {
                rate = lua_tonumber(L, -1);
            }
            else
            {
                return luaL_error(L, "Invalid second value passed to timer [rate|repeat]");
            }

            if (numValues > 1)
            {
                if (lua_isboolean(L, -2))
                {
                    repeat = lua_toboolean(L, -2) == 1;
                }
                else if (lua_isnumber(L, -2))
                {
                    rate = lua_tonumber(L, -2);
                }
                else
                {
                    return luaL_error(L, "Invalid first value passed to timer [rate|repeat]");
                }
            }
        }

        if (!Timers)
        {
            luabind::object g = luabind::globals(L);
            luabind::object lua_yaget = luabind::gettable(g, "yaget");
            Dispatcher *dsp = luabind::object_cast<Dispatcher *>(lua_yaget["dsp"]);

            Timers.reset(new LuaTimers(*dsp));
        }

        LuaTimers::Functor functor;
        functor.rate = rate;
        functor.repeat = repeat;
        size_t index = Timers->nextIndex();
        size_t *conUserData = (size_t *)lua_newuserdata(L, sizeof(size_t));// name, udata
        *conUserData = index;
        (*Timers)[index] = functor;

        luaL_getmetatable(L, "yaget.dispatcher.timer");                 // name, udata, mt
        lua_setmetatable(L, -2);                                        // name, udata

        // return the user data
        return 1;
    }


} // namespace



namespace eg { namespace script {

void init_timer(lua_State *L)
{
    using namespace luabind;

    // Create and fill the metatable
    luaL_newmetatable(L, "yaget.dispatcher.timer");     // mt

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

    lua_pushstring(L, "timer");                         // "timer"
    lua_pushcfunction(L, &timer);                       // "timer", timer
    lua_rawset(L, LUA_GLOBALSINDEX);                    //
}


}} // namespace script // namespace eg

