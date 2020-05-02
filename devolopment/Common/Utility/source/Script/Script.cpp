#include "Script.h"
#include "Script/ScriptErrorHandler.h"

namespace 
{
    using namespace eg;

    int yaget_Loader(lua_State *L)
    {
        //lua_pushstring(L, "AssetLoadSystem");
        //lua_gettable(L, LUA_REGISTRYINDEX);

        //script::stackDump(L);
        std::string module = lua_tostring(L, 1);
        //script::stackDump(L);
        return 0;
    }

    struct ModuleLoader
    {
        int operator()(lua_State *L)
        {
            std::string module = lua_tostring(L, 1);
            //script::stackDump(L);
            return 0;
        }
    };

} // namespace


namespace eg { namespace script {


void AddLoader(lua_State *L, lua_CFunction loaderCallback)
{
    lua_getfield(L, LUA_GLOBALSINDEX, "package");     // push "package"
    lua_getfield(L, -1, "loaders");                   // push "package.loaders"
    lua_remove(L, -2);                                // remove "package"

    // Count the number of entries in package.loaders.
    // Table is now at index -2, since 'nil' is right on top of it.
    // lua_next pushes a key and a value onto the stack.
    int numLoaders = 0;
    lua_pushnil(L);
    while (lua_next(L, -2) != 0)
    {
            lua_pop(L, 1);
            numLoaders++;
    }

    lua_pushinteger(L, numLoaders + 1);
    lua_pushcfunction(L, loaderCallback);
    lua_rawset(L, -3);

    // Table is still on the stack.  Get rid of it now.
    lua_pop(L, 1);
}


}} // namespace script  // namespace eg

