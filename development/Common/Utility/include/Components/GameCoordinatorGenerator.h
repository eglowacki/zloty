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

#include "YagetCore.h"
#include "StringHelpers.h"
#include "sqlite/SQLite.h"
#include "Components/Component.h"
#include "VTS/ResolvedAssets.h"
#include "VTS/VirtualTransportSystem.h"


namespace yaget::comp::db
{

    inline void hash_combine(int64_t& /*seed*/) { }

    template <typename T, typename... Rest>
    inline void hash_combine(int64_t& seed, const T& v, Rest... rest)
    {
        std::hash<T> hasher;
        seed ^= hasher(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
        hash_combine(seed, rest...);
    }

    // forward declarations
    template <typename T>
    Strings GetPolicyRowNames();

    template <typename T>
    Strings GetPolicyRowTypes();

    namespace internal
    {
        const Strings stripKeywords{
            "::yaget", "yaget::",
            "::comp", "comp::",
            "::db", "db::",
            "::io", "io::",
            "class",
            "struct"
        };


        Strings ResolveUserStripKeywords(const Strings& defaultSet);

        inline const Strings& ResolveStripKeywords()
        {
            static Strings keywords = ResolveUserStripKeywords(stripKeywords);
            return keywords;
        }

        struct ColumnData
        {
            std::string mName;                  // name of the column, this defaults to class_name
            std::string mPropertyTableName;
            Strings mPropertyNames;
            Strings mPropertyTypes;

            bool operator<(const ColumnData& v) const
            {
                return mName < v.mName;
            }
            bool operator==(const ColumnData& v) const
            {
                return mName == v.mName;
            }
        };

        template <typename T>
        std::string ResolveName()
        {
            using BaseType = meta::strip_qualifiers_t<T>;
            std::string typeName = meta::type_name_v<BaseType>();

            std::for_each(std::begin(internal::ResolveStripKeywords()), std::end(internal::ResolveStripKeywords()), [&typeName](const auto& element)
            {
                conv::ReplaceAll(typeName, element, "");
            });

            conv::Trim(typeName, ": ");

            return typeName;
        }

        // used in outputting sqlite type from c++ types.
        template <typename T>
        inline std::string ResolveDatabaseType() { return "STRING"; }

        template<>
        inline std::string ResolveDatabaseType<int>() { return "INTEGER"; }

        template<>
        inline std::string ResolveDatabaseType<unsigned int>() { return "INTEGER"; }
        template<>

        inline std::string ResolveDatabaseType<int64_t>() { return "INTEGER"; }
        template<>

        inline std::string ResolveDatabaseType<uint64_t>() { return "INTEGER"; }

        template<>
        inline std::string ResolveDatabaseType<float>() { return "REAL"; }

        template<>
        inline std::string ResolveDatabaseType<double>() { return "REAL"; }

        template <typename T>
        std::string ResolveComponentTableName()
        {
            auto typeName = internal::ResolveName<T>();
            return fmt::format("{}Properties", typeName);
        }

    }

    template <typename T>
    Strings GetPolicyRowNames()
    {
        using RowType = T;
        Strings results;

        meta::for_each_type<RowType>([&results]<typename T0>(const T0&)
        {
            auto typeName = internal::ResolveName<T0>();
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
    template <typename T>
    Strings GenerateSystemsCoordinatorSchema()
    {
        using namespace yaget;

        using SystemsCoordinator = T;
        using FullRow = typename SystemsCoordinator::CoordinatorSet::FullRow;

        Strings results;
        meta::for_each_type<FullRow>([&results]<typename T0>(const T0&)
        {
            using BaseType = meta::strip_qualifiers_t<T0>;
            using ParameterNames = typename comp::db::RowDescription_t<BaseType>::Row;
            using ParameterPack = typename comp::db::RowDescription_t<BaseType>::Types;
            static_assert(std::tuple_size_v<ParameterNames> == std::tuple_size_v<ParameterPack>, "Names and types of Component properties must match in size");

            const auto& tableName = internal::ResolveName<BaseType>();
            const auto& columnNames = comp::db::GetPolicyRowNames<ParameterNames>();
            const auto& typeNames = comp::db::GetPolicyRowTypes<ParameterPack>();

            std::string sqlCommand = fmt::format("CREATE TABLE '{}' ('Id' {} CHECK(Id > 0) UNIQUE", tableName, internal::ResolveDatabaseType<comp::Id_t>());
            if (!columnNames.empty())
            {
                auto cn_it = columnNames.begin();
                auto tn_it = typeNames.begin();
                for (; cn_it != columnNames.end(); ++cn_it, ++tn_it)
                {
                    sqlCommand += fmt::format(", '{}' {}", *cn_it, *tn_it);
                }
            }
            sqlCommand += ", PRIMARY KEY('Id'));";
            YLOG_NOTICE("TTT", "[%s]", sqlCommand.c_str());
            results.emplace_back(sqlCommand);
        });

        return results;
    };

    template <typename T>
    int64_t GenerateSystemsCoordinatorVersion()
    {
        using namespace yaget;

        using SystemsCoordinator = T;
        using FullRow = typename SystemsCoordinator::CoordinatorSet::FullRow;

        int64_t schemaVersion = 0;
        meta::for_each_type<FullRow>([&schemaVersion]<typename T0>(const T0&)
        {
            using BaseType = meta::strip_qualifiers_t<T0>;
            using ParameterNames = typename comp::db::RowDescription_t<BaseType>::Row;
            using ParameterPack = typename comp::db::RowDescription_t<BaseType>::Types;
            static_assert(std::tuple_size_v<ParameterNames> == std::tuple_size_v<ParameterPack>, "Names and types of Component properties must match in size");

            const auto& tableName = internal::ResolveName<BaseType>();
            const auto& columnNames = comp::db::GetPolicyRowNames<ParameterNames>();
            const auto& typeNames = comp::db::GetPolicyRowTypes<ParameterPack>();

            hash_combine(schemaVersion, tableName);
            if (!columnNames.empty())
            {
                auto cn_it = columnNames.begin();
                auto tn_it = typeNames.begin();
                for (; cn_it != columnNames.end(); ++cn_it, ++tn_it)
                {
                    hash_combine(schemaVersion, *cn_it, *tn_it);
                }
            }
        });

        return schemaVersion;
    }

    template <typename T>
    Strings GenerateSystemsCoordinatorSchemaVersion(int64_t& schemaVersion)
    {
        Strings results = GenerateSystemsCoordinatorSchema<T>();
        schemaVersion = GenerateSystemsCoordinatorVersion<T>();
        return results;
    }

    struct EmptySchema {};

    template <>
    inline Strings GenerateSystemsCoordinatorSchema<EmptySchema>()
    {
        return {};
    }

    template <>
    inline int64_t GenerateSystemsCoordinatorVersion<EmptySchema>()
    {
        return 0;
    }

    template <>
    inline Strings GenerateSystemsCoordinatorSchemaVersion<EmptySchema>(int64_t& schemaVersion)
    {
        Strings results = GenerateSystemsCoordinatorSchema<EmptySchema>();
        schemaVersion = GenerateSystemsCoordinatorVersion<EmptySchema>();
        return results;
    }

    struct PolicyName
    {
        constexpr static bool AutoComponent = true;
    };

    static constexpr const char* NewItem_Token = "NEW_ITEM";

    template <typename T, typename PolicyName = PolicyName>
    Strings GenerateDirectorLoadout(io::VirtualTransportSystem& vts, const std::string& name)
    {
        using namespace yaget;
        using Section = io::VirtualTransportSystem::Section;

        using SystemsCoordinator = T;
        using FullRow = typename SystemsCoordinator::CoordinatorSet::FullRow;

        Strings results;

        const Section directorSection(name);
        io::SingleBLobLoader<io::JsonAsset> directorBlobLoader(vts, directorSection);
        if (auto asset = directorBlobLoader.GetAsset())
        {
            if (json::IsSectionValid(asset->root, "Description", "Items"))
            {
                const auto& itemsBlock = json::GetSection(asset->root, "Description", "Items");
                for (const auto& itemBlock : itemsBlock)
                {
                    results.emplace_back(NewItem_Token);
                    for (const auto& componentBlock : itemBlock)
                    {
                        auto componentName = json::GetValue<std::string>(componentBlock, "Type", {});
                        YAGET_UTIL_THROW_ASSERT("TTT", !componentName.empty(), "Component Type can not be empty and must have one of game components names.");

                        if (!componentName.ends_with("Component") && PolicyName::AutoComponent)
                        {
                            componentName += "Component";

                            meta::for_each_type<FullRow>([&componentName, &componentBlock, &results]<typename T0>(const T0&)
                            {
                                using BaseType = meta::strip_qualifiers_t<T0>;
                                using ParameterPack = typename comp::db::RowDescription_t<BaseType>::Types;

                                const auto& tableName = internal::ResolveName<BaseType>();
                                if (tableName == componentName)
                                {
                                    const auto componentParams = json::GetValue<ParameterPack>(componentBlock, "Params", {});
                                    std::string message;
                                    print_tuple(componentParams, message);

                                    std::string sqCommand = fmt::format("INSERT INTO '{}' VALUES({{}}{}{});", tableName, message.empty() ? "" : ", ", message);
                                    YLOG_NOTICE("TTT", "[%s] ", sqCommand.c_str());
                                    results.emplace_back(sqCommand);
                                }
                            });
                        }
                    }
                }
            }
        }

        return results;
    }

}
