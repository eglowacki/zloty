#include "App/AppUtilities.h"
#include "fmt/printf.h"
#include "Exception/Exception.h"
#include "Logger/YLog.h"
#include "StringHelpers.h"
#include "Platform/Support.h"
#include "Debugging/DevConfiguration.h"
#include "App/FileUtilities.h"
#include "HashUtilities.h"
#include "Platform/WindowsLean.h"

#include <Shlwapi.h>
#include <filesystem>
#include <fstream>
#include <regex>
#include <comdef.h>
#include <Dbghelp.h>
#include <shlobj.h>

namespace fs = std::filesystem;

namespace
{
    constexpr int MaxLogFileNameDigits = 4;
    //--------------------------------------------------------------------------------------------------
    std::string ResolveBrandName()
    {
        std::string result = "Yaget";
        if (const auto getBrandName = yaget::util::ResolveFunction<YagetFuncBrandName>(YAGET_GET_BRAND_FUNCTION_STRING))
        {
            result = getBrandName();
        }

        return result;
    }

    //--------------------------------------------------------------------------------------------------
    /// some_drive:/some_folders/my_executable.exe
    std::string GetApllicationPath()
    {
        const size_t kMaxPath = 512;
        char buf[kMaxPath] = {'\0'};
        ::GetModuleFileName(nullptr, buf, kMaxPath);
        return fs::path(buf).generic_string();
    }

    //--------------------------------------------------------------------------------------------------
    /// return application folder where executable resides, without trailing slash
    /// some_drive:/some_folders
    std::string ResolveAppFolder()
    {
        std::string appPath = GetApllicationPath();
        std::string appName = ::PathFindFileName(appPath.c_str());

        appPath = yaget::conv::ReplaceString(appPath, appName, "");
        if (!appPath.empty())
        {
            char c = appPath[appPath.size() - 1];
            if (c == '\\' || c == '/')
            {
                appPath.erase(appPath.size() - 1);
            }
        }

        return fs::path(appPath).generic_string();
    }

    //--------------------------------------------------------------------------------------------------
    /// Return executable name with extension
    std::string ResolveExecutableName()
    {
        std::string appPath = GetApllicationPath();
        appPath = PathFindFileName(appPath.c_str());
        return appPath;
    }

    //--------------------------------------------------------------------------------------------------
    /// return just the name of executable without extension
    std::string ResolveAppName()
    {
        std::string appPath = ResolveExecutableName();
        std::string extApp = PathFindExtension(appPath.c_str());
        appPath = yaget::conv::ReplaceString(appPath, extApp, "");
        return appPath;
    }

    //--------------------------------------------------------------------------------------------------
    /// return full file path name
    /// some_drive:/some_folders/my_executable
    std::string ResolveAppPathName()
    {
        fs::path appPath = ResolveAppFolder();
        fs::path appName = ResolveAppName();
        fs::path fullPath = appPath / appName;

        return fullPath.generic_string();
    }

    //--------------------------------------------------------------------------------------------------
    /// return $(AppFolder)/*/folder_name if marker exist up to chain in that folder, otherwise returns $(AppFolder)
    std::string ResolveConfigurationFolderName(const char* marker)
    {
        fs::path confFolder = ResolveAppFolder();
        bool bMoreFolders = true;
        const bool& hasRoot = confFolder.has_root_path();
        const auto& rootDrive = confFolder.root_path();
        do 
        {
            if (fs::is_directory(confFolder) && fs::exists(confFolder / marker))
            {
                confFolder = fs::canonical(confFolder);
                return confFolder.generic_string();
            }

            const bool& hasParent = confFolder.has_parent_path();
            const bool& rootNotSame = rootDrive != confFolder;

            bMoreFolders = hasParent && hasRoot && rootNotSame;

            confFolder = confFolder.parent_path();

        } while (bMoreFolders);

        return ResolveAppFolder();
    }

    //--------------------------------------------------------------------------------------------------
    /// Return platform dependent path to temporary folder
    std::string ResolveTempPathName()
    {
        const auto result = fs::temp_directory_path() / fs::path(ResolveBrandName()) / fs::path(ResolveAppName());
        return result.generic_string();
    }

    //--------------------------------------------------------------------------------------------------
    /// used with util::EnvironmentList EnvList to fill in values for aliasses
    std::string ResolveBuildConfiguration()
    {
#ifdef YAGET_DEBUG
        return "Debug";
#elif YAGET_RELEASE
        return "Release";
#else
    #error "Missing build configuration for Alias $(BuildConfiguration) Names resolution"
#endif
    }

    std::string GetKnowFolder(REFKNOWNFOLDERID refId)
    {
        std::string result;
        PWSTR path = nullptr;
        HRESULT hr = ::SHGetKnownFolderPath(refId, KF_FLAG_CREATE, nullptr, &path);
        assert(SUCCEEDED(hr));
        if (SUCCEEDED(hr) && path)
        {
            result = yaget::conv::wide_to_utf8(path);
            CoTaskMemFree(path);
        }

        return fs::path(result).generic_string();
    }


    std::string ResolveGlobalDataFolder()
    {
        if (const fs::path folderPath = yaget::util::ResolveYagetMarker(yaget::dev::DataMarker); !folderPath.empty())
        {
            return folderPath.generic_string();
            //return (fs::path(folderPath) / fs::path(ResolveBrandName()) / fs::path(ResolveAppName())).generic_string();
        }

        return ResolveAppFolder();
    }

    //--------------------------------------------------------------------------------------------------
    std::string ResolveUserDataFolder()
    {
        if (const fs::path folderPath = yaget::util::ResolveYagetMarker(yaget::dev::UserDataMarker); !folderPath.empty())
        {
            return folderPath.generic_string();
            //return (fs::path(folderPath) / fs::path(ResolveBrandName()) / fs::path(ResolveAppName())).generic_string();
        }

        fs::path result = GetKnowFolder(FOLDERID_SavedGames);
        result /= fs::path(ResolveBrandName()) / fs::path(ResolveAppName());

        return result.generic_string();
    }

    //--------------------------------------------------------------------------------------------------
    std::string ResolveAppDataFolder()
    {
        if (const  fs::path folderPath = yaget::util::ResolveYagetMarker(yaget::dev::AppDataMarker); !folderPath.empty())
        {
            return folderPath.generic_string();
            //return (fs::path(folderPath) / fs::path(ResolveBrandName()) / fs::path(ResolveAppName())).generic_string();
        }

        fs::path result = GetKnowFolder(FOLDERID_LocalAppData);
        result /= fs::path(ResolveBrandName()) / fs::path(ResolveAppName());

        return result.generic_string();
    }

    //--------------------------------------------------------------------------------------------------
    std::string ResolveScreenshotsFolder()
    {
        fs::path result = GetKnowFolder(FOLDERID_Screenshots);
        result /= fs::path(ResolveBrandName()) / fs::path(ResolveAppName());

        return result.generic_string();
    }

    //--------------------------------------------------------------------------------------------------
    yaget::util::EnvironmentList& EnvList()
    {
        static yaget::util::EnvironmentList envList = {
            { "$(BuildConfiguration)", {ResolveBuildConfiguration(), true} },
            { "$(AppFolder)", {ResolveAppFolder(), true} },
            { "$(AppName)", {ResolveAppName(), true} },
            { "$(ExecutableName)", {ResolveExecutableName(), true} },
            { "$(AppPathName)", {ResolveAppPathName(), true} },
            { "$(ConfigurationFolder)",{ "$(AppFolder)", true } },
            { "$(DataFolder)",{ ResolveGlobalDataFolder(), true } },
            { "$(Temp)",{ ResolveTempPathName(), false } },
            { "$(LogFolder)",{ "$(UserDataFolder)/Logs", false } },
            { "$(SaveDataFolder)",{ "$(UserDataFolder)/Saves", false } },
            { "$(UserDataFolder)",{ ResolveUserDataFolder(), true } },
            { "$(AppDataFolder)",{ ResolveAppDataFolder(), true } },
            { "$(ScreenshotsFolder)",{ "$(UserDataFolder)/Screenshots", false } },
            { "$(Brand)",{ ResolveBrandName(), true } },
            { "$(AssetsFolder)",{ "$(AppDataFolder)/Assets", false } },
            { "$(DatabaseFolder)",{ "$(AppDataFolder)/Database", false } }
        };

        return envList;
    }

    //--------------------------------------------------------------------------------------------------
    // expend env aliases
    void ExpendAll(std::string& variable)
    {
        auto& env = EnvList();
        for (auto&& it : env)
        {
            yaget::conv::ReplaceAll(variable, it.first, it.second.Value);
        }
    }

    std::string ExpendAll(const std::string& variable)
    {
        std::string result = variable;
        ExpendAll(result);
        return result;
    }

    bool EraseAlias(std::string& expendedValue)
    {
        bool result = false;
        auto startIndex = expendedValue.find_first_of("$(");
        if (startIndex != std::string::npos)
        {
            auto endIndex = expendedValue.find_first_of(")", startIndex);
            if (endIndex != std::string::npos)
            {
                expendedValue.erase(startIndex, (endIndex - startIndex) + 1);
                result = true;
            }
            if (!expendedValue.empty() && ((*expendedValue.begin()) == '/' || (*expendedValue.begin()) == '\\'))
            {
                expendedValue = expendedValue.erase(0, 1);
                result = true;
            }
        }

        return result;
    }

    std::string to_string(bool value)
    {
        return value ? "True" : "False";
    }

    // return number based on "-0000" format
    int ExtractNumber(const std::string& name, int maxNameDigits)
    {
        const std::string fileName = fs::path(name).stem().generic_string();
        if (fileName.size() > maxNameDigits)
        {
            std::string_view v{ fileName };
            v.remove_prefix(v.size() - maxNameDigits);

            const std::string value(v.begin(), v.end());
            return yaget::conv::AtoN<int>(value.c_str());
        }

        return 0;
    }

    int GetNumDigits(int value)
    {
        int length = 1;
        while (value /= 10)
        {
            length++;
        }

        return length;
    }

} // namespace


//--------------------------------------------------------------------------------------------------
yaget::Guid yaget::util::ApplicationRuntimeId()
{
    static Guid appGuid = NewGuid();
    return appGuid;
}


//--------------------------------------------------------------------------------------------------
void yaget::util::AddToEnvironment(const yaget::util::EnvironmentList& envList)
{
    auto& env = EnvList();

    for (const auto& it : envList)
    {
        if (auto alias = env.find(it.first); alias != std::end(env))
        {
            // alias already exist, let's see if it's read only
            if (alias->second.ReadOnly)
            {
                // We only want to generate warning if requested alias name is not the same as current
                // otherwise we silently ignore the same value, since there is no change
                YLOG_CWARNING("UTIL", 
                    ExpendAll(it.second.Value) == alias->second.Value,
                    "Environment Alias: '%s' is marked as Read Only, can not be set from configuration. Current Value: '%s', Requested Value: '%s' [%s].", 
                    alias->first.c_str(), alias->second.Value.c_str(), ExpendAll(it.second.Value).c_str(), it.second.Value.c_str());
                continue;
            }
        }

        std::string newAliasValue = ExpendAll(it.second.Value);
        env[it.first] = { newAliasValue, it.second.ReadOnly };
    }

    // re-run expend again on all entries, since we do not enforce order of declaration of aliases for dependencies. 
    // TODO: How do we ensure that there is no circular dependencies?
    for (auto& it : env)
    {
        ExpendAll(it.second.Value);
    }
}


//---------------------------------------------------------------------------------------------------------------------------------
bool yaget::util::IsEnvironment(const std::string& variable)
{
    const auto& env = EnvList();

    return env.find(variable) != std::end(env);
}


//---------------------------------------------------------------------------------------------------------------------------------
const yaget::util::EnvironmentList& yaget::util::GetCurrentEnvironment()
{
    return EnvList();
}


//---------------------------------------------------------------------------------------------------------------------------------
std::string yaget::util::CollapseEnv(const std::string& variable, const std::string& alias)
{
    std::string collapsedValue = variable;

    const std::string aliasValue = conv::ToLower(util::ExpendEnv(alias, nullptr));
    std::size_t index = conv::ToLower(collapsedValue).find(aliasValue);
    if (index != std::string::npos)
    {
        // we found some matching path, replace that with alias
        collapsedValue.replace(index, aliasValue.length(), alias);
    }

    conv::ReplaceAll(collapsedValue, "\\", "/");
    //fs::path fsPath(collapsedValue);
    //collapsedValue = fsPath.generic_string();
    return collapsedValue;
}


//---------------------------------------------------------------------------------------------------------------------------------
std::string yaget::util::ExpendEnv(const std::string& variable, const char* extension)
{
    std::string expendedValue = ExpendAll(variable);

    fs::path canonicalPath(expendedValue);
    if (canonicalPath.has_root_path())
    {
        canonicalPath = fs::weakly_canonical(canonicalPath);
        expendedValue = canonicalPath.generic_string();
    }

    if (extension)
    {
        canonicalPath.replace_extension(fs::path(extension));
        expendedValue = canonicalPath.generic_string();
    }

    while (EraseAlias(expendedValue))
    {
    }

    return expendedValue;
}


//---------------------------------------------------------------------------------------------------------------------------------
std::string yaget::util::DisplayCurrentConfiguration(args::Options* options)
{
    const std::string& configFile = io::file::FindConfigFile("Configuration", true, options);

    // Show loaded config file
    // show all search path for config file and marked used one with *
    std::string message = "=== Configuration Search Path:";
    message += "\nFound and Used:\n   " + (!configFile.empty() ? configFile : "*NO CONFIGURATION*") + ".\nSearch Order:";

    Strings searches = io::file::GenerateConfigSearchPath("Configuration", true, options);
    for (const auto& searched : searches)
    {
        std::string linkable;
        if (searched == configFile)
        {
            linkable = "file:///";
        }

        message += fmt::format("\n   {}{}", linkable, searched);
     }

    // Show all aliases and it's corresponding path
    message += "\n=== Environment Aliases:";
    const util::EnvironmentList& envList = util::GetCurrentEnvironment();

    auto it = std::max_element(envList.begin(), envList.end(), [](const util::EnvironmentList::value_type& a, const util::EnvironmentList::value_type& b)
    {
        return a.first.length() < b.first.length();
    });
    std::size_t maxLen = it != envList.end() ? it->first.length() : 0;

    for (const auto& env : envList)
    {
        std::size_t numPadding = maxLen ? maxLen - env.first.length() : 0;
        std::string spacing(numPadding, ' ');
        std::string expendedAlias = util::ExpendEnv(env.first, nullptr);

        const bool isLinkable = fs::is_directory(expendedAlias) || fs::is_regular_file(expendedAlias);
        const char* filePrefix = isLinkable ? "file:///" : "";

        message += fmt::format("\n   Alias: '{}'{} = {}{}{}", env.first, spacing, (env.second.ReadOnly ? "R- " : "RW "), filePrefix, expendedAlias);
    }

    const dev::Configuration& configuration = dev::CurrentConfiguration();

    // Show VTS sections data and expend search path for each section
    message += "\n=== VTS Sections:";
    for (const auto& section : configuration.mInit.mVTSConfig)
    {
        message += fmt::format("\n  Section: '{}' [{}] -{}- ReadOnly: {}, Recursive: {}", section.Name, conv::Combine(section.Filters, ", "), section.Converters, to_string(section.ReadOnly), to_string(section.Recursive));
        for (const auto& pathName : section.Path)
        {
            std::string potentialPath = util::ExpendEnv(pathName, nullptr);
            message += fmt::format("\n     file:///{}", potentialPath);
        }
    }

    const auto& logging = configuration.mDebug.mLogging;
    message += "\n=== Log:";
    message += fmt::format("\n   Level: '{}'", logging.Level);
    message += fmt::format("\n   Filters: '{}'", conv::Combine(logging.Filters, ", "));

    std::vector<std::string> keys;
    std::transform(std::begin(logging.Outputs), std::end(logging.Outputs), std::back_inserter(keys), [](auto const& pair) { return pair.first; });
    message += fmt::format("\n   Outputs: '{}'", conv::Combine(keys, ", "));

    return message;
}


//---------------------------------------------------------------------------------------------------------------------------------
std::string yaget::util::ValidatePath(const std::string& potentialPath)
{
    std::string result;

    if (!potentialPath.empty())
    {
        enum class PassProcess { Raw, AppFolder, MarkerFolder, Done };
        PassProcess passProcess = PassProcess::Raw;
        fs::path filePath{ potentialPath };

        do 
        {
            std::error_code ec;
            filePath = fs::canonical(filePath, ec);
            if (ec)
            {
                if (passProcess == PassProcess::Raw)
                {
                    passProcess = PassProcess::AppFolder;
                    std::string appFolder = util::ExpendEnv("$(AppFolder)", nullptr);
                    filePath = fs::path(appFolder) / fs::path(potentialPath);
                }
                else if (passProcess == PassProcess::AppFolder)
                {
                    passProcess = PassProcess::MarkerFolder;
                    std::string appFolder = util::ExpendEnv("$(ConfigurationFolder)", nullptr);
                    filePath = fs::path(appFolder) / fs::path(potentialPath);
                }
                else if (passProcess == PassProcess::MarkerFolder)
                {
                    passProcess = PassProcess::Done;
                }
                
                ec.clear();
            }
            else
            {
                result = filePath.generic_string();
                passProcess = PassProcess::Done;
            }

        } while (passProcess != PassProcess::Done);
    }

    return result;
}


//---------------------------------------------------------------------------------------------------------------------------------
std::string yaget::util::ResolveYagetMarker(const char* markerName)
{
#ifndef YAGET_SHIPPING
    const auto& appFolder = ResolveAppFolder();

    const auto& userFolder = ResolveConfigurationFolderName(markerName);
    if (userFolder != appFolder)
    {
        // we have found the dev yaget marker file to point us which
        // folder to set as User's, otherwise it returns whatever the
        // underlying platform returns.
        const fs::path markerPath = fs::path(userFolder) / fs::path(markerName);
        std::ifstream markerFile(markerPath.generic_string());
        if (markerFile.is_open())
        {
            std::string markerText;
            std::getline(markerFile, markerText);

            if (!markerText.empty() && !markerText.starts_with(';'))
            {
                const auto devUserFolder = fs::path(userFolder) / fs::path(markerText);
                if (fs::is_directory(devUserFolder))
                {
                    return devUserFolder.generic_string();
                }
            }
        }

        return userFolder;
    }
#endif // YAGET_SHIPPING

    return "";
}


//---------------------------------------------------------------------------------------------------------------------------------
bool yaget::util::FileCycler(const std::string& filePath, int maxFiles /*= 10*/)
{
    const fs::path logFileName(filePath);

    const std::string folderName = logFileName.has_parent_path() ? logFileName.parent_path().generic_string() : "$(LogFolder)";
    const std::string fileName = logFileName.stem().generic_string();
    const std::string extension = logFileName.has_extension() ? logFileName.extension().generic_string() : "log";

    return FileCycler(folderName, fileName, extension, maxFiles);
}


//---------------------------------------------------------------------------------------------------------------------------------
bool yaget::util::FileCycler(const std::string& folder, const std::string& fileName, const std::string& extension, int maxFiles /*= 10*/)
{
    using namespace yaget;

    fs::path fp = fs::path(folder) / fs::path(fileName);
    fp.replace_extension(extension);
    const std::string filePathName = fs::path(ExpendEnv(fp.generic_string(), nullptr)).generic_string();

    const int maxNameDigits = MaxLogFileNameDigits;

    const std::string digitFilter = fmt::format("-{:?<{}}", "", maxNameDigits);
    if (io::file::IsFileExists(filePathName))
    {
        int lastFileIndex = 0;
        fs::path filterFile = "*" + fileName + digitFilter;
        filterFile.replace_extension(extension);

        const std::string folderNameText = fs::path(ExpendEnv(folder, nullptr)).generic_string();
        const std::string filterText = fs::path(ExpendEnv(filterFile.generic_string(), nullptr)).generic_string();

        const auto logNames = io::file::GetFileNames(folderNameText, false, filterText);
        for (const auto& name : logNames)
        {
            int fileIndex = ExtractNumber(name, maxNameDigits);
            lastFileIndex = std::max(lastFileIndex, fileIndex);
        }

        auto leftNames = io::file::GetFileNames(folderNameText, false, filterText);
        std::sort(leftNames.begin(), leftNames.end(), [](const std::string& elem1, const std::string& elem2)
        {
            return ExtractNumber(elem1, maxNameDigits) < ExtractNumber(elem2, maxNameDigits);
        });

        if (GetNumDigits(lastFileIndex + 1) > maxNameDigits)
        {
            std::string partialName = fs::path(fs::path(folder) / fs::path(fileName)).generic_string();
            lastFileIndex = 0;

            for (const auto& name : leftNames)
            {
                const std::string newName = fs::path(ExpendEnv(partialName + fmt::format("-{:0{}}", ++lastFileIndex, maxNameDigits), extension.c_str())).generic_string();
                const auto& [result, errorMessage] = io::file::RenameFile(name, newName);
            }
        }

        std::string partialName = fs::path(fs::path(folder) / fs::path(fileName)).generic_string();
        partialName += fmt::format("-{:0{}}", lastFileIndex + 1, maxNameDigits);

        const std::string newName = fs::path(ExpendEnv(partialName, extension.c_str())).generic_string();
        const auto& [result, errorMessage] = io::file::RenameFile(filePathName, newName);
        leftNames.push_back(newName);

        if (leftNames.size() > maxFiles)
        {
            io::file::RemoveFiles({ leftNames.rbegin() + maxFiles, leftNames.rend() });
        }

        return result;
    }

    return true;
}


//---------------------------------------------------------------------------------------------------------------------------------
bool yaget::util::IsExtension(const std::string& name, const std::string& extension)
{
    std::string fileName = conv::ToLower(name);
    std::string fileExtension = conv::ToLower(extension);
    return fs::path(fileName).extension() == fileExtension;
}


//---------------------------------------------------------------------------------------------------------------------------------
#if defined(_WIN32)
#include <commdlg.h>
#include <comdef.h>
std::vector<std::string> yaget::util::ui::SelectOpenFileNames(const char* filter, const char* dialogTitle, bool bMultiSelect)
{
    fs::path currentPath = fs::current_path();
    std::vector<std::string> selectedFiles;

    const char *Title = dialogTitle ? dialogTitle : "Open File...";
    const size_t kResultBufferSize = MAX_PATH * 10;
    char szFileName[kResultBufferSize] = { 0 };
    char szFileTitle[MAX_PATH] = { 0 };

    OPENFILENAME ofn;
    memset(&ofn, 0, sizeof(ofn));
    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = GetFocus();
    ofn.lpstrFilter = filter;
    ofn.lpstrCustomFilter = nullptr;
    ofn.nMaxCustFilter = 0;
    ofn.nFilterIndex = 0;
    ofn.lpstrFile = szFileName;
    ofn.nMaxFile = kResultBufferSize;
    ofn.lpstrInitialDir = "."; // Initial directory.
    ofn.lpstrFileTitle = szFileTitle;
    ofn.nMaxFileTitle = MAX_PATH;
    ofn.lpstrTitle = Title;
    //ofn.lpstrDefExt = default_extension;

    ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | (bMultiSelect ? (OFN_ALLOWMULTISELECT | OFN_EXPLORER) : 0);

    if (::GetOpenFileName((LPOPENFILENAME)&ofn))
    {
        ::SetCurrentDirectory(currentPath.string().c_str());

        // check to see if user selected multi files or just singe one
        // by checking the first bit if it's only a folder or points to a file
        if (fs::is_directory(fs::path(szFileName)))
        {
            // pass over the initial file path and set 'being' and 'end'
            const char* beginBuffer = szFileName + std::strlen(szFileName) + 1;

            // loop to add found file names to selected files list
            while (*beginBuffer != '\0')
            {
                const char* endBuffer = beginBuffer;
                // find next file name
                while (*endBuffer != '\0')
                {
                    endBuffer++;
                }

                // now we construct full file path based on searched file name and szFileName as our path prefix
                std::string selectedBuffer(beginBuffer, endBuffer);
                std::string selected(szFileName);
                fs::path filePath = fs::path(selected) / fs::path(selectedBuffer);
                selectedFiles.push_back(filePath.string());

                beginBuffer = endBuffer + 1;
            }
        }
        else
        {
            selectedFiles.emplace_back(szFileName);
        }
    }
    else
    {
        // Failed or canceled
        DWORD errorCode = ::CommDlgExtendedError(); errorCode;
        fs::path currentPathA = fs::current_path();
        SetCurrentDirectory(currentPath.string().c_str());
    }

    return selectedFiles;
}


//---------------------------------------------------------------------------------------------------------------------------------
std::string yaget::util::SelectSaveFileName(const char* filter, const char* dialogTitle)
{
    const char *Title = dialogTitle ? dialogTitle : "Save Asset As";

    //char szFileName[MAX_PATH] = "";
    char szFileName[MAX_PATH] = { 0 };
    char szFileTitle[MAX_PATH] = { 0 };

    OPENFILENAME ofn;
    memset(&ofn, 0, sizeof(ofn));

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = GetFocus();
    ofn.lpstrFilter = filter;
    ofn.lpstrCustomFilter = nullptr;
    ofn.nMaxCustFilter = 0;
    ofn.nFilterIndex = 0;
    ofn.lpstrFile = szFileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrInitialDir = "."; // Initial directory.
    ofn.lpstrFileTitle = szFileTitle;
    ofn.nMaxFileTitle = MAX_PATH;
    ofn.lpstrTitle = Title;
    ofn.Flags = OFN_EXPLORER | OFN_HIDEREADONLY;
    //ofn.lpstrDefExt = (LPCWSTR)L"txt";

    if (!GetSaveFileName(&ofn))
    {
        return ""; // Failed or canceled
    }
    else
    {
        return std::string(szFileName);
    }
}


//---------------------------------------------------------------------------------------------------------------------------------
void yaget::util::DisplayDialog(const char* title, const char* message)
{
    // NOTE: Do we need to have a way to turn any visual dialog off, for automated testing, headless runs, etc.
    if (!dev::CurrentConfiguration().mDebug.mFlags.SuppressUI)
    {
        ::MessageBox(nullptr, message, title, MB_ICONERROR | MB_OK | MB_SYSTEMMODAL);
    }
}

#else
    #error  "Need to implement SelectOpenFileName, SelectSaveFileName, DisplayDialog, ThrowOnError for your platform."
#endif // _WIN32


//---------------------------------------------------------------------------------------------------------------------------------
void yaget::util::DefaultOptions(args::Options& options)
{
    
    options.add_options()
        ("options_view", "Display current option values.")
        ("generate_config", "Display default configuration values as json (can be used as starting point for a config file).")
        ("v,vsync_off", "Controls monitor vsync on or off. Not supported in window mode.")
        ("f,log_filter", "Filter out specific log tags.", args::value<std::vector<std::string>>())
        ("log_filter_clear", "Clear all log filter (show it all)") 
        ("o,log_output", "Which log outputs to attach (ylog::OutputFile, ylog::OutputConsole, ylog::OutputDebug, imgui::OutputConsole).", args::value<std::vector<std::string>>())
        ("l,log_level", "Log Level to show (DBUG, INFO, NOTE, WARN, CRIT).", args::value<std::string>())
        ("res_x", "Resolution x (width) (default 1366).", args::value<int>())
        ("res_y", "Resolution y (height) (default 768).", args::value<int>())
        ("full_screen", "Full screen with requested resolution, or if res_x/y not set, use current desktop size.")
        ("h,help", "Show command line options.")
        ("configuration_path", "Relative or absolute path to Configuration file.", args::value<std::string>())
        ("configuration_file", "Custom name to use for configuration file, Configuration_<custom_name>", args::value<std::string>())
        ("keybindings_file", "Relative or absolute path to Key Bindings file.", args::value<std::string>())
        ("logic_tick", "Game Logic thread tick update (hz) (default 60)", args::value<uint32_t>())
        ("vts_fix", "Fix VTS errors.")
        ("log_write_tags", "Write out file to $(LogFolder) of all active log tags.")
        ("config_value", "Override individual configuration values --config_value = Debug.Metrics.TraceOn=false (no spaces around =)", args::value<std::vector<std::string>>())
        ("software_render", "Force software renderer")
        ("gpu_traceback", "Activate GPU crash dump")
        ;
}


//---------------------------------------------------------------------------------------------------------------------------------
void* yaget::util::ResolveFunction(const char* name)
{
    if (HMODULE handle = ::GetModuleHandle(nullptr))
    {
        if (const auto resolvedFunction = (void *)::GetProcAddress(handle, name))
        {
            return resolvedFunction;
        }
    }

    return nullptr;
}