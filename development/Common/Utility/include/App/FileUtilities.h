//////////////////////////////////////////////////////////////////////
// FileUtilities.h
//
//  Copyright 3/18/2018 Edgar Glowacki
//
//  Maintained by: Edgar
//
//  NOTES:
//      Provides getting files from local disk (platform independent)
//
//
//  #include "App/FileUtilities.h"
//
//////////////////////////////////////////////////////////////////////
//! \file

#pragma once

#include "YagetCore.h"
#include "Streams/Buffers.h"
#include <filesystem>
#include <streambuf>


namespace yaget::io::file
{
    //! Provides stream that operates on already allocated memory buffer.
    //! It does not take ownership of memory
    struct membuf : std::streambuf
    {
        membuf(char const* base, size_t size)
        {
            char* p(const_cast<char*>(base));
            this->setg(p, p, p + size);
        }
    };

    struct imemstream : virtual membuf, std::istream
    {
        imemstream(char const* base, size_t size)
            : membuf(base, size)
            , std::istream(static_cast<std::streambuf*>(this))
        {}
    };

    //! sourceLocation - Starting location to search for files. 
    //! filter - Functor to filter which file to process. Return true to include the file, otherwise skip it .
    //! Return vector of strings where each entry is absolute path to each file found under sourceLocation, and recursively under all sub-folders.
    Strings GetFileNames(const std::string& sourceLocation, bool recursive, std::function<bool(const std::string & fileName)> filter);
    //! Helper function to simply pass wild card filter rather then lambda
    Strings GetFileNames(const std::string& sourceLocation, bool recursive, const std::string& filter);

    //! Creates search path for config (json) file
    //! Order of searches, .json extension is added to name
    //! command line option if one exist (name + "_file")
    //! application folder/name + "_file"
    //! settings folder/name + "_file"
    //! application folder/name
    //! settings folder/name
    //! param name - name of config file
    //! param addAppName - if true then combine name + $(AppName), otherwise just use name
    //! param options - if valid, then look for name + "_file" option value and add to search path
    Strings GenerateConfigSearchPath(const std::string& name, bool addAppName, const args::Options* options);

    //! Return file path to requested config, or empty string if file does not exist in any of search paths
    //! Internally, it calls GenerateConfigSearchPath.
    std::string FindConfigFile(const std::string& name, bool addAppName, const args::Options* options);

    // Returned by some file functions
    // first - success (true) or failure (false)
    // second - error text if failure (first = false).
    using FileOpResult = std::tuple<bool, std::string>;
    //! Remove/delete file from local storage. This is blocking request
    //! Return <true, ""> if no error and file removed successfully or file did not exist
    //!        <false, "some error text"> if error occurred.
    //! usage example:
    //!     const auto& [result, errorMessage] = file::RemoveFile("file.txt");
    FileOpResult RemoveFile(const std::string& fileName);
    std::vector<FileOpResult> RemoveFiles(const Strings& fileNames);

    FileOpResult RenameFile(const std::string& oldFileName, const std::string& newFileName);

    //! Save data in buffer to a file
    FileOpResult SaveFile(const std::string& fileName, const io::Buffer& buffer);

    //! Return true if file exist, otherwise false
    bool IsFileExists(const std::string& fileName);

    //! Set file attribute to one of Attributes enum
    enum class Attributes { ReadOnly, ReadWrite };
    FileOpResult SetFileAtrribute(const std::string& fileName, Attributes attribute);
    bool IsFileAtrribute(const std::string& fileName, Attributes attribute);

    //! Makes sure that all folders exist on the drive
    //! pathName is valid path name. If the final component of the path is a directory, not a file name, 
    //! the string must end with a backslash character.
    FileOpResult AssureDirectories(const std::string& pathName);

    // Splits filePath into three components:
    // folderName
    // fileName
    // extension
    using FileComponents = std::tuple<std::string /*folderName*/, std::string /*fileName*/, std::string /*extension*/>;
    namespace FileComp
    {
        constexpr int Path = 0;
        constexpr int File = 1;
        constexpr int Extension = 2;
    }
    FileComponents SplitComponents(const std::filesystem::path& filePath);

    enum class Traverse { Deep, Flat };
    void EnumerateDirectoryContent(std::vector<std::string>& foundFiles, const std::string& sourceLocation, Traverse traverse, std::function<bool(const std::string & fileName)> filter);

} // yaget::io::file

