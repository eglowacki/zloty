//////////////////////////////////////////////////////////////////////
// CompilerAlgo.h
//
//  Copyright 6/30/2019 Edgar Glowacki
//
//  Maintained by: Edgar
//
//  NOTES:
//      
//
//
//  #include "Meta/CompilerAlgo.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "YagetCore.h"


namespace yaget
{
    namespace meta
    {
        // compile time iteration over tuple and calling callback for each entry with Type
        // The Type is passed as Type pointer set to nullptr.
        template <
            typename TTuple,
            size_t Index = 0,
            size_t Size = std::tuple_size_v<std::remove_reference_t<TTuple>>,
            typename TCallable
        >
        void for_each_type(TCallable&& callable)
        {
            if constexpr (Index < Size)
            {
                using RequestedType = typename std::tuple_element<Index, TTuple>::type;
                RequestedType* rt = nullptr;
                std::invoke(callable, rt);

                if constexpr (Index + 1 < Size)
                {
                    for_each_type<TTuple, Index + 1>(std::forward<TCallable>(callable));
                }
            }
        }

        // similar as above, but iterates over each element
        template <
            size_t Index = 0,
            typename TTuple,
            size_t Size = std::tuple_size_v<std::remove_reference_t<TTuple>>,
            typename TCallable
        >
        void for_each(TTuple&& tuple, TCallable&& callable)
        {
            if constexpr (Index < Size)
            {
                std::invoke(callable, std::get<Index>(tuple));

                if constexpr (Index + 1 < Size)
                {
                    for_each<Index + 1>(std::forward<TTuple>(tuple), std::forward<TCallable>(callable));
                }
            }
        }

        // similar as above, but iterates over two elements at the time. tuple size must be even.
        template <
            size_t Index = 0,
            typename TTuple,
            size_t Size = std::tuple_size_v<std::remove_reference_t<TTuple>>,
            typename TCallable
        >
            void for_each_pair(TTuple&& tuple, TCallable&& callable)
        {
            if constexpr (Index + 1 < Size)
            {
                std::invoke(callable, std::get<Index>(tuple), std::get<Index + 1>(tuple));

                if constexpr (Index + 2 < Size)
                {
                    for_each_pair<Index + 2>(std::forward<TTuple>(tuple), std::forward<TCallable>(callable));
                }
            }
        }

        // Copies one tuple into another. Both must match size
        // From tuple elements are used like a function call
        template<int N, typename To, typename From, size_t Size = std::tuple_size_v<std::remove_reference_t<To>>>
        void tuple_copy(To& to, const From& from)
        {
            std::get<N>(to) = std::get<N>(from)();
            if constexpr (N + 1 < Size)
            {
                meta::tuple_copy<N + 1, To, From>(to, from);
            }
        }


    } // namespace meta
} // namespace yaget
