///////////////////////////////////////////////////////////////////////
// ConfigHelper.h
//
//  Copyright 10/9/2005 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      This file provides some helper functions for reading Config files
//
//
//  #include "Config/ConfigHelper.h"
//
//////////////////////////////////////////////////////////////////////
//! \file

#ifndef CONFIG_CONFIG_HELPER_H
#define CONFIG_CONFIG_HELPER_H
#pragma once
#if 0
#include "Base.h"
#include "Math/Vector.h"
#include <wx/gdicmn.h>
#include <string>
#include <vector>

class wxConfigBase;

namespace eg
{
    namespace config
    {

        //! Install config file.
        void Install(const std::string& appName, const std::string& globalIni = "", const std::string& localIni = "", const std::string& vendorName = "");
        //! This allows us to provide string data to use as a config file data. It will delete existing config object
        //! and create new one based on string
        //! \param configData config data in a text form
        void Use(const std::string& configData);

        //! Called by framework on shutdown to cleanup any config files
        void Shutdown();

        //! Replace textData with macros if any
        //! configuration file can provide this section
        //! [Macros]
        //!    MY_GAME = "Yaget"
        //! and if textData is "../Assets/$(MY_GAME)/Textures/"
        //! it will become "../Assets/Yaget/Textures/" after calling this function
        //! \param textData [OUT] string to substitute macros if any
        //! \return true if there was substitution, otherwise false and string is not modified.
        bool ExpandMacros(std::string& textData);

        //@{
        //! Read Key as a Vector2. First function will use default configuration file.
        Vector2 ReadV2(const char *pKeyName, const Vector2& defaultValue = Vector2(0, 0));
        Vector2 ReadV2(const wxConfigBase *pConfigFile, const char *pKeyName, const Vector2& defaultValue = Vector2(0, 0));
        //@}

        //@{
        //! Read Key as a wxPoint. First function will use default configuration file.
        wxPoint ReadP2(const char *pKeyName, const wxPoint& defaultValue = wxPoint(0, 0));
        wxPoint ReadP2(const wxConfigBase *pConfigFile, const char *pKeyName, const wxPoint& defaultValue = wxPoint(0, 0));
        //@}

        //@{
        //! Read Key as a wxSize. First function will use default configuration file.
        wxSize ReadS2(const char *pKeyName, const wxSize& defaultValue = wxSize(0, 0));
        wxSize ReadS2(const wxConfigBase *pConfigFile, const char *pKeyName, const wxSize& defaultValue = wxSize(0, 0));
        //@}

        //@{
        //! Read Key as a wxRect. First function will use default configuration file.
        wxRect ReadRect(const char *pKeyName, const wxRect& defaultValue = wxRect(0, 0, 0, 0));
        wxRect ReadRect(const wxConfigBase *pConfigFile, const char *pKeyName, const wxRect& defaultValue = wxRect(0, 0, 0, 0));
        //@}

        //
        //@{
        //! This will read path folder and create fully qualified path, which can also include pathPrefix.
        //! First function will use default configuration file.
        //! It also supports $(RootFolder) macro in folder name, which will be replaced by specifying RootFolder key under same section
        //! as this queried one
        std::string ReadPath(const char *pKeyName, const std::string& pathPrefix = "");
        std::string ReadPath(const wxConfigBase *pConfigFile, const char *pKeyName, const std::string& pathPrefix = "");
        std::string ReadFullPath(const std::string& keyName);
        //@}

        //@{
        //! This will read string.
        //! First function will use default configuration file.
        std::string ReadString(const char *ppKeyName, const std::string& defaultValue = "");
        std::string ReadString(const wxConfigBase *pConfigFile, const char *pKeyName, const std::string& defaultValue = "");
        //@}

        //@{
        //! This will read array of string, which are separated by commas
        std::vector<std::string> ReadStringArray(const char *ppKeyName, const std::vector<std::string>& defaultValue =  std::vector<std::string>());
        std::vector<std::string> ReadStringArray(const wxConfigBase *pConfigFile, const char *pKeyName, const std::vector<std::string>& defaultValue =  std::vector<std::string>());


        //@{
        //! Read value as a bool. First function will use default configuration file
        bool ReadBool(const char *pKeyName, bool defaultValue = false);
        bool ReadBool(const wxConfigBase *pConfigFile, const char *pKeyName, bool defaultValue = false);
        //@}

        //@{
        //! Read value as a int32_t. First function will use default configuration file
        int32_t ReadInt(const char *pKeyName, int32_t defaultValue = 0);
        int32_t ReadInt(const wxConfigBase *pConfigFile, const char *pKeyName, int32_t defaultValue = 0);
        //@}

        //@{
        //! Read value as a uint64_t. First function will use default configuration file
        uint64_t ReadUInt64(const char *pKeyName, uint64_t defaultValue = 0);
        uint64_t ReadUInt64(const wxConfigBase *pConfigFile, const char *pKeyName, uint64_t defaultValue = 0);
        //@}

        //@{
        //! Read value as a float or double. First function will use default configuration file
        float ReadFloat(const char *pKeyName, float defaultValue = 0);
        float ReadFloat(const wxConfigBase *pConfigFile, const char *pKeyName, float defaultValue = 0);
        double ReadDouble(const char *pKeyName, double defaultValue = 0);
        double ReadDouble(const wxConfigBase *pConfigFile, const char *pKeyName, double defaultValue = 0);
        //@}

        //@{
        //! This will enumerate all of the keys in one group. First function will use default configuration file.
        bool EnumerateGroup(const std::string& groupName, std::vector<std::string>& keyList);
        bool EnumerateGroup(const wxConfigBase *pConfigFile, const std::string& groupName, std::vector<std::string>& keyList);
        //@}

        //@{
        //! This will enumerate all of the groups/section in one group. First function will use default configuration file.
        bool EnumerateSections(const std::string& groupName, std::vector<std::string>& sectionList);
        bool EnumerateSections(const wxConfigBase *pConfigFile, const std::string& groupName, std::vector<std::string>& sectionList);
        //@}


        //! Write Key as a Vector2. First function will use default configuration file.
        void WriteV2(const char *pKeyName, const Vector2& value);
        void WriteV2(wxConfigBase *pConfigFile, const char *pKeyName, const Vector2& value);
        //@}

        //@{
        //! Write Key as a wxPoint. First function will use default configuration file.
        void WriteP2(const char *pKeyName, const wxPoint& value);
        void WriteP2(wxConfigBase *pConfigFile, const char *pKeyName, const wxPoint& value);
        //@}

        //@{
        //! Write Key as a wxSize. First function will use default configuration file.
        void WriteS2(const char *pKeyName, const wxSize& value);
        void WriteS2(wxConfigBase *pConfigFile, const char *pKeyName, const wxSize& value);
        //@}

        //@{
        //! Write Key as a wxRect. First function will use default configuration file.
        void WriteRect(const char *pKeyName, const wxRect& value);
        void WriteRect(wxConfigBase *pConfigFile, const char *pKeyName, const wxRect& value);
        //@}

        //@{
        //! This will write path folder
        //! First function will use default configuration file.
        void WritePath(const char *pKeyName, const std::string& value);
        void WritePath(wxConfigBase *pConfigFile, const char *pKeyName, const std::string& value);
        //@}

        //@{
        //! This will write string.
        //! Write function will use default configuration file.
        void WriteString(const char *pKeyName, const std::string& value);
        void WriteString(wxConfigBase *pConfigFile, const char *pKeyName, const std::string& value);
        //@}

        //@{
        //! Write value as a bool. First function will use default configuration file
        void WriteBool(const char *pKeyName, bool value);
        void WriteBool(wxConfigBase *pConfigFile, const char *pKeyName, bool value);
        //@}

        //@{
        //! Write value as a int32_t. First function will use default configuration file
        void WriteInt(const char *pKeyName, int32_t value);
        void WriteInt(wxConfigBase *pConfigFile, const char *pKeyName, int32_t value);
        //@}

    } // namespace config
} // namespace eg
#endif // 0
#endif // CONFIG_CONFIG_HELPER_H

