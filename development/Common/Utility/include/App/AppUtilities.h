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

#include "Base.h"
#include "Streams/Guid.h"
#include <string>
#include <map>
#include <vector>


namespace yaget
{
    namespace util
    {
        //! Unique application id, persistent only for the duration of application execution
        Guid ApplicationId();

        //! built in environment variables available at the start (examples for vts_test.exe project)
        //!  $(BuildConfiguration)  = Build configuration:                                                  Debug, Release, Shipping (and possibly other variations like Performance)
        //!  $(AppFolder)           = Application folder where executable resides, without trailing slash:  C:/Development/yaget/branch/version_0_2/bin/vts_test/x64.Debug
        //!  $(AppName)             = Executable name without extension:                                    vts_test
        //!  $(ExecutableName)      = Executable name with extension                                        vts_test.exe
        //!  $(AppPathName)         = Full file path name without extension                                 C:/Development/yaget/branch/version_0_2/bin/vts_test/x64.Debug/vts_test
        //!  $(ConfigurationFolder) = Defaults to $(AppFolder), but searches up for yaget::BinMarker:       C:/Development/yaget/branch/version_0_2/bin
        //!  $(RootFolder)          = Defaults to $(AppFolder), but searches up for yaget::RootMarker. It points to global data, modules, etc
        //!  $(Temp)                = System temporary folder                                               C:/Users/me_user/AppData/Local/Temp/vts_test
        //! Newest additions:
        //!  $(LogFolder)           = Location of all log files.                                            $(UserDataFolder)/Logs
        //!  $(SaveDataFolder)      = Location of saved game data.                                          $(UserDataFolder)/Saves
        //!  $(UserDataFolder)      = Location of saved user data.                                          FOLDERID_SavedGames   [%USERPROFILE%\Saved Games\$(AppName)]
        //!  $(AppDataFolder)       = Location of saved application data                                    FOLDERID_LocalAppData [%LOCALAPPDATA%\$(AppName) (%USERPROFILE%\AppData\Local\$(AppName))]
        //!  $(ScreenshotFolder)    = Location of saved screenshots                                         FOLDERID_Screenshots [%USERPROFILE%\Pictures\Screenshots\$(AppName)]

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
        std::string ValidatePath(const std::string potentialPath);

        std::string SelectSaveFileName(const char* filter, const char* dialogTitle);
        void DisplayDialog(const char* title, const char* message);

        void Throw(const char* tag, const std::string& message, const char* file = nullptr, unsigned line = 0, const char* functionName = nullptr);
        void ThrowOnError(bool resultValid, const std::string& message, const char* file = nullptr, unsigned line = 0, const char* functionName = nullptr);
        void ThrowOnError(long hr, const std::string& message, const char* file = nullptr, unsigned line = 0, const char* functionName = nullptr);
        void ThrowOnResult(const char* tag, bool result, const std::string& message, const char* file = nullptr, unsigned line = 0, const char* functionName = nullptr);

        // fill in options with default engine options, should be called first, before system::Initialized 
        void DefaultOptions(args::Options& options);

        // Return formated string for current configuration, like:
        // which configuration file was used
        // Search path for configuration file
        // Alias expansion
        // vts section name, filters, converter
        std::string DisplayCurrentConfiguration(args::Options* options);


        namespace ui
        {
            std::vector<std::string> SelectOpenFileNames(const char* filter, const char* dialogTitle, bool bMultiSelect);

        } // namespace ui
    } // namespace util
} // namespace yaget

#define YAGET_UTIL_THROW_ON_RROR(resultValid, message) yaget::util::ThrowOnError(resultValid, message, __FILE__, __LINE__, __FUNCTION__)
#define YAGET_UTIL_THROW_ASSERT(tag, result, message) yaget::util::ThrowOnResult(tag, result, message, __FILE__, __LINE__, __FUNCTION__)
#define YAGET_UTIL_THROW(tag, message) yaget::util::Throw(tag, message, __FILE__, __LINE__, __FUNCTION__)
