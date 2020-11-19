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
    //-------------------------------------------------------------------------------------------------
    // specialize this to return friendly name
    //template <>
    //struct CoordinatorName <ttt::GameCoordinator::GlobalCoordinator>
    //{
    //    static constexpr const char* Name() { return "Globals"; }
    //};
    template <typename T>
    struct CoordinatorName;

    //-------------------------------------------------------------------------------------------------
    // specialize this to return which id is used for specific coordinator (Global (0) or Entity (1)).
    //template <>
    //struct CoordinatorName <ttt::GameCoordinator::GlobalCoordinator>
    //{
    //    static constexpr const int Value() { return ttt::GameCoordinator::GLOBAL_ID; }
    //};
    template <typename T>
    struct CoordinatorId;

    // Sample of common implementation. Put this in cpp above where you call GenerateGameDirectorSchema 
    //namespace yaget::comp::db
    //{
    //    template <>
    //    struct CoordinatorName <ttt::GameCoordinator::GlobalCoordinator>
    //    {
    //        static constexpr const char* Name() { return "Globals"; }
    //    };

    //    template <>
    //    struct CoordinatorName <ttt::GameCoordinator::EntityCoordinator>
    //    {
    //        static constexpr const char* Name() { return "Entities"; }
    //    };

    //    template <>
    //    struct CoordinatorId <ttt::GameCoordinator::GlobalCoordinator>
    //    {
    //        static constexpr int Value() { return ttt::GameCoordinator::GLOBAL_ID; }
    //    };

    //    template <>
    //    struct CoordinatorId <ttt::GameCoordinator::EntityCoordinator>
    //    {
    //        static constexpr int Value() { return ttt::GameCoordinator::ENTITY_ID; }
    //    };

    //}



    inline void hash_combine(int64_t& /*seed*/) { }

    template <typename T, typename... Rest>
    inline void hash_combine(int64_t& seed, const T& v, Rest... rest)
    {
        std::hash<T> hasher;
        seed ^= hasher(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
        hash_combine(seed, rest...);
    }

    // forward declerations
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
            std::string typeName = meta::ViewToString(meta::type_name<BaseType>());

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
       
        using Columns = std::vector<ColumnData>;

        template <typename T>
        Columns CreateQuery()
        {
            using namespace yaget;

            Columns columns;

            auto callback = [&columns]<typename T0>(const T0&)
            {
                using BaseType = meta::strip_qualifiers_t<T0>;

                auto typeName = internal::ResolveName<T0>();

                const auto propertiesTable = fmt::format("{}Properties", typeName);

                using rowType = typename RowDescription_t<BaseType>::Row;
                using valueType = typename RowDescription_t<BaseType>::Types;
                const auto& propNames = db::GetPolicyRowNames<rowType>();
                const auto& propTypes = db::GetPolicyRowTypes<valueType>();
                YAGET_ASSERT(propNames.size() == propTypes.size(), "propNames has '%d' elements, but propTypes does not match with '%d' elements.", propNames.size(), propTypes.size());
                columns.emplace_back(ColumnData{
                    .mName = typeName,
                    .mPropertyTableName = propertiesTable,
                    .mPropertyNames = propNames,
                    .mPropertyTypes = propTypes
                });
            };

            auto rowTemplate = T();
            meta::for_each(rowTemplate, callback);

            return columns;
        }

        template <typename T>
        std::string MakeTable(const std::string& tableName, std::set<std::string>& propertyNames, Columns& schemeTableData, int coordinatorId)
        {
            auto command{ fmt::format("CREATE TABLE '{}' ('Id' INTEGER, ", tableName) };
            command += "'Name' TEXT DEFAULT '', ";
            command += "'Layer' INTEGER DEFAULT 0, ";
            command += fmt::format("'Coordinator' INTEGER DEFAULT {}, ", coordinatorId);

            const auto& table = CreateQuery<T>();
            for (const auto& element : table)
            {
                command += fmt::format("'{}' INTEGER DEFAULT 0, ", element.mName);
                propertyNames.insert(element.mPropertyTableName);

                schemeTableData.push_back(element);
            }
            command += "PRIMARY KEY('Id'));";

            return  command;
        }

    }

    template <typename T>
    std::string GetTypeTableName()
    {
        return internal::ResolveComponentTableName<T>();
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

    template <typename T>
    struct ComponentRowDescription
    {
        using RowNames = typename RowDescription_t<T>::Row;
        using RowTypes = typename RowDescription_t<T>::Types;
    };

    template <typename T>
    std::string ItemQuery(comp::Id_t id)
    {
        const Strings names = GetPolicyRowNames<T>();
        const auto& command = fmt::format("SELECT {} FROM Entities WHERE Id = {};", conv::Combine(names, ", "), id);

        return command;
    }

    template <typename T>
    void PreCacheInsertComponent(SQLite& db)
    {
        using CRT = ComponentRowDescription<T>;

        const auto& tableName = GetTypeTableName<T>();
        const auto& rowNames = GetPolicyRowNames<typename CRT::RowNames>();
        if (!db.IsStatementCached("ComponentInsert" + tableName))
        {
            db.PreCacheStatementTuple<CRT::RowTypes>("ComponentInsert" + tableName, tableName, rowNames, SQLite::Behaviour::Insert);
        }
    }

    template <typename T>
    bool InsertComponent(SQLite& db, const typename ComponentRowDescription<T>::RowTypes& row)
    {
        const auto& tableName = GetTypeTableName<T>();
        auto result = db.ExecuteStatementTuple("ComponentInsert" + tableName, row);
    
        return result;
    }

    template <typename T>
    void InsertComponentTuple(SQLite& db, const typename ComponentRowDescription<T>::RowTypes& row)
    {
        const auto& tableName = GetTypeTableName<T>();

        if (!db.IsStatementCached("ComponentInsert" + tableName))
        {
            PreCacheInsertComponent<T>(db);
        }

        InsertComponent<T>(db, row);
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
