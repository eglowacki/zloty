/////////////////////////////////////////////////////////////////////////
// AppUtilities.h
//
//  Copyright 4/11/2009 Edgar Glowacki.
//
// NOTES:
//      Common functions and global utilities
//
//
// #include "App/AppUtilities.h"
//
/////////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "YagetCore.h"
#include "Streams/Guid.h"
#include <string>
#include <map>
#include <vector>


namespace yaget::util
{
    //! Unique application id, persistent only for the duration of application execution
    Guid ApplicationRuntimeId();

    //! built in environment variables available at the start (examples for vts_test.exe project)
    //!  $(BuildConfiguration)  = Build configuration:                                                  Debug, Release, Shipping (and possibly other variations like Performance)
    //!  $(AppFolder)           = Application folder where executable resides, without trailing slash:  C:/Development/yaget/branch/version_0_2/bin/vts_test/x64.Debug
    //!  $(AppName)             = Executable name without extension:                                    vts_test
    //!  $(ExecutableName)      = Executable name with extension                                        vts_test.exe
    //!  $(AppPathName)         = Full file path name without extension                                 C:/Development/yaget/branch/version_0_2/bin/vts_test/x64.Debug/vts_test
    //!  $(Temp)                = System temporary folder                                               C:/Users/me_user/AppData/Local/Temp/$(Brand)/$(AppName)
    //!
    //!  $(DataFolder)          = Defaults to $(AppDataFolder), but searches up from $(AppFolder) for yaget::DataMarker. It points to data folder which contains assets, files, etc needed by application.
    //!  $(ConfigurationFolder) = Defaults to $(AppFolder)
    //!
    //! Newest additions:
    //!     %USERPROFILE% expends to  'C:/Users/edgar'
    //!     %LOCALAPPDATA% expends to 'c:/Users/edgar/AppData/Local'
    //!
    //!  $(UserDataFolder)      = Location of saved user data.                                          %USERPROFILE%/Saved Games/$(Brand)/$(AppName) : Windows Id: FOLDERID_SavedGames
    //!  $(AppDataFolder)       = Location of saved application data                                    %LOCALAPPDATA%/$(Brand)/$(AppName) or %USERPROFILE%/AppData/Local/$(Brand)/$(AppName): Windows Id: FOLDERID_LocalAppData
    //!  $(ScreenshotsFolder)   = Location of saved screenshots                                         $(UserDataFolder)/Screenshots   (old folder: %USERPROFILE%/Pictures/Screenshots/$(Brand)/$(AppName) : Windows Id: FOLDERID_Screenshots)
    //!  $(LogFolder)           = Location of all log files.                                            $(UserDataFolder)/Logs
    //!  $(SaveDataFolder)      = Location of saved game data.                                          $(UserDataFolder)/Saves
    //!  $(Brand)               = Name of company or brand used as prefix to application specific folders (default: Yaget). There is macro YAGET_BRAND_NAME_F to customize brand name per project.
    //!
    //!  $(AssetsFolder)        = Location to assets folder for application                             "$(AppDataFolder)/Assets"       Both of these aliases default under $(AppDataFolder),
    //!  $(DatabaseFolder)      = Location to database folder for application                           "$(AppDataFolder)/Database"     but since it's not visible by default, during development
    //!                                                                                                                                 it can be redirected to $(UserDataFolder)/... for easy of access

    //! environment variable storage
    struct EnvAlias { std::string Value; bool ReadOnly = false; };
    using EnvironmentList = std::map<std::string, EnvAlias>;
    void AddToEnvironment(const EnvironmentList& envList);
    bool IsEnvironment(const std::string& variable);

    //! Expends env variables in string
    std::string ExpendEnv(const std::string& variable, const char* extension);
    std::string CollapseEnv(const std::string& variable, const std::string& alias);

    const EnvironmentList& GetCurrentEnvironment();

    bool IsExtension(const std::string& name, const std::string& extension);
    std::string ValidatePath(const std::string& potentialPath);

    // It will return valid folder name if markerName file exist in or at above directories.
    // That file can contain text consist of 'subfolder path'.
    // If combination of directory where markerFile resides and 'subfolder path'
    // is valid text and that folder exist at the location, return that full path.
    // Otherwise return folder name where markerFile is located.
    // If no fileMarker exists, return empty string.
    std::string ResolveYagetMarker(const char* markerName);

    // Check for if folder/fileName.extension exist
    // rename to folder/fileName-0000.extension
    // where 0000 will be replace with hightest number from all files fallowing
    // pattern: folder/fileName-????.extension
    // maxFiles will cap number of renamed files. It will leave hightest numbered files and delete the smaller numbered files
    bool FileCycler(const std::string& folder, const std::string& fileName, const std::string& extension, int maxFiles = 10);
    // helper function that splits filePath into separate components (folder, file, ext), it just calls function above
    bool FileCycler(const std::string& filePath, int maxFiles = 10);

    std::string SelectSaveFileName(const char* filter, const char* dialogTitle);
    void DisplayDialog(const char* title, const char* message);

    // fill in options with default engine options, should be called first, before system::Initialized 
    void DefaultOptions(args::Options& options);

    // Return formatted string for current configuration, like:
    // which configuration file was used
    // Search path for configuration file
    // Alias expansion
    // vts section name, filters, converter
    std::string DisplayCurrentConfiguration(args::Options* options);

    void* ResolveFunction(const char* name);

    template<typename T>
    T ResolveFunction(const char* name)
    {
        using Function = T;
        return (Function)ResolveFunction(name);
    }


    namespace ui
    {
        std::vector<std::string> SelectOpenFileNames(const char* filter, const char* dialogTitle, bool bMultiSelect);

    } // namespace ui
} // namespace yaget::util
