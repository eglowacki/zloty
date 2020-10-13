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
#include <string_view>


namespace yaget::meta
{
    // removes any cv, pointers and references qualifiers from T
    // using BaseType = typename meta::strip_qualifiers<T>::type;
    template <typename T>
    struct strip_qualifiers
    {
        using type = typename std::remove_pointer<typename std::decay<T>::type>::type;
    };

    // Helper types
    // using BaseType = typename meta::strip_qualifiers_t<T>;
    template<typename T>
    using strip_qualifiers_t = typename strip_qualifiers<T>::type;

    // chekc if a specific type exist in tuple
    template <typename T1, typename... T2>
    constexpr bool check_for_type(std::tuple<T2...>)
    {
        return std::disjunction_v<std::is_same<T1, T2>...>;
    }

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
            using BaseType = strip_qualifiers_t<RequestedType>;
            //using BaseType = typename std::remove_pointer<typename std::decay<RequestedType>::type>::type;
            BaseType* rt = nullptr;
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

    // return index of tuple based on type
    // Row is some tuple of types <int, float, std::string, ...>
    // T represents a type to find index of, int, std::string, etc.
    // Index then can be used to get element from tuple at that position
    // constexpr std::size_t index = meta::Index<T, Row>::value;
    // return std::get<index>(mDataStorage);
    template <class T, class Tuple>
    struct Index;

    template <class T, class... Types>
    struct Index<T, std::tuple<T, Types...>>
    {
        static const std::size_t value = 0;
    };

    template <class T, class U, class... Types>
    struct Index<T, std::tuple<U, Types...>>
    {
        static const std::size_t value = 1 + Index<T, std::tuple<Types...>>::value;
    };

    // used to create array of visitor objects
    //constexpr temp::meta::Visitor visitor{
    //    [](comp::LocationComponent* component)
    //    {
    //    },
    //    [](comp::PhysicsComponent* component)
    //    {
    //    },
    //};
    //
    //auto v = std::variant<comp::LocationComponent*, comp::PhysicsComponent*, ponger::DebugComponent*, comp::ScriptComponent*, scripting::PythonComponent*>{};
    // use visit of you have variant, or simply call directly
    //std::visit(visitor, v);
    //visitor((comp::LocationComponent*) nullptr);
    template <typename ... Base>
    struct Visitor : Base...
    {
        using Base::operator()...;
    };

    template <typename ... T> Visitor(T...) -> Visitor<T...>;

    // Return name of type
    //const int ci = 0;
    //auto varName = type_name<decltype(ci)>();
    //auto varName = type_name<const int>();
    //
    // const Foo* foo = ...;
    //using base_type = typename std::remove_pointer<typename std::decay<decltype(foo)>::type>::type;
    // base_type is Foo
    template <typename T>
    constexpr std::string_view type_name()
    {
        std::string_view name, prefix, suffix;
#ifdef __clang__
        name = __PRETTY_FUNCTION__;
        prefix = "std::string_view type_name() [T = ";
        suffix = "]";
#elif defined(__GNUC__)
        name = __PRETTY_FUNCTION__;
        prefix = "constexpr std::string_view type_name() [with T = ";
        suffix = "; std::string_view = std::basic_string_view<char>]";
#elif defined(_MSC_VER)
        name = __FUNCSIG__;
        prefix = "class std::basic_string_view<char,struct std::char_traits<char> > __cdecl yaget::meta::type_name<";
        suffix = ">(void)";
#endif
        name.remove_prefix(prefix.size());
        name.remove_suffix(suffix.size());
        return name;
    }

    // meta::ViewToString(meta::type_name<>())
    inline std::string ViewToString(const std::string_view& view)
    {
        return std::string(view.begin(), view.end());
    }

} // namespace yaget::meta


#include <tuple>
#include <utility>

// Based on
// * http://alexpolt.github.io/type-loophole.html
//   https://github.com/alexpolt/luple/blob/master/type-loophole.h
//   by Alexandr Poltavsky, http://alexpolt.github.io
// * https://www.youtube.com/watch?v=UlNUNxLtBI0
//   Better C++14 reflections - Antony Polukhin - Meeting C++ 2018

namespace refl {

// tag<T, N> generates friend declarations and helps with overload resolution.
// There are two types: one with the auto return type, which is the way we read types later.
// The second one is used in the detection of instantiations without which we'd get multiple
// definitions.
template <typename T,int N>
struct tag {
    friend auto loophole(tag<T,N>);
    constexpr friend int cloophole(tag<T,N>);
};

// The definitions of friend functions.
template <typename T,typename U,int N,bool B,
    typename = typename std::enable_if_t<
    !std::is_same_v<
    std::remove_cv_t<std::remove_reference_t<T>>,
    std::remove_cv_t<std::remove_reference_t<U>>>>>
    struct fn_def {
    friend auto loophole(tag<T,N>) { return U{}; }
    constexpr friend int cloophole(tag<T,N>) { return 0; }
};

// This specialization is to avoid multiple definition errors.
template <typename T,typename U,int N> struct fn_def<T,U,N,true> {};

// This has a templated conversion operator which in turn triggers instantiations.
// Important point, using sizeof seems to be more reliable. Also default template
// arguments are "cached" (I think). To fix that I provide a U template parameter to
// the ins functions which do the detection using constexpr friend functions and SFINAE.
template <typename T,int N>
struct c_op {
    template <typename U,int M>
    static auto ins(...) -> int;
    template <typename U,int M,int = cloophole(tag<T,M>{})>
    static auto ins(int) -> char;

    template <typename U,int = sizeof(fn_def<T,U,N,sizeof(ins<U,N>(0)) == sizeof(char)>)>
    operator U();
};

// Here we detect the data type field number. The byproduct is instantiations.
// Uses list initialization. Won't work for types with user-provided constructors.
// In C++17 there is std::is_aggregate which can be added later.
template <typename T,int... Ns>
constexpr int fields_number(...) { return sizeof...(Ns) - 1; }

template <typename T,int... Ns>
constexpr auto fields_number(int) -> decltype(T{ c_op<T, Ns>{}... },0) {
    return fields_number<T,Ns...,sizeof...(Ns)>(0);
}

// Here is a version of fields_number to handle user-provided ctor.
// NOTE: It finds the first ctor having the shortest unambigious set
//       of parameters.
template <typename T,int... Ns>
constexpr auto fields_number_ctor(int) -> decltype(T(c_op<T,Ns>{}...),0) {
    return sizeof...(Ns);
}

template <typename T,int... Ns>
constexpr int fields_number_ctor(...) {
    return fields_number_ctor<T,Ns...,sizeof...(Ns)>(0);
}

// This is a helper to turn a ctor into a tuple type.
// Usage is: refl::as_tuple<data_t>
template <typename T,typename U> struct loophole_tuple;

template <typename T,int... Ns>
struct loophole_tuple<T,std::integer_sequence<int,Ns...>> {
    using type = std::tuple<decltype(loophole(tag<T,Ns>{}))...>;
};

template <typename T>
using as_tuple =
typename loophole_tuple<T,std::make_integer_sequence<int,fields_number_ctor<T>(0)>>::type;

}  // namespace refl