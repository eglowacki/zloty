#include "Script/ComponentBinder.h"


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
std::string yaget::script::print_lua_arg_values(const sol::variadic_args& va)
{
    Strings values;
    for (const auto& arg : va)
    {
        auto luaType = arg.get_type();
        switch (luaType)
        {
            case sol::type::number:
            {
                double value = arg;
                values.push_back(std::to_string(value));
                break;
            }
            case sol::type::string:
            {
                std::string value = arg;
                values.push_back(value);
                break;
            }
            default:
                YLOG_ERROR("LUA", "Unhandled Lua type: %s", std::string(magic_enum::enum_name(luaType)).c_str());
                break;
        }
    }

    return conv::Combine(values, ", ");
}
