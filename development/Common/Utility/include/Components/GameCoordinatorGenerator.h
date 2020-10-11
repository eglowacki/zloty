//////////////////////////////////////////////////////////////////////
// Coordinator.h
//
//  Copyright 6/16/2019 Edgar Glowacki
//
//  Maintained by: Edgar
//
//  NOTES:
//      Helper framework to generate Director datababe schema
//      for a specific set of user game data, using meta programming.
//      Main input to GenerateGameDirectorSchema is user GameCoordinator type.
//
//      add that where GenerateGameDirectorSchema is called
//      #include "HashUtilities.h"
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

    // forward decleration
    template <typename T>
    Strings GetPolicyRowNames();

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


        Strings ResolveUserStripKeywords(const Strings defaultSet);

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
            using BaseType = typename std::remove_pointer<typename std::decay<T>::type>::type;
            std::string typeName = meta::ViewToString(meta::type_name<BaseType>());

            std::for_each(std::begin(internal::ResolveStripKeywords()), std::end(internal::ResolveStripKeywords()), [&typeName](const auto& element)
            {
                conv::ReplaceAll(typeName, element, "");
            });

            conv::Trim(typeName, ": ");

            return typeName;
        }

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

            auto callback = [&columns]<typename T0>(const T0& element)
            {
                using BaseType = typename std::remove_pointer<typename std::decay<T0>::type>::type;

                auto typeName = internal::ResolveName<T0>();

                const auto propertiesTable = fmt::format("{}Properties", typeName);

                using propertyTypes = typename ComponentProperties<BaseType>::Row;
                const auto& propNames = db::GetPolicyRowNames<propertyTypes>();
                columns.emplace_back(ColumnData{
                    .mName = typeName, 
                    .mPropertyTableName = propertiesTable,
                    .mPropertyNames = propNames
                });
            };

            auto rowTemplate = T();
            meta::for_each(rowTemplate, callback);

            return columns;
        }

        template <typename T>
        std::string MakeTable(int64_t& schemaVersion, const std::string& tableName, std::set<std::string>& propertyNames, Columns& schemeTableData, int coordinatorId)
        {
            auto command{ fmt::format("CREATE TABLE '{}' ('Id' INTEGER, ", tableName) };
            command += "'Name' TEXT DEFAULT '', ";
            command += "'Layer' INTEGER DEFAULT 0, ";
            command += fmt::format("'Coordinator' INTEGER DEFAULT {}, ", coordinatorId);

            const auto& table = CreateQuery<T>();
            for (const auto& element : table)
            {
                //comp::db::hash_combine(schemaVersion,element.mHash);
                command += fmt::format("'{}' INTEGER DEFAULT 0, ", element.mName);
                propertyNames.insert(element.mPropertyTableName);

                schemeTableData.push_back(element);
            }
            command += "PRIMARY KEY('Id'));";

            return  command;
        }

    }

    // call this to generate db scheme representing GameCoordinator
    template <typename T>
    Strings GenerateGameDirectorSchema(int64_t& schemaVersion)
    {
        using namespace yaget;

        using Entity = typename T::Entity;
        using Global = typename T::Global;

        using EntityRow = typename Entity::Row;
        using GlobalRow = typename Global::Row;

        using Coordinators = typename T::Coordinators;

        Strings resultSchema;
        std::set<std::string> propertyNames;
        internal::Columns schemaTableData;

        meta::for_each_type<Coordinators>([&schemaVersion, &propertyNames, &resultSchema, &schemaTableData]<typename T0>(const T0& element)
        {
            using BaseType = typename std::remove_pointer<typename std::decay<T0>::type>::type;

            constexpr auto tableName = CoordinatorName<BaseType>::Name();
            constexpr int coordinatorId = CoordinatorId<BaseType>::Value();

            auto command = internal::MakeTable<BaseType::Row>(schemaVersion, tableName, propertyNames, schemaTableData, coordinatorId);
            resultSchema.emplace_back(command);
        });

        std::sort(schemaTableData.begin(), schemaTableData.end());
        auto it = std::unique(schemaTableData.begin(), schemaTableData.end());
        schemaTableData.resize(std::distance(schemaTableData.begin(), it));

        for (const auto& componentTable : schemaTableData)
        {
            std::string columns;
            for (const auto& element : componentTable.mPropertyNames)
            {
                columns += fmt::format("'{}' {}, ", element, "TEXT");
            }
            // since we do not want to share component types between Coordinators, Coordinator field should be DEPRECATED.
            auto command = fmt::format("CREATE TABLE '{}' ('Id' INTEGER, {} PRIMARY KEY('Id'));", componentTable.mPropertyTableName, columns);
            resultSchema.emplace_back(command);
        }

        std::hash<std::string> hasher;
        schemaVersion = hasher(conv::Combine(resultSchema, ""));

        return resultSchema;
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
    struct ComponentRowTypes
    {
    private:
        using BaseRowName = ComponentProperties<comp::Component>::Row;
        using TName = typename ComponentProperties<T>::Row;
        
        using BaseRowType = ComponentProperties<comp::Component>::Types;
        using TType = typename ComponentProperties<T>::Types;

    public:
        using RowNames = decltype(std::tuple_cat(BaseRowName{}, TName{}));
        using RowTypes = decltype(std::tuple_cat(BaseRowType{}, TType{}));

        static RowTypes DefaultRow() { return std::tuple_cat(ComponentProperties<comp::Component>::DefaultRow(), ComponentProperties<T>::DefaultRow()); }
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
        using CRT = ComponentRowTypes<T>;

        const auto& tableName = GetTypeTableName<T>();
        const auto& rowNames = GetPolicyRowNames<typename CRT::RowNames>();
        if (!db.IsStatementCached("ComponentInsert" + tableName))
        {
            db.PreCacheStatementTuple<CRT::RowTypes>("ComponentInsert" + tableName, tableName, rowNames, SQLite::Behaviour::Insert);
        }
    }

    template <typename T>
    bool InsertComponent(SQLite& db, const typename ComponentRowTypes<T>::RowTypes& row)
    {
        const auto& tableName = GetTypeTableName<T>();
        auto result = db.ExecuteStatementTuple("ComponentInsert" + tableName, row);
    
        return result;
    }

    template <typename T>
    void InsertComponentTuple(SQLite& db, const typename ComponentRowTypes<T>::RowTypes& row)
    {
        const auto& tableName = GetTypeTableName<T>();

        if (!db.IsStatementCached("ComponentInsert" + tableName))
        {
            PreCacheInsertComponent<T>(db);
        }

        InsertComponent<T>(db, row);
    }



}