///////////////////////////////////////////////////////////////////////
// Database.h
//
//  Copyright 8/13/2017 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      Base class to extend usage of databases (SQLite), supports whatever scheme user provides
//
//
//  #include "Database/Database.h"
//
//////////////////////////////////////////////////////////////////////
//! \file

#pragma once

#include "Base.h"
#include "sqlite/SQLite.h"

namespace yaget
{
    class Database
    {
    public:
        constexpr int static NonVersioned = -1;
        Database(const std::string& name, const std::vector<std::string>& schema, int excpectedVersion);
        ~Database();

        SQLite& DB() { return mDatabase; }
        const SQLite& DB() const { return mDatabase; }

        void Log(const std::string& type, const std::string& message);

    private:
        SQLite mDatabase;
    };

} // namespace yaget

