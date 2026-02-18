//////////////////////////////////////////////////////////////////////
// GameCoordinatorGenerator.h
//
//  Copyright 6/16/2020 Edgar Glowacki
//
//  Maintained by: Edgar
//
//  NOTES:
//      Helper framework to generate Director database schema
//      for a specific set of user game data, using meta programming.
//      Main input to GenerateGameDirectorSchema is user derived, aliased GameCoordinator type.
//
//      add that where GenerateGameDirectorSchema is called
//      #include "HashUtilities.h"
//
//      and for any components used in template declarations, include relevant header for that component
//      in the same place as above.
//
//
//  #include "Components/GameCoordinatorGenerator.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "HashUtilities.h"
#include "Core/ErrorHandlers.h"
#include "sqlite/SQLite.h"
#include "Components/Component.h"
#include "VTS/ResolvedAssets.h"


namespace yaget::comp::db
{
    namespace internal
    {
        inline void ClearKeyword(std::string& text, const char* keyword)
        {
            if (text.starts_with(keyword))
            {
                text.erase(0, strlen(keyword));
            }
            
        }

        template <typename T>
        std::string ResolveName()
        {
            using BaseType = meta::strip_qualifiers_t<T>;
            std::string typeName = meta::type_name_v<BaseType>();

            const auto result = typeName.find_last_of("::");
            if (result != std::string::npos)
            {
                typeName.erase(0, result+1);
            }

            ClearKeyword(typeName, "struct ");
            ClearKeyword(typeName, "class ");

            conv::Trim(typeName, ": ");
            return typeName;
        }

        // used in outputting sqlite type from c++ types.
        template <typename T>
        std::string ResolveDatabaseType() { return "STRING"; }

        template<>
        inline std::string ResolveDatabaseType<int>() { return "INTEGER"; }

        template<>
        inline std::string ResolveDatabaseType<unsigned int>() { return "INTEGER"; }

        template<>
        inline std::string ResolveDatabaseType<int64_t>() { return "INTEGER"; }

        template<>
        inline std::string ResolveDatabaseType<float>() { return "REAL"; }

        template<>
        inline std::string ResolveDatabaseType<double>() { return "REAL"; }

        template<typename T>
        concept has_coordinatorset = requires { typename T::CoordinatorSet; };

    }

    template <typename T>
    std::string ResolveName()
    {
        return internal::ResolveName<T>();
    }

    template <typename T>
    Strings GetPolicyRowNames()
    {
        using RowType = T;
        Strings results;

        meta::for_each_type<RowType>([&results]<typename T0>(const T0&)
        {
            auto typeName = ResolveName<T0>();
            results.emplace_back(typeName);
        });

        return results;
    }

    template <typename T>
    Strings GetPolicyRowTypes()
    {
        using RowType = T;
        Strings results;

        meta::for_each_type<RowType>([&results]<typename T0>(const T0&)
        {
            using BaseType = meta::strip_qualifiers_t<T0>;
            auto typeName = internal::ResolveDatabaseType<BaseType>();
            results.emplace_back(typeName);
        });

        return results;
    }

    // Generates sql schema for items composed of various components,
    // each component composed of columns, which are properties of values (parameters to ctor)
    template <typename T, typename R>
    R GenerateSystemsCoordinator2(auto callback)
    {
        using namespace yaget;

        R results{};
        if constexpr (internal::has_coordinatorset<T>)
        {
            using FullRow = typename T::CoordinatorSet::FullRow;

            meta::for_each_type<FullRow>([&results, &callback]<typename T0>(const T0&)
            {
                using BaseType = meta::strip_qualifiers_t<T0>;
                using ParameterNames = typename RowDescription_t<BaseType>::Row;
                using ParameterPack = typename RowDescription_t<BaseType>::Types;
                static_assert(std::tuple_size_v<ParameterNames> == std::tuple_size_v<ParameterPack>, "Names and types of Component properties must match in size");

                const auto& tableName = ResolveName<BaseType>();
                const auto& columnNames = GetPolicyRowNames<ParameterNames>();
                const auto& typeNames = GetPolicyRowTypes<ParameterPack>();

                results = callback(results, tableName, columnNames, typeNames);
            });
        }

        return results;
    }


    struct EmptySchema {};

    enum class GenerateCoordinator
    {
        Version,
        Schema,
        Log
    };

    template <typename T, GenerateCoordinator F>
    auto GenerateSystemsCoordinator()
    {
        if constexpr (F == GenerateCoordinator::Schema)
        {
            return GenerateSystemsCoordinator2<T, Strings>([](auto results, auto tableName, auto columnNames, auto typeNames)
            {
                std::string sqlCommand = fmt::format("CREATE TABLE '{}' ('Id' {} CHECK(Id != 0) UNIQUE", tableName, internal::ResolveDatabaseType<comp::Id_t>());

                auto cn_it = columnNames.begin();
                auto tn_it = typeNames.begin();
                for (; cn_it != columnNames.end(); ++cn_it, ++tn_it)
                {
                    sqlCommand += fmt::format(", '{}' {}", *cn_it, *tn_it);
                }
            
                sqlCommand += ", PRIMARY KEY('Id'));";
                YLOG_DEBUG("GSYS", "[%s]", sqlCommand.c_str());
                results.emplace_back(sqlCommand);

                return results;
            });
        }
        if constexpr (F == GenerateCoordinator::Version)
        {
            return GenerateSystemsCoordinator2<T, int64_t>([](auto results, auto tableName, auto columnNames, auto typeNames)
            {
                conv::hash_combine(results, tableName);
                auto cn_it = columnNames.begin();
                auto tn_it = typeNames.begin();
                for (; cn_it != columnNames.end(); ++cn_it, ++tn_it)
                {
                    conv::hash_combine(results, *cn_it, *tn_it);
                }

                return results;
            });
        }
        if constexpr (F == GenerateCoordinator::Log)
        {

            auto lines = GenerateSystemsCoordinator2<T, Strings>([](auto results, auto tableName, auto columnNames, auto typeNames)
            {
                std::string logLine = fmt::format("====: '{}'\n\t\t\t{} id", tableName, ResolveName<comp::Id_t>());

                auto cn_it = columnNames.begin();
                auto tn_it = typeNames.begin();
                for (; cn_it != columnNames.end(); ++cn_it, ++tn_it)
                {
                    logLine += fmt::format(",\n\t\t\t'{}' {}", *tn_it, *cn_it);
                }

                if (!columnNames.empty())
                {
                    //logLine += "\n";
                }
            
                results.emplace_back(logLine);

                return results;
            });

            auto systemNames = fmt::format("=== Game System Hierarchy:\n\tTop Coordinator: '{}' with registered systems:\n", ResolveName<T>());
            systemNames += "\t\t" + conv::Combine(GetPolicyRowNames<typename T::Systems>(), "\n\t\t");
            systemNames += "\n\tOperating on components:\n\t\t" + conv::Combine(lines, "\n\t\t");

            return systemNames;


            //lines.insert(lines.begin(), fmt::format("Root Coordinator: {}\n", ResolveName<T>()));
            //auto systemNames = conv::Combine(GetPolicyRowNames<typename T::Systems>(), "\n");
            //systemNames.insert(systemNames.begin(), fmt::format("Root Coordinator: {}\n", ResolveName<T>()));
            //systemNames.insert(systemNames.end(), lines.begin(), lines.end());


            //return conv::Combine(systemNames, nullptr);
        }
    }
}
