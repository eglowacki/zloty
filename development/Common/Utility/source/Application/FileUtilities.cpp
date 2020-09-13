#include "App/FileUtilities.h"
#include "App/AppUtilities.h"
#include "Platform/Support.h"
#include "Logger/YLog.h"
#include "StringHelpers.h"
#include "Debugging/Assert.h"
#include "Platform/WindowsLean.h"

#include <comdef.h>

//#pragma warning( push )
//#pragma warning( disable : 4091 )  // ' ' : ignored on left of '' when no variable is declared
#include <Dbghelp.h>
//#pragma warning( pop )

#include <filesystem>
namespace fs = std::filesystem;

// Notes:
//  To specify an extended-length path, use the "\\?\" prefix. For example, "\\?\D:\very long path".


namespace
{
    //-------------------------------------------------------------------------------------------------------------------------------
    void FilterFileNames(std::vector<std::string>& foundFiles, const std::string& sourceLocation, std::function<bool(const std::string& fileName)> filter)
    {
        fs::path sourcePath = sourceLocation;
        if (fs::is_directory(sourcePath))
        {
            yaget::Strings directories;
            for (const auto& p : fs::directory_iterator(sourcePath))
            {
                fs::path currentFilePath = p;
    
                if (fs::is_directory(currentFilePath))
                {
                    directories.push_back(currentFilePath.generic_string());
                }
                else if (filter(currentFilePath.string()))
                {
                    foundFiles.push_back(currentFilePath.generic_string());
                }
            }

            for (const auto& p : directories)
            {
                FilterFileNames(foundFiles, p, filter);
            }
        }
        else if (fs::is_regular_file(sourcePath) && filter(sourcePath.string()))
        {
            foundFiles.push_back(sourcePath.generic_string());
        }
    }


    //-------------------------------------------------------------------------------------------------------------------------------
    void AddSearchPath(yaget::Strings& searches, const std::string& name)
    {
        using namespace yaget;

        std::string searchPath = util::ExpendEnv("$(AppFolder)/" + name, "json");
        searches.push_back(searchPath);

        searchPath = util::ExpendEnv("$(ConfigurationFolder)/" + name, "json");
        searches.push_back(searchPath);

        searchPath = util::ExpendEnv("$(RootFolder)/Data/" + name, "json");
        searches.push_back(searchPath);

        if (util::IsEnvironment("$(DataFolder)"))
        {
            searchPath = util::ExpendEnv("$(DataFolder)/" + name, "json");
            searches.push_back(searchPath);
        }
    }

} // namespace


//-------------------------------------------------------------------------------------------------------------------------------
yaget::Strings yaget::io::file::GetFileNames(const std::string& sourceLocation, bool recursive, std::function<bool(const std::string& fileName)> filter)
{
    Strings foundFiles;

    double timeStart = platform::GetRealTime();
    if (fs::is_directory(sourceLocation))
    {
        foundFiles.reserve(2000);
        std::string canonicalPath = sourceLocation;
        if (*sourceLocation.rbegin() == '/' || *sourceLocation.rbegin() == '\\')
        {
            canonicalPath.assign(sourceLocation.begin(), sourceLocation.end() - 1);
        }
        canonicalPath = fs::path(canonicalPath).make_preferred().string();
        EnumerateDirectoryContent(foundFiles, canonicalPath, recursive ? Traverse::Deep : Traverse::Flat, filter);
    }
    else
    {
        std::string canonicalPath = fs::path(sourceLocation).make_preferred().string();
        FilterFileNames(foundFiles, canonicalPath, filter);
    }

    std::sort(foundFiles.begin(), foundFiles.end());
    double timeEnd = platform::GetRealTime();

    YLOG_INFO("FILE", "Found '%d' files in '%s' took: '%f' seconds.", foundFiles.size(), sourceLocation.c_str(), timeEnd - timeStart);

    return foundFiles;
}


//-------------------------------------------------------------------------------------------------------------------------------
yaget::Strings yaget::io::file::GenerateConfigSearchPath(const std::string& name, bool addAppName, const args::Options* options) // KeyBindings, Configuration 
{
    yaget::Strings searches;
    std::string searchPath;
    std::string userFileName;
    std::string userFilePath;

    if (options)
    {
        std::string optionName = conv::ToLower(std::string(name + "_file"));
        userFileName = options->find<std::string>(optionName, "");
        userFileName = userFileName.empty() ? userFileName : name + "_" + userFileName;

        std::string optionFilePath = options->find<std::string>("configuration_path", "");
        userFilePath = util::ValidatePath(optionFilePath);
        if (!userFilePath.empty())
        {
            util::AddToEnvironment({ { "$(DataFolder)", { userFilePath, true } } });
        }
    }

    if (addAppName)
    {
        if (!userFileName.empty())
        {
            if (!userFilePath.empty())
            {
                searchPath = util::ExpendEnv(userFilePath + "/" + userFileName, "json");
                searches.push_back(searchPath);
            }

            AddSearchPath(searches, userFileName);
        }

        std::string defaultFileName = name + "_$(AppName)";

        if (!userFilePath.empty())
        {
            searchPath = util::ExpendEnv(userFilePath + "/" + defaultFileName, "json");
            searches.push_back(searchPath);

            searchPath = util::ExpendEnv(userFilePath + "/" + "$(AppName)", "json");
            searches.push_back(searchPath);
        }

        AddSearchPath(searches, defaultFileName);
        AddSearchPath(searches, "$(AppName)");
    }

    if (!userFilePath.empty())
    {
        searchPath = util::ExpendEnv(userFilePath + "/" + name, "json");
        searches.push_back(searchPath);

        searchPath = util::ExpendEnv(userFilePath + "/" + "$(AppName)", "json");
        searches.push_back(searchPath);
    }

    AddSearchPath(searches, name);
    AddSearchPath(searches,"$(AppName)");

    return searches;
}


//-------------------------------------------------------------------------------------------------------------------------------
std::string yaget::io::file::FindConfigFile(const std::string& name, bool addAppName, const args::Options* options)
{
    yaget::Strings searchPath = GenerateConfigSearchPath(name, addAppName, options);

    Strings sortedPath = searchPath;
    std::sort(std::rbegin(sortedPath), std::rend(sortedPath));

    for (const auto& it : searchPath)
    {
        if (io::file::IsFileExists(it))
        {
            return fs::canonical(it).generic_string();
        }
    }

    return "";
}


//-------------------------------------------------------------------------------------------------------------------------------
yaget::io::file::FileOpResult yaget::io::file::RemoveFile(const std::string& fileName)
{
    fs::path sourcePath = fs::path(util::ExpendEnv(fileName, nullptr));
    YLOG_INFO("FILE", "Removing file '%s'.", sourcePath.generic_string().c_str());

    std::error_code ec;
    bool result = fs::remove(sourcePath, ec);
    if (!result && ec)
    {
        std::string message = fmt::format("Remove file '{}' from disk failed with error: '{}: {}'.", sourcePath.generic_string(), ec.value(), ec.message());
        return { false, message };
    }

    return { true, "" };
}


//-------------------------------------------------------------------------------------------------------------------------------
yaget::io::file::FileOpResult yaget::io::file::SaveFile(const std::string& fileName, const io::Buffer& buffer)
{
    fs::path sourcePath = fs::path(util::ExpendEnv(fileName, nullptr));
    YLOG_INFO("FILE", "Saving file '%s'.", sourcePath.generic_string().c_str());

    if (sourcePath.has_parent_path() && !fs::is_directory(sourcePath.parent_path()))
    {
        const auto& [result, errorMessage] = AssureDirectories(sourcePath.generic_string());
        if (!result)
        {
            return { false, errorMessage };
        }
    }

    std::ofstream file(sourcePath.generic_string().c_str(), std::ios_base::binary);
    file.write(io::BufferPointer(buffer), io::BufferSize(buffer));
    if (!file.good())
    {
        std::string textError = platform::LastErrorMessage();
        std::string errorMessage = fmt::format("Failed saving file: '{}'. {}", sourcePath.generic_string(), textError);
        return { false, errorMessage };
    }

    return { true, "" };
}


//---------------------------------------------------------------------------------------------------------------------------------
bool yaget::io::file::IsFileExists(const std::string& fileName)
{
    fs::path sourcePath = fs::path(util::ExpendEnv(fileName, nullptr));
    return fs::exists(sourcePath);
}


//---------------------------------------------------------------------------------------------------------------------------------
yaget::io::file::FileOpResult yaget::io::file::SetFileAtrribute(const std::string& fileName, Attributes attribute)
{
    std::string sourcePath = fs::path(util::ExpendEnv(fileName, nullptr)).generic_string();
    YLOG_INFO("FILE", "Setting file attributes: '%d', '%s'.", attribute, sourcePath.c_str());

    DWORD attr = ::GetFileAttributes(sourcePath.c_str());
    if (attr != INVALID_FILE_ATTRIBUTES) 
    {
        switch (attribute)
        {
        case Attributes::ReadOnly:
            attr |= FILE_ATTRIBUTE_READONLY;
        	break;
        case Attributes::ReadWrite:
            attr &= ~FILE_ATTRIBUTE_READONLY;
            break;

        default:
            return { false, fmt::format("Unsupported file '{}' attribute '{}' to set.", sourcePath, static_cast<int>(attribute)) };
        }

        if (!::SetFileAttributes(sourcePath.c_str(), attr))
        {
            std::string textError = platform::LastErrorMessage();
            return { false, fmt::format("Did not set attribute '{}' on file '{}'. {}.", attr, sourcePath, textError) };
        }

        return { true, "" };
    }

    return { false, fmt::format("Invalid file '{}' to get attributes from.", sourcePath) };
}


//---------------------------------------------------------------------------------------------------------------------------------
bool yaget::io::file::IsFileAtrribute(const std::string& fileName, Attributes attribute)
{
    std::string sourcePath = fs::path(util::ExpendEnv(fileName, nullptr)).generic_string();
    DWORD attr = ::GetFileAttributes(sourcePath.c_str());
    if (attr != INVALID_FILE_ATTRIBUTES)
    {
        switch (attribute)
        {
        case Attributes::ReadOnly:
            return attr & FILE_ATTRIBUTE_READONLY;
            break;
        case Attributes::ReadWrite:
            return !(attr & FILE_ATTRIBUTE_READONLY);
            break;
        }
    }

    return false;
}


//---------------------------------------------------------------------------------------------------------------------------------
yaget::io::file::FileOpResult yaget::io::file::AssureDirectories(const std::string& pathName)
{
    std::string canonicalPath = fs::path(util::ExpendEnv(pathName, nullptr)).make_preferred().string();

    //YLOG_INFO("FILE", "Assuring directory path: '%s'.", fs::path(canonicalPath).parent_path().generic_string().c_str());

    if (!::MakeSureDirectoryPathExists(canonicalPath.c_str()))
    {
        std::string textError = platform::LastErrorMessage();
        return { false, fmt::format("Did not create full folder path for '{}'. {}.", fs::path(canonicalPath).parent_path().generic_string(), textError) };
    }

    return { true, "" };
}


//-------------------------------------------------------------------------------------------------------------------------------
void yaget::io::file::EnumerateDirectoryContent(std::vector<std::string>& foundFiles, const std::string& sourceLocation, Traverse traverse, std::function<bool(const std::string & fileName)> filter)
{
    WIN32_FIND_DATA fdFile{};
    HANDLE hFind = nullptr;

    std::string currentFilePath = sourceLocation + "\\*.*";

    if ((hFind = ::FindFirstFile(currentFilePath.c_str(), &fdFile)) == INVALID_HANDLE_VALUE)
    {
        return;
    }

    do
    {
        //Find first file will always return "."
        //    and ".." as the first two directories.
        if (strcmp(fdFile.cFileName, ".") != 0 && strcmp(fdFile.cFileName, "..") != 0)
        {
            //Build up our file path using the passed in
            //  [sourceLocation] and the file/foldername we just found:
            currentFilePath = sourceLocation + "\\" + fdFile.cFileName;

            //Is the entity a File or Folder?
            if (fdFile.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                if (traverse == Traverse::Deep)
                {
                    EnumerateDirectoryContent(foundFiles, currentFilePath, traverse, filter);
                }
            }
            else
            {
                if (filter(currentFilePath))
                {
                    foundFiles.push_back(currentFilePath);
                }
            }
        }
    } while (::FindNextFile(hFind, &fdFile)); //Find the next file.

    ::FindClose(hFind);
}
