#include "Script/ComponentBinder.h"
#include <MathFacade.h>


//----------------------------------------------------------------------------------------------------------------------------------
std::string yaget::script::print_lua_arg_types(const sol::variadic_args& va)
{
    yaget::Strings results;
    for (const auto& arg : va)
    {
        auto luaType = arg.get_type();
        const auto typeName = magic_enum::enum_name(luaType);

        results.push_back(std::string(typeName));
    }

    return yaget::conv::Combine(results, ", ");
}


//----------------------------------------------------------------------------------------------------------------------------------
std::string yaget::script::print_lua_arg_values(const sol::stack_proxy& arg)
{
    std::string result;
    auto luaType = arg.get_type();

    switch (luaType)
    {
        case sol::type::number:
        {
            double value = arg;
            result = std::to_string(value);
            break;
        }
        case sol::type::lua_nil:
        {
            result = "null";
            break;
        }
        case sol::type::string:
        {
            result = arg;
            break;
        }
        case sol::type::table:
        {
            if (arg.is<Strings>())
            {
                Strings value = arg.get<Strings>();
                result = "[ " + conv::Combine(value, ", ") + " ]";
            }
            else
            {
                YLOG_ERROR("LUA", "Unhandled Lua table type variant.");
            }
            break;
        }
        case sol::type::userdata:
        {
            if (arg.is<math3d::Vector2>())
            {
                auto value = arg.get<math3d::Vector2>();
                result = conv::ToString(value);
            }
            else if (arg.is<math3d::Vector3>())
            {
                auto value = arg.get<math3d::Vector3>();
                result = conv::ToString(value);
            }
            else if (arg.is<math3d::Vector4>())
            {
                auto value = arg.get<math3d::Vector4>();value;
                result = conv::ToString(value);
            }
            else if (arg.is<math3d::Quaternion>())
            {
                auto value = arg.get<math3d::Quaternion>();
                result = conv::ToString(value);
            }
            else
            {
                YLOG_ERROR("LUA", "Unhandled Lua userdata variant.");
            }
            break;
        }

        default:
            YLOG_ERROR("LUA", "Unhandled Lua type: '%s'.", std::string(magic_enum::enum_name(luaType)).c_str());
            break;
    }

    return result;
}


//----------------------------------------------------------------------------------------------------------------------------------
std::string yaget::script::print_lua_arg_values(const sol::variadic_args& va)
{
    Strings values;
    for (const auto& arg : va)
    {
        values.push_back(print_lua_arg_values(arg));
    }

    return conv::Combine(values, ", ");
}
