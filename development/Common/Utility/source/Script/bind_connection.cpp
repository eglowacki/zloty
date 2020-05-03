#include "Message/Dispatcher.h"
#include "MessageInterface.h"
#include "Script/ScriptErrorHandler.h"

using namespace eg;

namespace
{
    const int kNumTries = 5;

    //! Functor used to connect to any message in dispatcher
    class LuaMsgFunctor
    {
    // no copy semantics
    LuaMsgFunctor(const LuaMsgFunctor&);
    LuaMsgFunctor& operator=(const LuaMsgFunctor&);

    public:
        LuaMsgFunctor(Dispatcher& dsp, guid_t id)
        : mErrorCounter(kNumTries)
        {
            mConnection = dsp[id].connect(boost::ref(*this));
            mConnection.block();
        }

        ~LuaMsgFunctor() {mConnection.disconnect();}

        void attach(const luabind::object& callback)
        {
            mCallback = callback;
            if (mCallback)
            {
                mConnection.unblock();
            }
            else
            {
                mConnection.block();
            }
        }

        void operator()(const Message& msg)
        {
            if (msg.Is<float>())
            {
                triggerCallback<float>(msg);
            }
            else
            {
                log_error << "Message Type: " << logs::hex<guid_t>(msg.mType) << " not handled.";
            }
        }

    private:
        template <typename T>
        inline void triggerCallback(const Message& msg)
        {
            if (!mCallback)
            {
                log_error << "There is no lua callback attached but it received message: " << logs::hex<guid_t>(msg.mType);
                return;
            }

            if (!mErrorCounter)
            {
                return;
            }

            T param = msg.GetValue<T>();
            try
            {
                luabind::call_function<void>(mCallback, param);
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

        luabind::object mCallback;
        // we only want to log error on first exception
        // since last good run
        int mErrorCounter;
        boost::signals::connection mConnection;
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
        if (LuaMsgFunctor *functor = (LuaMsgFunctor *)luaL_checkudata(L, 1, "yaget.dispatcher.connection"))
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

        return luaL_error(L, "Connection object nil.");
    }


    // collect garbage
    int lua_gc(lua_State* L) // object
    {
        // get object's metatable
        if (LuaMsgFunctor *con = (LuaMsgFunctor *)luaL_checkudata(L, 1, "yaget.dispatcher.connection"))
        {
            con->~LuaMsgFunctor();
        }

        // return one value from the top of the stack
        return 1;
    }

    // will return connection object which then can be manipulated
    // and used
    int connection(lua_State* L)                                        // type
    {
        //lua_stackdump(L);
        int type = luaL_checkint(L, 1);
        if (type == mtype::kFrameTick)
        {
            return luaL_error(L, "Connection object does not support FrameTick. Use timer.");
        }

        luabind::object g = luabind::globals(L);
        luabind::object lua_yaget = luabind::gettable(g, "yaget");
        Dispatcher *dsp = luabind::object_cast<Dispatcher *>(lua_yaget["dsp"]);

        void *conUserData = lua_newuserdata(L, sizeof(LuaMsgFunctor));  // type, udata
        /*LuaMsgFunctor *con =*/ new(conUserData)LuaMsgFunctor(*dsp, type);

        luaL_getmetatable(L, "yaget.dispatcher.connection");            // type, udata, mt
        lua_setmetatable(L, -2);                                        // type, udata

        // return the user data
        return 1;
    }

} // namespace



namespace eg { namespace script {

void init_connection(lua_State *L)
{
    using namespace luabind;

    // Create and fill the metatable
    luaL_newmetatable(L, "yaget.dispatcher.connection");// mt

    lua_pushstring(L, "__index");                       // mt, "__index"
    lua_pushcfunction(L, &lua_index);                   // mt, "__index", lua_index
    lua_rawset(L, -3);                                  // mt

    lua_pushstring(L, "__newindex");                    // mt, "__newindex"
    lua_pushcfunction(L, &lua_newindex);                // mt, "__newindex", lua_index
    lua_rawset(L, -3);                                  // mt

    lua_pushstring(L, "__gc");                          // mt, "__gc"
    lua_pushcfunction(L, &lua_gc);                      // mt, "__gc", lua_gc
    lua_rawset(L, -3);                                  // mt

    lua_pop(L, 1);                                      //

    lua_pushstring(L, "connection");                    // "connection"
    lua_pushcfunction(L, &connection);                  // "connection", connection
    lua_rawset(L, LUA_GLOBALSINDEX);                    //
}


}} // namespace script // namespace eg

