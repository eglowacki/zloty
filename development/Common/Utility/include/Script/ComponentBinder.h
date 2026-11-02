///////////////////////////////////////////////////////////////////////
// ComponentBinder.h
//
//  Copyright 11/1/2026 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      Provides binding of component creation and saving to DB (Director)
//      
//
//  #include "Script/ComponentBinder.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "YagetCore.h"
#include "Components/ComponentTypes.h"
#include "Components/GameCoordinatorGenerator.h"

#include "Script/ScriptState.h"
#include "VTS/VirtualTransportSystem.h"


namespace yaget::script
{
    //----------------------------------------------------------------------------------------------------------------------------------
    template <typename T, sol::type ExpectedType>
    void AssertLuaParameter(const sol::stack_proxy& luaParam, const std::source_location& location = std::source_location::current())
    {
        location;
        YAGET_ASSERT(luaParam.get_type() == ExpectedType, "Expected 'lua %s' for '%s', provided: '%s'.", 
            std::string(magic_enum::enum_name(ExpectedType)).c_str(),
            comp::db::ResolveName<T>().c_str(),
            std::string(magic_enum::enum_name(luaParam.get_type())).c_str());
        
    }


    //----------------------------------------------------------------------------------------------------------------------------------
    template <typename T>
    struct TypeToLuaParam;


    //----------------------------------------------------------------------------------------------------------------------------------
    template <>
    struct TypeToLuaParam<yaget::comp::Id_t>
    {
        static constexpr sol::type value = sol::type::number;
    };


    //----------------------------------------------------------------------------------------------------------------------------------
    template <>
    struct TypeToLuaParam<yaget::io::VirtualTransportSystem::Section>
    {
        static constexpr sol::type value = sol::type::string;
    };


    //----------------------------------------------------------------------------------------------------------------------------------
    template <typename T>
    struct ConvertLuaArgToParamValue;


    //----------------------------------------------------------------------------------------------------------------------------------
    template <>
    struct ConvertLuaArgToParamValue<io::VirtualTransportSystem::Section>
    {
        static io::VirtualTransportSystem::Section From(const sol::stack_proxy& source)
        {
            AssertLuaParameter<io::VirtualTransportSystem::Section, sol::type::string>(source);

            io::VirtualTransportSystem::Section value{ source.as<std::string>() };
            return value;
        }
    };


    //----------------------------------------------------------------------------------------------------------------------------------
    template <>
    struct ConvertLuaArgToParamValue<yaget::comp::Id_t>
    {
        static comp::Id_t From(const sol::stack_proxy& source)
        {
            AssertLuaParameter<comp::Id_t, sol::type::number>(source);

            comp::Id_t value{ source.as<yaget::comp::Id_t>() };
            return value;
        }
    };


    //----------------------------------------------------------------------------------------------------------------------------------
    template <typename T>
    void CopyToValue(const sol::stack_proxy& sourceParam, T& targetParam)
    {
        targetParam = ConvertLuaArgToParamValue<T>::From(sourceParam);
    }


    //----------------------------------------------------------------------------------------------------------------------------------
    template<typename T>
    std::string print_tuple_types()
    {
        auto names = comp::db::GetPolicyRowNames<T>();;
        return conv::Combine(names, ", ");
    }


    //----------------------------------------------------------------------------------------------------------------------------------
    std::string print_lua_arg_types(const sol::variadic_args& va);
    std::string print_lua_arg_values(const sol::variadic_args& va);


    //----------------------------------------------------------------------------------------------------------------------------------
    template <typename ComponentType, typename SystemCoordinator>
	struct PersistentComponent
	{
        using ParameterPack = yaget::comp::db::RowDescription_t<ComponentType>::Types;
        using ComponentParameters = yaget::meta::tuple_cat_t<std::tuple<yaget::comp::Id_t>, ParameterPack>;
        static constexpr size_t NumParameters = std::tuple_size_v<ComponentParameters>;

        PersistentComponent(SystemCoordinator& coordinator)
            : mCoordinator(coordinator)
        {
        }

        bool operator()(sol::variadic_args va)
        {
            using namespace yaget;

            bool validCall = false;

            if (va.size() == NumParameters)
            {
                const auto numValidParams = ValidateParameters(va);
                if (numValidParams == NumParameters)
                {
                    ComponentParameters fullParams;

                    meta::for_loop<ComponentParameters>([&fullParams, &va]<std::size_t T0>()
                    {
                        constexpr std::size_t index = T0;

                        const auto& sourceParam = va[index];
                        auto& targetParam = std::get<index>(fullParams);

                        script::CopyToValue(sourceParam, targetParam);
                    });

                    //template <typename S>
                    //bool SaveComponentState(comp::Id_t componentId, const std::string& componentName, const S& storage);

                    //ComponentType* component = std::apply([this](auto&&... params)
                    //{
                    //    return mCoordinator.template AddComponent<ComponentType>(params...);

                    //}, fullParams);

                    //component;
                    validCall = true;
                }
            }

            if (!validCall)
            {
                LogError(va);
            }

            return validCall;
        }

    private:
        void LogError(const sol::variadic_args& va) const
        {
            using namespace yaget;

            const auto& expectedParamNames = script::print_tuple_types<ComponentParameters>();
            const auto& providedParamNames = script::print_lua_arg_types(va);
            const auto& providedParamValues = script::print_lua_arg_values(va);

            YLOG_ERROR("LUA", "Invalid arguments for %s.\n\tExpected %d parameters: '%s'.\n\tProvided %d parameters: '%s' (%s)",
                comp::db::ResolveName<ComponentType>().c_str(),
                NumParameters,
                expectedParamNames.c_str(),
                va.size(),
                providedParamNames.c_str(),
                providedParamValues.c_str());
        }


        size_t ValidateParameters(const sol::variadic_args& va) const
        {
            using namespace yaget;

            size_t numValidParams = 0;
            meta::for_loop<ComponentParameters>([&va, &numValidParams]<std::size_t T0>()
            {
                constexpr std::size_t index = T0;
                using ElementType = std::tuple_element_t<index, ComponentParameters>;
                const auto paramType = script::TypeToLuaParam<ElementType>::value;
                const auto luaParam = va.get_type(index);
                numValidParams += (luaParam == paramType) ? 1 : 0;
            });

            return numValidParams;
        }

        SystemCoordinator& mCoordinator;
	};


}
