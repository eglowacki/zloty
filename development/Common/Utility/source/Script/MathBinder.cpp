#include "Script/MathBinder.h"
#include "Script/ScriptState.h"
#include <MathFacade.h>


//----------------------------------------------------------------------------------------------------------------------------------
void yaget::script::BindMath(sol::state& luaState, const std::string& luaNamespace)
{
    if (luaNamespace.empty())
    {
        YLOG_ERROR("LUA", "Invalid namespace: '%s' provided.", luaNamespace.c_str());
        return;
    }

    auto yaget_lua_env = luaState[luaNamespace].get_or_create<sol::table>();

    sol::usertype<math3d::Vector2> vector2Type = yaget_lua_env.new_usertype<math3d::Vector2>("Vector2",
        // 3 constructors
        sol::constructors<math3d::Vector2(),
        math3d::Vector2(float),
        math3d::Vector2(float, float)>(),
        "Zero", sol::var(std::cref(math3d::Vector2::Zero)),
        "One", sol::var(std::cref(math3d::Vector2::One)));

    sol::usertype<math3d::Vector3> vector3Type = yaget_lua_env.new_usertype<math3d::Vector3>("Vector3",
        // 3 constructors
        sol::constructors<math3d::Vector3(),
        math3d::Vector3(float),
        math3d::Vector3(float, float, float)>(),
        "Zero", sol::var(std::cref(math3d::Vector3::Zero)),
        "One", sol::var(std::cref(math3d::Vector3::One)));

    sol::usertype<math3d::Vector4> vector4Type = yaget_lua_env.new_usertype<math3d::Vector4>("Vector4",
        // 3 constructors
        sol::constructors<math3d::Vector4(),
        math3d::Vector4(float),
        math3d::Vector4(float, float, float, float)>(),
        "Zero", sol::var(std::cref(math3d::Vector4::Zero)),
        "One", sol::var(std::cref(math3d::Vector4::One)));

    sol::usertype<math3d::Quaternion> quaternionType = yaget_lua_env.new_usertype<math3d::Quaternion>("Quaternion",
        // 2 constructors
        sol::constructors<math3d::Quaternion(),
        math3d::Quaternion(float, float, float, float)>(),
        "Identity", sol::var(std::cref(math3d::Quaternion::Identity)));
}
