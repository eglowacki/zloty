#include "Script/Executant.h"
#include "Script/luacpp.h"


//lua_State* L = luaL_newstate(); // Create new Lua state
//luaL_openlibs(L);               // Load Lua libraries

//lua_pushcfunction(L, l_log);
//lua_setglobal(L, "log");

//// Execute Lua script
//if (luaL_dostring(L, "log('Hello from Lua!')")) 
//{
//    printf("Error: %s\n", lua_tostring(L, -1));
//}

//lua_close(L); // Close Lua state

namespace
{
    //--------------------------------------------------------------------------------------------------
    static int l_log(lua_State* L) 
    {
        const char* msg = luaL_checkstring(L, 1);
        YLOG_INFO("DEF", "%s\n", msg);
        return 0; // Number of results
    }


    //--------------------------------------------------------------------------------------------------
    class LuaExecutant : public yaget::NoCopy
    {
    public:
        LuaExecutant()
            : L{ luaL_newstate() }
        {
            luaL_openlibs(L);
            lua_pushcfunction(L, l_log);
            lua_setglobal(L, "log");
        }

        ~LuaExecutant()
        {
            lua_close(L);
        }

        std::string Run(const std::string& sourceText)
        {
            if (luaL_dostring(L, sourceText.c_str()) == LUA_OK)
            {
                lua_pop(L, lua_gettop(L));
            }
            else
            {
                YLOG_ERROR("SCRT", "%s", lua_tostring(L, -1));
            }

            return {};
        }

    private:
        lua_State* L{};
    };


    //--------------------------------------------------------------------------------------------------
    struct LuaEnvironment;
    std::unique_ptr<LuaEnvironment> lifeTimeEnvironment;

    //--------------------------------------------------------------------------------------------------
    void CleanupLuaEnvironment() 
    {
        lifeTimeEnvironment = nullptr;
    }


    //--------------------------------------------------------------------------------------------------
    struct LuaEnvironment
    {
        LuaEnvironment()
        {
            std::ignore = std::atexit(CleanupLuaEnvironment);
        }

        LuaExecutant mLuaExecutant;
    };


    //--------------------------------------------------------------------------------------------------
    LuaEnvironment* GetLuaEnv()
    {
        if (!lifeTimeEnvironment)
        {
            lifeTimeEnvironment = std::make_unique<LuaEnvironment>();
        }

        return lifeTimeEnvironment.get();
    }


    //--------------------------------------------------------------------------------------------------
    LuaExecutant& lua_exe()
    {
        static auto luaEnvironment = GetLuaEnv();
        return luaEnvironment->mLuaExecutant;
    }

}


//--------------------------------------------------------------------------------------------------
std::string yaget::script::Run(const std::string& sourceText)
{
    auto result = lua_exe().Run(sourceText);

    return result;
}
