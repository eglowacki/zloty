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
#include <array>


namespace yaget::meta
{
    //-------------------------------------------------------------------------------------------------
    // make_array_of provides initialization of arrays with same value over all elements 
    //// std::array<int, 4>{42, 42, 42, 42}
    //constexpr auto test_array = make_array_of<4/*, int*/>(42);
    //static_assert(test_array[0] == 42);
    //static_assert(test_array[1] == 42);
    //static_assert(test_array[2] == 42);
    //static_assert(test_array[3] == 42);
    //// static_assert(test_array[4] == 42); out of bounds
    namespace internal
    {
        /// [3]
        /// This functions's only purpose is to ignore the index given as the second
        /// template argument and to always produce the value passed in.
        template<class T, size_t /*ignored*/>
        constexpr T identity_func(const T& value)
        {
            return value;
        }

        /// [2]
        /// At this point, we have a list of indices that we can unfold
        /// into an initializer list using the `identity_func` above.
        template<class T, size_t... Indices>
        constexpr std::array<T, sizeof...(Indices)>
        make_array_of_impl(const T& value, std::index_sequence<Indices...>)
        {
            return { identity_func<T, Indices>(value)... };
        }
    }

    //-------------------------------------------------------------------------------------------------
    /// [1]
    /// This is the user-facing function.
    /// The template arguments are swapped compared to the order used
    /// for std::array, this way we can let the compiler infer the type
    /// from the given value but still define it explicitly if we want to.
    template<size_t Size, class T>
    constexpr std::array<T, Size> make_array_of(const T& value)
    {
        using Indices = std::make_index_sequence<Size>;
        return internal::make_array_of_impl(value, Indices{});
    }


    //-------------------------------------------------------------------------------------------------
    // removes any cv, pointers and references qualifiers from T
    // using BaseType = typename meta::strip_qualifiers<T>::type;
    template <typename T>
    struct strip_qualifiers
    {
        using type = typename std::remove_pointer<typename std::decay<T>::type>::type;
    };

    // removes any cv, pointers and references qualifiers from T
    // using BaseType = meta::strip_qualifiers_t<T>;
    template<typename T>
    using strip_qualifiers_t = typename strip_qualifiers<T>::type;

    // check if a specific type exist in tuple
    template <typename T1, typename... T2>
    constexpr bool check_for_type(std::tuple<T2...>)
    {
        return std::disjunction_v<std::is_same<T1, T2>...>;
    }

    //-------------------------------------------------------------------------------------------------
    // compile time iteration over tuple and calling callback for each entry with Type
    // The Type is passed as Type pointer set to nullptr.
    template <
        typename TTuple,
        size_t Index = 0,
        size_t Size = std::tuple_size_v<std::remove_reference_t<TTuple>>,
        typename TCallable
    >
    constexpr int for_each_type(TCallable&& callable)
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

        return 0;
    }

    // similar as above, but iterates over each element
    template <
        size_t Index = 0,
        typename TTuple,
        size_t Size = std::tuple_size_v<std::remove_reference_t<TTuple>>,
        typename TCallable
    >
    constexpr void for_each(TTuple&& tuple, TCallable&& callable)
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
    constexpr void for_each_pair(TTuple&& tuple, TCallable&& callable)
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

    //-------------------------------------------------------------------------------------------------
    // Makes unique instance from elements and copies
    // that tuple element into another. Both must match size
    // From tuple elements are used like a function call to make a separate copy
    template<int N, typename To, typename From, size_t Size = std::tuple_size_v<std::remove_reference_t<To>>>
    constexpr void tuple_clone(To& to, const From& from)
    {
        std::get<N>(to) = std::get<N>(from)();
        if constexpr (N + 1 < Size)
        {
            meta::tuple_clone<N + 1, To, From>(to, from);
        }
    }

    struct TupleCopyPolicy
    {
        template <typename TS, typename TT>
        static constexpr TS Copy(TS source, TT target)
        {
            return source ? source : target;
        }
    };

    // Copy tuple from Source to Target, if Source element is false/nullptr, it will skip and preserve value in Target
    // Use TupleCopyPolicy to customize how to copy element From To
    template<typename S, typename T, typename P = TupleCopyPolicy, int N = std::tuple_size_v<std::remove_reference_t<S>>>
    constexpr void tuple_copy(const S& source, T& target)
    {
        using ET = typename std::tuple_element<N - 1, S>::type;

        std::get<ET>(target) = P::template Copy<ET>(std::get<ET>(source), std::get<ET>(target));

        if constexpr (N - 1 > 0)
        {
            tuple_copy<S, T, P, N - 1>(source, target);
        }
    }

    //-------------------------------------------------------------------------------------------------
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
        static constexpr std::size_t value = 1 + Index<T, std::tuple<Types...>>::value;
    };


    //-------------------------------------------------------------------------------------------------
    // Finding a type in a tuple by Raymond
    // https://devblogs.microsoft.com/oldnewthing/20200629-00/?p=103910
    //// index = 1
    //    constexpr std::size_t index =
    //    tuple_element_index_v<int, std::tuple<char, int, float>>;

    //// error: type does not appear in tuple
    //constexpr std::size_t index =
    //    tuple_element_index_v<double, std::tuple<char, int, float>>;

    //// error: type appears more than once in tuple
    //constexpr std::size_t index =
    //    tuple_element_index_v<int, std::tuple<char, int, int>>;
    //
    template<typename T, typename Tuple>
    struct tuple_element_index_helper;

    template<typename T>
    struct tuple_element_index_helper<T, std::tuple<>>
    {
        static constexpr std::size_t value = 0;
    };

    template<typename T, typename... Rest>
    struct tuple_element_index_helper<T, std::tuple<T, Rest...>>
    {
        static constexpr std::size_t value = 0;
        using RestTuple = std::tuple<Rest...>;
        static_assert(tuple_element_index_helper<T, RestTuple>::value == std::tuple_size_v<RestTuple>,
            "type appears more than once in tuple");
    };

    template<typename T, typename First, typename... Rest>
    struct tuple_element_index_helper<T, std::tuple<First, Rest...>>
    {
        using RestTuple = std::tuple<Rest...>;
        static constexpr std::size_t value = 1 + tuple_element_index_helper<T, RestTuple>::value;
    };

    template<typename T, typename Tuple>
    struct tuple_element_index
    {
        static constexpr std::size_t value = tuple_element_index_helper<T, Tuple>::value;
        static_assert(value < std::tuple_size_v<Tuple>, "type does not appear in tuple");
    };

    template<typename T, typename Tuple>
    inline constexpr std::size_t tuple_element_index_v = tuple_element_index<T, Tuple>::value;

    template<typename T, typename Tuple>
    struct tuple_element_not_index
    {
        static constexpr std::size_t value = tuple_element_index_helper<T, Tuple>::value;
        static_assert(value == std::tuple_size_v<Tuple>, "type appears in tuple");
    };

    template<typename T, typename Tuple>
    inline constexpr std::size_t tuple_element_not_index_v = tuple_element_not_index<T, Tuple>::value;


    template<typename T, typename Tuple>
    struct tuple_is_element
    {
        static constexpr bool value = tuple_element_index_helper<T, Tuple>::value<std::tuple_size_v<Tuple>;
    };

    template<typename T, typename Tuple>
    inline constexpr bool tuple_is_element_v = tuple_is_element<T, Tuple>::value;

    using bits_t = std::size_t;
    namespace internal
    {
        template<typename T, typename U, int N = std::tuple_size_v<std::remove_reference_t<U>>>
        constexpr bits_t tuple_bits()
        {
            static_assert(std::tuple_size_v<std::remove_reference_t<T>> <= std::numeric_limits<bits_t>::digits, "Number of components in T exceeds bits_t ");
            using Template = T;
            using UserRow = U;

            using UserElementType = typename std::tuple_element<N - 1, UserRow>::type;

            constexpr bits_t result = 1 << yaget::meta::Index<UserElementType, Template>::value;

            if constexpr (N - 1 > 0)
            {
                return result | tuple_bits<T, U, N - 1>();
            }
            else
            {
                return result;
            }
        }
        template <std::size_t TupleIndex, std::size_t MaxTupleSize, typename... Tuple>
        constexpr auto tuple_combine()
        {
            using Tuples = std::tuple<Tuple...>;

            constexpr auto currentRow = std::get<TupleIndex>(Tuples{});
            if constexpr (TupleIndex + 1 < MaxTupleSize)
            {
                constexpr auto nextRow = tuple_combine<TupleIndex + 1, MaxTupleSize, Tuple...>();
                return std::tuple_cat(currentRow, nextRow);
            }
            else
            {
                return currentRow;
            }
        }

        template <
            size_t Index = 0
            , typename TTuple
            , size_t Size = std::tuple_size_v<std::remove_reference_t<TTuple>>
        >
        constexpr bool tuple_is_unique()
        {
            if constexpr (Index < Size)
            {
                using ThisType = std::tuple_element_t<Index, TTuple>;
                yaget::meta::tuple_is_element_v<ThisType, TTuple>;

                if constexpr (Index + 1 < Size)
                {
                    return tuple_is_unique<Index + 1, TTuple>();
                }

                return true;
            }
        }

    }

    // return bit pattern representing where types from T are located in R
    // Like give me components that represent location of any entity (Tree)
    // using Location = std::tuple<int, bool);
    // using Tree = std::tuple<float, const char*, bool, double, int>;
    // auto bits = tuple_bit_pattern<Location, Tree>::value;
    // auto bits = tuple_bit_pattern_v<Location, Tree>;
    // note that tuple position is from left to right, but bit representation follows least significant
    // 00000
    // 10100 
    template <typename T, typename U>
    struct tuple_bit_pattern
    {
        static constexpr bits_t value = internal::tuple_bits<T, U>();
    };

    template<typename T, typename U = T>
    inline constexpr bits_t tuple_bit_pattern_v = tuple_bit_pattern<T, U>::value;

    // samples:
    //using TestOne = std::tuple<int, bool, float, char>;
    //using TestTwo = std::tuple<float, char, int, char>;
    //using Row = meta::tuple_combine_t<TestOne, TestTwo>;
    //EXPECT_EQ(typeid(Row), typeid(std::tuple<int, bool, float, char, float, char, int, char>));

    template <typename... Tuple>
    struct tuple_combine
    {
        using type = decltype(internal::tuple_combine<0, std::tuple_size_v<std::remove_reference_t<std::tuple<Tuple...>>>, Tuple...>());
    };

    template<typename... Tuple>
    using tuple_combine_t = typename tuple_combine<Tuple...>::type;

    // Compile time error check if any value types are duplicated in tuple
    // using Test1 = std::tuple<int, char>;
    // meta::tuple_is_unique_v<Test1>;
    // using Test2 = std::tuple<int, char, int>;
    // meta::tuple_is_unique_v<Test2>;  // <-- compile error 'error C2338: type appears more than once in tuple'
    template <typename T>
    inline constexpr auto tuple_is_unique_v = internal::tuple_is_unique<0, T>();

    //-------------------------------------------------------------------------------------------------
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

    //-------------------------------------------------------------------------------------------------
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

    template <typename T>
    inline std::string type_name_v()
    {
        constexpr auto n = type_name<T>();
        return std::string(n.begin(), n.end());
    }

} // namespace yaget::meta


////-------------------------------------------------------------------------------------------------
//#include <tuple>
//#include <utility>
//
//// Based on
//// * http://alexpolt.github.io/type-loophole.html
////   https://github.com/alexpolt/luple/blob/master/type-loophole.h
////   by Alexandr Poltavsky, http://alexpolt.github.io
//// * https://www.youtube.com/watch?v=UlNUNxLtBI0
////   Better C++14 reflections - Antony Polukhin - Meeting C++ 2018
//
//namespace refl {
//
//// tag<T, N> generates friend declarations and helps with overload resolution.
//// There are two types: one with the auto return type, which is the way we read types later.
//// The second one is used in the detection of instantiations without which we'd get multiple
//// definitions.
//template <typename T,int N>
//struct tag {
//    friend auto loophole(tag<T,N>);
//    constexpr friend int cloophole(tag<T,N>);
//};
//
//// The definitions of friend functions.
//template <typename T,typename U,int N,bool B,
//    typename = typename std::enable_if_t<
//    !std::is_same_v<
//    std::remove_cv_t<std::remove_reference_t<T>>,
//    std::remove_cv_t<std::remove_reference_t<U>>>>>
//    struct fn_def {
//    friend auto loophole(tag<T,N>) { return U{}; }
//    constexpr friend int cloophole(tag<T,N>) { return 0; }
//};
//
//// This specialization is to avoid multiple definition errors.
//template <typename T,typename U,int N> struct fn_def<T,U,N,true> {};
//
//// This has a templated conversion operator which in turn triggers instantiations.
//// Important point, using sizeof seems to be more reliable. Also default template
//// arguments are "cached" (I think). To fix that I provide a U template parameter to
//// the ins functions which do the detection using constexpr friend functions and SFINAE.
//template <typename T,int N>
//struct c_op {
//    template <typename U,int M>
//    static auto ins(...) -> int;
//    template <typename U,int M,int = cloophole(tag<T,M>{})>
//    static auto ins(int) -> char;
//
//    template <typename U,int = sizeof(fn_def<T,U,N,sizeof(ins<U,N>(0)) == sizeof(char)>)>
//    operator U();
//};
//
//// Here we detect the data type field number. The byproduct is instantiations.
//// Uses list initialization. Won't work for types with user-provided constructors.
//// In C++17 there is std::is_aggregate which can be added later.
//template <typename T,int... Ns>
//constexpr int fields_number(...) { return sizeof...(Ns) - 1; }
//
//template <typename T,int... Ns>
//constexpr auto fields_number(int) -> decltype(T{ c_op<T, Ns>{}... },0) {
//    return fields_number<T,Ns...,sizeof...(Ns)>(0);
//}
//
//// Here is a version of fields_number to handle user-provided ctor.
//// NOTE: It finds the first ctor having the shortest unambigious set
////       of parameters.
//template <typename T,int... Ns>
//constexpr auto fields_number_ctor(int) -> decltype(T(c_op<T,Ns>{}...),0) {
//    return sizeof...(Ns);
//}
//
//template <typename T,int... Ns>
//constexpr int fields_number_ctor(...) {
//    return fields_number_ctor<T,Ns...,sizeof...(Ns)>(0);
//}
//
//// This is a helper to turn a ctor into a tuple type.
//// Usage is: refl::as_tuple<data_t>
//template <typename T,typename U> struct loophole_tuple;
//
//template <typename T,int... Ns>
//struct loophole_tuple<T,std::integer_sequence<int,Ns...>> {
//    using type = std::tuple<decltype(loophole(tag<T,Ns>{}))...>;
//};
//
//template <typename T>
//using as_tuple =
//typename loophole_tuple<T,std::make_integer_sequence<int,fields_number_ctor<T>(0)>>::type;
//
//}  // namespace refl
