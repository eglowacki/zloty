//////////////////////////////////////////////////////////////////////
// ComponentMetaTypes.h
//
//  Copyright 11/5/2026 Edgar Glowacki
//
//  Maintained by: Edgar
//
//  NOTES:
//      Helper templates and utilities to help with 
//      meta programming for components and coordinators
//
//
//  #include "Components/ComponentMetaTypes.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "YagetCore.h"

#include "App/AppHarness.h"

#include "Meta/CompilerAlgo.h"

namespace yaget::comp::metatypes
{
    //----------------------------------------------------------------------------------------------------------------------------------
    template<typename T>
    concept requires_global_coordinator = requires { typename T::Global; };


    //----------------------------------------------------------------------------------------------------------------------------------
    template <typename T>
    concept has_coordinator_set = requires { typename T::CoordinatorSet_t; };


    //----------------------------------------------------------------------------------------------------------------------------------
    template <typename T>
    concept has_pattern_function = requires { typename T::PatternFunction; };


    //----------------------------------------------------------------------------------------------------------------------------------
    template<typename T>
    concept is_coordinator_set = requires
    {
        typename T::Coordinators;
        T::NumCoordinators > 0;
    };

    //----------------------------------------------------------------------------------------------------------------------------------
    // Set of templates to extract CoordinateSet class type 
    // from either GameCoordinator or CoordinatorSet
    template<typename T>
    struct coordinator_set;


    //----------------------------------------------------------------------------------------------------------------------------------
    template<has_coordinator_set T>
    struct coordinator_set<T>
    {
        using type = typename T::CoordinatorSet_t;
    };


    //----------------------------------------------------------------------------------------------------------------------------------
    template<is_coordinator_set T>
    struct coordinator_set<T>
    {
        using type = T;
    };


    //----------------------------------------------------------------------------------------------------------------------------------
    template <typename T>
    using coordinator_set_t = coordinator_set<T>::type;


    //----------------------------------------------------------------------------------------------------------------------------------
    constexpr size_t InvalidCoordinatorIndex = std::numeric_limits<size_t>::max();


    //----------------------------------------------------------------------------------------------------------------------------------
    // return index of coordinator that has this component type in its FullRow
    template <typename ComponentType, typename T>
    consteval size_t FindCoordinatorIndex()
    {
        using CS = coordinator_set<T>::type;

        constexpr size_t NumCoordinators = CS::NumCoordinators;
        size_t indexResult = InvalidCoordinatorIndex;

        meta::for_loop<NumCoordinators>([&indexResult]<std::size_t T0>()
        {
            if (indexResult == InvalidCoordinatorIndex)
            {
                constexpr std::size_t coordinatorIndex = T0;

                using Coordinators = typename CS::Coordinators;
                using CoordinatorType = std::tuple_element_t<coordinatorIndex, Coordinators>;
                using CoordinatorComponents = CoordinatorType::FullRow;

                if constexpr (yaget::meta::tuple_is_element_v<ComponentType*, CoordinatorComponents>)
                {
                    indexResult = coordinatorIndex;
                }
            }
        });

        return indexResult;
    }


    //----------------------------------------------------------------------------------------------------------------------------------
    template <typename ComponentType, typename T>
    constexpr size_t coordinator_index_v = FindCoordinatorIndex<ComponentType, T>();
    

    //----------------------------------------------------------------------------------------------------------------------------------
    // Checks if component type is valid for this coordinator set
    template <typename ComponentType, typename T>
    consteval bool IsComponentValid()
    {
        constexpr size_t coordinatorIndex = coordinator_index_v<ComponentType, T>;
        return coordinatorIndex != InvalidCoordinatorIndex;
    }


    //----------------------------------------------------------------------------------------------------------------------------------
    template <typename ComponentType, typename T>
    constexpr bool component_valid_v = IsComponentValid<ComponentType, T>();


    //----------------------------------------------------------------------------------------------------------------------------------
    enum class ComponentQueryType
    {
        Invalid,
        Valid,
        Global
    };


    //----------------------------------------------------------------------------------------------------------------------------------
    // returns component type for this coordinator set, either Invalid, Valid or Global
    template <typename ComponentType, typename T>
    consteval ComponentQueryType ComponentPolicyType()
    {
        using CS = coordinator_set_t<T>;

        constexpr size_t coordinatorIndex = coordinator_index_v<ComponentType, T>;
        if (coordinatorIndex != InvalidCoordinatorIndex)
        {
            using Coordinators = typename CS::Coordinators;
            using CoordinatorType = std::tuple_element_t<coordinatorIndex, Coordinators>;
            using Policy = CoordinatorType::Policy;
            if constexpr (requires_global_coordinator<Policy>)
            {
                return ComponentQueryType::Global;
            }

            return ComponentQueryType::Valid;
        }

        return ComponentQueryType::Invalid;
    }


    //----------------------------------------------------------------------------------------------------------------------------------
    template <typename ComponentType, typename T>
    constexpr static bool component_global_v = ComponentPolicyType<ComponentType, T>() == ComponentQueryType::Global;


    //----------------------------------------------------------------------------------------------------------------------------------
    template <typename ComponentType, typename T>
    constexpr static bool component_local_v = ComponentPolicyType<ComponentType, T>() == ComponentQueryType::Valid;


    //----------------------------------------------------------------------------------------------------------------------------------
    // extract component type from coordinator set, if component is not valid for this coordinator set, then it will fail to compile
    template <typename ComponentType, typename T>
    struct CoordinatorType
    {
        using CS = coordinator_set_t<T>;
        using Coordinators = typename CS::Coordinators;
        using type = std::tuple_element_t<coordinator_index_v<ComponentType, T>, Coordinators>;
    };


    //----------------------------------------------------------------------------------------------------------------------------------
    template<typename ComponentType, typename T>
    using coordinator_type_t = typename CoordinatorType<ComponentType, T>::type;


    //----------------------------------------------------------------------------------------------------------------------------------
    // extract component row tuple from component based if its global or local component
    template<typename ComponentType, typename T, typename C>
    struct global_component_tuple;


    //----------------------------------------------------------------------------------------------------------------------------------
    template<typename ComponentType, typename T, typename C>
    requires component_global_v<ComponentType, T>
    struct global_component_tuple<ComponentType, T, C>
    {
        using type = std::tuple<C>;
    };


    //----------------------------------------------------------------------------------------------------------------------------------
    template<typename ComponentType, typename T, typename C>
    requires component_local_v<ComponentType, T>
    struct global_component_tuple<ComponentType, T, C>
    {
        using type = std::tuple<>;
    };


    //----------------------------------------------------------------------------------------------------------------------------------
    template<typename ComponentType, typename T, typename C>
    using global_component_tuple_t = typename global_component_tuple<ComponentType, T, C>::type;


    //----------------------------------------------------------------------------------------------------------------------------------
    template<typename ComponentType, typename T, typename C>
    struct local_component_tuple;


    //----------------------------------------------------------------------------------------------------------------------------------
    template<typename ComponentType, typename T, typename C>
    requires component_local_v<ComponentType, T>
    struct local_component_tuple<ComponentType, T, C>
    {
        using type = std::tuple<C>;
    };


    //----------------------------------------------------------------------------------------------------------------------------------
    template<typename ComponentType, typename T, typename C>
    requires component_global_v<ComponentType, T>
    struct local_component_tuple<ComponentType, T, C>
    {
        using type = std::tuple<>;
    };


    //----------------------------------------------------------------------------------------------------------------------------------
    template<typename ComponentType, typename T, typename C>
    using local_component_tuple_t = typename local_component_tuple<ComponentType, T, C>::type;


    //----------------------------------------------------------------------------------------------------------------------------------
    // select either global or local component tuple based on Select template parameter
    template<typename ComponentType, typename T, typename C, bool Select>
    struct SelectionTupleType;


    //----------------------------------------------------------------------------------------------------------------------------------
    template<typename ComponentType, typename T, typename C, bool Select>
    requires (Select == true)
    struct SelectionTupleType<ComponentType, T, C, Select>
    {
        using type = global_component_tuple_t<ComponentType, T, C>;
    };


    //----------------------------------------------------------------------------------------------------------------------------------
    template<typename ComponentType, typename T, typename C, bool Select>
    requires (Select == false)
    struct SelectionTupleType<ComponentType, T, C, Select>
    {
        using type = local_component_tuple_t<ComponentType, T, C>;
    };


    //----------------------------------------------------------------------------------------------------------------------------------
    template<typename ComponentType, typename T, typename C, bool Select>
    using selection_tuple_type_t = typename SelectionTupleType<ComponentType, T, C, Select>::type;


    //----------------------------------------------------------------------------------------------------------------------------------
    // Return tuple of either global or local components based on Select template parameter
    // Call to this function should start with TupleIndex = 0
    template <std::size_t TupleIndex, typename T, bool Select>
    constexpr auto GetTypedComponentsRow()
    {
        using CS = coordinator_set_t<T>;
        using Components = typename CS::FullRow;
        using ComponentType = std::tuple_element_t<TupleIndex, Components>;
        using CurrentComponentType = meta::strip_qualifiers_t<std::tuple_element_t<TupleIndex, Components>>;

        constexpr size_t NumComponents = std::tuple_size_v<Components>;

        using CurrentRow = selection_tuple_type_t<CurrentComponentType, T, ComponentType, Select>;
        CurrentRow currentRow{};

        if constexpr (TupleIndex + 1 < NumComponents)
        {
            constexpr auto nextRow = GetTypedComponentsRow<TupleIndex + 1, T, Select>();
            return std::tuple_cat(currentRow, nextRow);
        }
        else
        {
            return currentRow;
        }
    }


    //----------------------------------------------------------------------------------------------------------------------------------
    template <typename T>
    struct global_components_row
    {
        using type = decltype(GetTypedComponentsRow<0, T, true>());
    };


    //----------------------------------------------------------------------------------------------------------------------------------
    template <typename T>
    using global_components_row_t = typename global_components_row<T>::type;


    //----------------------------------------------------------------------------------------------------------------------------------
    template <typename T>
    struct local_components_row
    {
        using type = decltype(GetTypedComponentsRow<0, T, false>());
    };


    //----------------------------------------------------------------------------------------------------------------------------------
    template <typename T>
    using local_components_row_t = typename local_components_row<T>::type;

}
