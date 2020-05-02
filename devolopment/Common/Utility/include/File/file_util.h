/////////////////////////////////////////////////////////////////////////
// file_util.h
//
//  Copyright 9/21/2009 Edgar Glowacki.
//
// NOTES:
//
//
// #include "File/file_util.h"
//
/////////////////////////////////////////////////////////////////////////
//! \file

#ifndef FILE_FILE_UTIL_H
#define FILE_FILE_UTIL_H
#pragma once

#include "Base.h"
#include <string>
#include <vector>
#include <boost/filesystem.hpp>

namespace eg
{
    namespace file
    {
        typedef boost::function<std::string (const std::string& name)> filter_predicate_t;
        inline void get_all_files(const boost::filesystem::path& dir_path, std::vector<std::string>& file_names, filter_predicate_t filter_predicate)
        {
            using namespace boost::filesystem;

            if (!exists(dir_path) || !is_directory(dir_path))
            {
                return;
            }

            directory_iterator it(dir_path), end_iter;

            for (; it!= end_iter; ++it)
            {
                if (is_directory(*it))
                {
                    get_all_files(*it, file_names, filter_predicate);
                }
                else if (is_regular_file(*it))
                {
                    std::string fileName = (*it).string();
                    if (filter_predicate)
                    {
                        fileName = filter_predicate(fileName);
                    }
                    
                    if (!fileName.empty())
                    {
                        file_names.push_back(fileName);
                    }
                }
            }
        }
    } // namespace file
} // namespace eg

#endif // FILE_FILE_UTIL_H

