#include "Script/AppUtilitiesBinder.h"
#include "App/Application.h"
#include "Script/ScriptState.h"


//----------------------------------------------------------------------------------------------------------------------------------
void yaget::script::BindAppUtilities(sol::state& luaState, const std::string& luaNamespace, Application& app)
{
    if (luaNamespace.empty())
    {
        YLOG_ERROR("LUA", "Invalid namespace: '%s' provided.", luaNamespace.c_str());
        return;
    }

    auto yaget_lua_env = luaState[luaNamespace].get_or_create<sol::table>();

    yaget_lua_env.set_function("NewItemId", [&app]()
    {
        auto id = yaget::idspace::get_persistent(app.IdCache);;
        return id;
    });
}
