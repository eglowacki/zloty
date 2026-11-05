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

#include "Components/ComponentTypes.h"
#include "Components/GameCoordinatorGenerator.h"
#include "Components/SystemsCoordinator.h"
#include "Items/StageComponent.h"
#include "Script/ScriptState.h"
#include "VTS/VirtualTransportSystem.h"


namespace yaget::script
{
    //----------------------------------------------------------------------------------------------------------------------------------
    template <typename T>
    struct TypeToLuaParam;


    //----------------------------------------------------------------------------------------------------------------------------------
    template <typename T>
    concept LuaParamNumber = (
        std::integral<T>
        or std::floating_point<T>
        or std::is_enum_v<T>
        )
        and (not std::same_as<T, bool>);


    //----------------------------------------------------------------------------------------------------------------------------------
    template <typename T>
    concept LuaParamMath3d = (
        std::same_as<T, math3d::Vector2>
        or std::same_as<T, math3d::Vector3>
        or std::same_as<T, math3d::Vector4>
        or std::same_as<T, math3d::Quaternion>
        );


    //----------------------------------------------------------------------------------------------------------------------------------
    template <typename T> requires LuaParamNumber<T>
    struct TypeToLuaParam<T>
    {
        static constexpr sol::type value = sol::type::number;
    };
    

    //----------------------------------------------------------------------------------------------------------------------------------
    template <>
    struct TypeToLuaParam<std::string>
    {
        static constexpr sol::type value = sol::type::string;
    };


    //----------------------------------------------------------------------------------------------------------------------------------
    template <>
    struct TypeToLuaParam<Strings>
    {
        static constexpr sol::type value = sol::type::table;
    };


    //----------------------------------------------------------------------------------------------------------------------------------
    template <>
    struct TypeToLuaParam<yaget::io::VirtualTransportSystem::Section>
    {
        static constexpr sol::type value = sol::type::string;
    };


    //----------------------------------------------------------------------------------------------------------------------------------
    template <typename T> requires LuaParamMath3d<T>
    struct TypeToLuaParam<T>
    {
        static constexpr sol::type value = sol::type::userdata;
    };


    //----------------------------------------------------------------------------------------------------------------------------------
    template <typename T>
    void AssertLuaParameter(const sol::stack_proxy& luaParam, const std::source_location& location = std::source_location::current())
    {
        location;
        YAGET_ASSERT(luaParam.get_type() == TypeToLuaParam<T>::value, "Expected 'lua %s' for '%s', provided: '%s'.", 
            std::string(magic_enum::enum_name(TypeToLuaParam<T>::value)).c_str(),
            comp::db::ResolveName<T>().c_str(),
            std::string(magic_enum::enum_name(luaParam.get_type())).c_str());
        
    }


    //----------------------------------------------------------------------------------------------------------------------------------
    template <typename T>
    struct ConvertLuaArgToParamValue;


    //----------------------------------------------------------------------------------------------------------------------------------
    template <>
    struct ConvertLuaArgToParamValue<io::VirtualTransportSystem::Section>
    {
        static io::VirtualTransportSystem::Section From(const sol::stack_proxy& source)
        {
            AssertLuaParameter<io::VirtualTransportSystem::Section>(source);

            io::VirtualTransportSystem::Section value{ source.as<std::string>() };
            return value;
        }
    };


    //----------------------------------------------------------------------------------------------------------------------------------
    template  <typename T> requires LuaParamNumber<T>
    struct ConvertLuaArgToParamValue<T>
    {
        static T From(const sol::stack_proxy& source)
        {
            AssertLuaParameter<T>(source);

            T value{ source.as<T>() };
            return value;
        }
    };


    //----------------------------------------------------------------------------------------------------------------------------------
    template  <typename T> requires LuaParamMath3d<T>
    struct ConvertLuaArgToParamValue<T>
    {
        static T From(const sol::stack_proxy& source)
        {
            AssertLuaParameter<T>(source);

            T value{ source.as<T>() };
            return value;
        }
    };


    //----------------------------------------------------------------------------------------------------------------------------------
    template <>
    struct ConvertLuaArgToParamValue<std::string>
    {
        static std::string From(const sol::stack_proxy& source)
        {
            AssertLuaParameter<std::string>(source);

            std::string value{ source.as<std::string>() };
            return value;
        }
    };


    //----------------------------------------------------------------------------------------------------------------------------------
    template <>
    struct ConvertLuaArgToParamValue<Strings>
    {
        static Strings From(const sol::stack_proxy& source)
        {
            AssertLuaParameter<Strings>(source);

            Strings value{ source.as<Strings>() };
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
    std::string print_lua_arg_values(const sol::stack_proxy& arg);
    std::string print_lua_arg_values(const sol::variadic_args& va);


    //----------------------------------------------------------------------------------------------------------------------------------
    template <typename ComponentType, typename SystemsCoordinator>
	struct PersistentComponent
	{
        using ParameterPack = comp::db::RowDescription_t<ComponentType>::Types;
        using ComponentParameters = meta::tuple_cat_t<std::tuple<comp::Id_t>, ParameterPack>;
        static constexpr size_t NumParameters = std::tuple_size_v<ComponentParameters>;

        PersistentComponent(SystemsCoordinator& systemsCoordinator)
            : mSystemsCoordinator(systemsCoordinator)
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

                    const auto& componentName = comp::db::ResolveName<ComponentType>();
                    if (mSystemsCoordinator.IsComponentTyped(componentName))
                    {
                        ComponentType* component = nullptr;
                        using Coordinators = typename SystemsCoordinator::CoordinatorSet_t::Coordinators;
                        Coordinators* coordinators = nullptr;
                        component; coordinators;

                        constexpr size_t coorinatorIndex = comp::FindCoordinatorIndex<ComponentType, SystemsCoordinator>();
                        coorinatorIndex;

                        if constexpr (comp::IsComponentValid<ComponentType, SystemsCoordinator>())
                        {
                            if constexpr (comp::IsComponentGlobal<ComponentType, SystemsCoordinator>() == comp::ComponentQueryType::Global)
                            {
                                std::get<0>(fullParams) = comp::GLOBAL_ID_MARKER;
                                int z = 0;
                                z;
                            }
                        }



                        // ====
                        //auto coordinator = comp::GetCoordinator<ComponentType, SystemsCoordinator>(mSystemsCoordinator);
                        //coordinator;

                        //const auto& coordinatorCREF = comp::GetCoordinator<ComponentType, SystemsCoordinator>(std::cref(mSystemsCoordinator));
                        //coordinatorCREF;

                        auto& tt = mSystemsCoordinator.GetCoordinatorSet();
                        tt;
                        using CS = meta::strip_qualifiers_t<decltype(tt)>;
                        CS* cs = nullptr;
                        cs;

                        //std::integral<CS>
                        //auto coordinator2 = comp::GetCoordinator<ComponentType, CS>(tt);
                        //coordinator2;

                        //const auto& coordinator2CREF = comp::GetCoordinator<ComponentType, decltype(tt)>(std::cref(tt));
                        //coordinator2CREF;

                        //constexpr bool systemCoordinator = comp::is_systems_coordinator<SystemsCoordinator>;
                        //constexpr bool coordinatorSet = comp::is_coordinator_set<SystemsCoordinator>;

                        //using CoordSet = typename SystemsCoordinator::CoordinatorSet_t;
                        ////CoordSet* coordSet = nullptr; coordSet;
                        //constexpr bool systemCoordinator2 = comp::is_systems_coordinator<CoordSet>;
                        //constexpr bool coordinatorSet2 = comp::is_coordinator_set<CoordSet>;
                        //////constexpr bool coordinatorSet3 = comp::is_coordinator_set2<CoordSet>;

                        //if constexpr (CoordSet::NumCoordinators == 0)
                        //{
                        //    int z = 0;
                        //    z;
                        //}
                        
                        
                        
                        // ====




                        auto& director = mSystemsCoordinator.Director();
                        validCall = director.SaveComponentState(componentName, fullParams);

                        yaget::comp::ItemId itemId(std::get<0>(fullParams));
                        YLOG_INFO("LUA", "Saved Component %s: '%s(%s). LuaParameters: '%s'.", 
                            itemId.ToString().c_str(),
                            componentName.c_str(), 
                            print_tuple(fullParams).c_str(), 
                            print_lua_arg_values(va).c_str());
                    }
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

            const auto& expectedParamNames = print_tuple_types<ComponentParameters>();
            const auto& providedParamNames = print_lua_arg_types(va);
            const auto& providedParamValues = print_lua_arg_values(va);

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

        SystemsCoordinator& mSystemsCoordinator;
	};


    //----------------------------------------------------------------------------------------------------------------------------------
    template <typename ComponentTypes, typename SC>
    void BindComponents(sol::state& luaState, const std::string& luaNamespace, SC& systemCoordinator)
    {
        if (luaNamespace.empty())
        {
            YLOG_ERROR("LUA", "Invalid namespace: '%s' provided.", luaNamespace.c_str());
            return;
        }

        auto yaget_lua_env = luaState[luaNamespace].get_or_create<sol::table>();

        using DecayedComponentTypes = meta::decay_args_tuple<ComponentTypes>::type;

        if constexpr (yaget::meta::tuple_is_element_v<yaget::items::StageComponent, DecayedComponentTypes>)
        {
            yaget_lua_env.new_enum<items::db_stage::BlendOp>( "StageBlendOp", { 
                {"Replace", items::db_stage::BlendOp::Replace},
                {"MergeSame", items::db_stage::BlendOp::MergeSame},
                {"MergeAll", items::db_stage::BlendOp::MergeAll}
            });
        }

        meta::for_loop<DecayedComponentTypes>([&yaget_lua_env, &systemCoordinator]<std::size_t T0>()
        {
            constexpr std::size_t index = T0;
            using ComponentType = std::tuple_element_t<index, DecayedComponentTypes>;
            const auto& componentName = comp::db::ResolveName<ComponentType>();
            yaget_lua_env.set_function(componentName, script::PersistentComponent<ComponentType, SC>{systemCoordinator});
        });

        yaget_lua_env.set_function("AddStages", [&systemCoordinator](const Strings stageNames)
        {
            auto& director = systemCoordinator.Director();
            for (const auto& stageName : stageNames)
            {
                director.AddStage(stageName);
            }
        });

        yaget_lua_env.set_function("AddStageItems", [&systemCoordinator](const std::string stageName, const comp::ItemIds itemIds)
        {
            auto& director = systemCoordinator.Director();
            director.AddStageItems(stageName, itemIds);
        });
    }

}
