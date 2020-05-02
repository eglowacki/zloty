#if 0
#include "Config/ConfigHelper.h"
#include "Registrate.h"
#include "StringHelper.h"
#include <wx/fileconf.h>
#include <wx/stdpaths.h>
#include <wx/msw/regconf.h>
#include <wx/filename.h>
#include <wx/tokenzr.h>
#include <wx/sstream.h>
#include <wx/regex.h>
#include <wx/log.h>

#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>


namespace {

    void GetValues(const wxConfigBase *pConfigFile, const char *pKeyName, int *x, int *y, int *z, int *w)
    {
        std::string defaultString;
        if (w && z && y && x)
        {
            defaultString = boost::str(boost::format("%1%, %2%, %3%, %4%") % *x % *y % *z % *w);
        }
        else if (z && y && x)
        {
            defaultString = boost::str(boost::format("%1%, %2%, %3%") % *x % *y % *z);
        }
        else if (y && x)
        {
            defaultString = boost::str(boost::format("%1%, %2%") % *x % *y);
        }
        else if (x)
        {
            defaultString = boost::str(boost::format("%1%") % *x);
        }
        else
        {
            wxLogWarning("GetValues for ConfigFile does not have valid parameters. Missing input.");
            return;
        }

        std::string resolution = pConfigFile->Read(pKeyName, defaultString.c_str());
        wxStringTokenizer tkz(resolution.c_str(), _T(","));
        size_t numTokens = tkz.CountTokens();
        if (numTokens)
        {
            wxString token = tkz.GetNextToken();
            long tempValue = *x;
            token.ToLong(&tempValue);
            *x = tempValue;
            if (y && tkz.HasMoreTokens())
            {
                token = tkz.GetNextToken();
                tempValue = *y;
                token.ToLong(&tempValue);
                *y = tempValue;
            }
            if (z && tkz.HasMoreTokens())
            {
                token = tkz.GetNextToken();
                tempValue = *z;
                token.ToLong(&tempValue);
                *z = tempValue;
            }
            if (w && tkz.HasMoreTokens())
            {
                token = tkz.GetNextToken();
                tempValue = *w;
                token.ToLong(&tempValue);
                *w = tempValue;
            }
        }
    }

    std::string GetConfigFileName(const std::string& appName, const std::string& globalIni, const std::string& defaultName)
    {
        std::string workingDir = eg::registrate::GetExecutablePath();
        if (globalIni == "")
        {
            // check in current directory
            std::string globalIniFileName = workingDir + wxFILE_SEP_PATH + appName + defaultName;
            if (wxFileName::FileExists(globalIniFileName))
            {
                return globalIniFileName;
            }
            else
            {
                // check one directory up
                wxFileName fileName(workingDir + wxFILE_SEP_PATH);
                fileName.RemoveLastDir();
                std::string newWorkingDir = fileName.GetPath();

                globalIniFileName = newWorkingDir + wxFILE_SEP_PATH + appName + defaultName;
                if (wxFileName::FileExists(globalIniFileName))
                {
                    return globalIniFileName;
                }
                else
                {
                    // check for just default name
                    if (appName != "")
                    {
                        return GetConfigFileName("", "", defaultName);
                    }
                }
            }
        }
        else
        {
            std::string globalIniFileName = workingDir + wxFILE_SEP_PATH + globalIni;
            if (!wxFileName::FileExists(globalIniFileName))
            {
                wxFileName fileName(workingDir + wxFILE_SEP_PATH);
                fileName.RemoveLastDir();
                std::string newWorkingDir = fileName.GetPath();

                globalIniFileName = newWorkingDir + wxFILE_SEP_PATH + globalIni;
            }

            return globalIniFileName;
        }

        return defaultName;
    }

} // namespace

namespace eg {
namespace config {


void Install(const std::string& appName, const std::string& globalIni, const std::string& localIni, const std::string& vendorName)
{
    std::string globalIniFileName = GetConfigFileName(appName, globalIni, "Global.ini");
    std::string localIniFileName = GetConfigFileName(appName, localIni, "Local.ini");

    if (wxFileName::FileExists(globalIniFileName))
    {
        wxConfigBase *pConfigFile = new wxFileConfig(appName.c_str(),
                                                     vendorName == "" ? "BeyondLimits" : vendorName.c_str(),
                                                     localIniFileName.c_str(),
                                                     globalIniFileName.c_str());

        delete wxConfigBase::Set(pConfigFile);
    }
    else
    {
        wxConfigBase *pConfigFile = new wxRegConfig(appName.c_str(),
                                                    vendorName == "" ? "BeyondLimits" : vendorName.c_str(),
                                                    localIniFileName.c_str(),
                                                    globalIniFileName.c_str());

        delete wxConfigBase::Set(pConfigFile);
    }
}


void Use(const std::string& configData)
{
    wxStringInputStream inputStream(configData.c_str());
    wxConfigBase *pConfigFile = new wxFileConfig(inputStream);
    delete wxConfigBase::Set(pConfigFile);
}


void Shutdown()
{
    delete wxConfigBase::Set(0);
}


bool ExpandMacros(std::string& textData)
{
    bool bConverted = false;
    wxString convertedtextData = textData.c_str();
    wxRegEx regKey = "\\$\\([A-Za-z_-]*\\)";
    while (regKey.Matches(convertedtextData))
    {
        wxString valueMatch = regKey.GetMatch(convertedtextData, 0);

        wxRegEx macroNameReg = "[A-Za-z_-]+";
        if (macroNameReg.Matches(valueMatch))
        {
            wxString macroName = macroNameReg.GetMatch(valueMatch);
            std::string macroValue = ReadString("Macros/" + macroName, std::string(valueMatch));
            regKey.Replace(&convertedtextData, macroValue.c_str(), 1);
            bConverted = true;
        }
    }

    if (bConverted)
    {
        textData = convertedtextData.c_str();
    }

    return bConverted;
}


Vector2 ReadV2(const char *pKeyName, const Vector2& defaultValue)
{
    return ReadV2(wxConfigBase::Get(false), pKeyName, defaultValue);
}


Vector2 ReadV2(const wxConfigBase *pConfigFile, const char *pKeyName, const Vector2& defaultValue)
{
    Vector2 configValue = defaultValue;
    if (pConfigFile)
    {
        boost::format f("%1%, %2%");
        f % static_cast<int>(configValue.x);
        f % static_cast<int>(configValue.y);
        std::string defaultString = f.str();

        std::string resolution = pConfigFile->Read(pKeyName, defaultString);
        wxStringTokenizer tkz(resolution.c_str(), wxT(","));
        size_t numTokens = tkz.CountTokens();
        if (numTokens)
        {
            wxString token= tkz.GetNextToken();
            long tempValue = static_cast<int>(configValue.x);
            token.ToLong(&tempValue);
            configValue.x = tempValue;
            if (tkz.HasMoreTokens())
            {
                token = tkz.GetNextToken();
                tempValue = static_cast<int>(configValue.y);
                token.ToLong(&tempValue);
                configValue.y = tempValue;
            }
        }
    }

    return configValue;
}


wxRect ReadRect(const char *pKeyName, const wxRect& defaultValue)
{
    return ReadRect(wxConfigBase::Get(false), pKeyName, defaultValue);
}


wxRect ReadRect(const wxConfigBase *pConfigFile, const char *pKeyName, const wxRect& defaultValue)
{
    wxRect returnedValue = defaultValue;
    GetValues(pConfigFile, pKeyName, &returnedValue.x, &returnedValue.y, &returnedValue.width, &returnedValue.height);
    return returnedValue;
}


wxPoint ReadP2(const char *pKeyName, const wxPoint& defaultValue)
{
    return ReadP2(wxConfigBase::Get(false), pKeyName, defaultValue);
}


wxPoint ReadP2(const wxConfigBase *pConfigFile, const char *pKeyName, const wxPoint& defaultValue)
{
    Vector2 dValue(defaultValue.x, defaultValue.y);
    Vector2 configValue = ReadV2(pConfigFile, pKeyName, dValue);
    wxPoint returnedValue(configValue.x, configValue.y);

    return returnedValue;
}


wxSize ReadS2(const char *pKeyName, const wxSize& defaultValue)
{
    return ReadS2(wxConfigBase::Get(false), pKeyName, defaultValue);
}


wxSize ReadS2(const wxConfigBase *pConfigFile, const char *pKeyName, const wxSize& defaultValue)
{
    Vector2 dValue(defaultValue.x, defaultValue.y);
    Vector2 configValue = ReadV2(pConfigFile, pKeyName, dValue);
    wxSize returnedValue(configValue.x, configValue.y);

    return returnedValue;
}


std::string ReadFullPath(const std::string& keyName)
{
    std::string retValue = ReadPath(keyName.c_str());
    eg::config::ExpandMacros(retValue);
    retValue = NormalizePath(retValue, true, false);
    if (retValue == std::string(1, wxFILE_SEP_PATH))
    {
        retValue = registrate::GetExecutablePath();
        retValue = NormalizePath(retValue, true, false);
    }

    return retValue;
}


std::string ReadPath(const char *pKeyName, const std::string& pathPrefix)
{
    return ReadPath(wxConfigBase::Get(false), pKeyName, pathPrefix);
}


std::string ReadPath(const wxConfigBase *pConfigFile, const char *pKeyName, const std::string& pathPrefix)
{
    std::string folderPath = pathPrefix;
    if (pConfigFile)
    {
        wxString value;
        pConfigFile->Read(pKeyName, &value);
        folderPath += (pathPrefix == "" ? wxT("") : wxT("\\")) + value + wxT("\\");

        // now, let's see if we need to replace $(RootFolder) macro.
        if (boost::icontains(folderPath, std::string("$(RootFolder)")))
        {
            // let's see if we have RootFolder key under the same keyName but stripped the last section
            //boost::iterator_range<char*> result = boost::find_last(pKeyName, "/");
            std::string keyName(pKeyName);
            std::string::size_type index = keyName.find_last_of("/");
            if (index != std::string::npos)
            {
                // strip last part of path form pKeyName
                keyName.erase(index);
                keyName += "/RootFolder";
                std::string rootFolderName = config::ReadPath(pConfigFile, keyName.c_str(), "");
                if (rootFolderName != "")
                {
                    boost::ireplace_all(folderPath, std::string("$(RootFolder)"), rootFolderName);
                    wxFileName fileObject(folderPath.c_str());
                    fileObject.Normalize();
                    folderPath = fileObject.GetFullPath();
                }
            }
        }
    }

    return folderPath;
}


bool ReadBool(const char *pKeyName, bool defaultValue)
{
    return ReadBool(wxConfigBase::Get(false), pKeyName, defaultValue);
}


std::string ReadString(const char *pkeyName, const std::string& defaultValue)
{
    return ReadString(wxConfigBase::Get(false), pkeyName, defaultValue);
}


std::string ReadString(const wxConfigBase *pConfigFile, const char *pKeyName, const std::string& defaultValue)
{
    std::string value = defaultValue;
    if (pConfigFile)
    {
        value = pConfigFile->Read(pKeyName, defaultValue);
    }
    return value;
}


std::vector<std::string> ReadStringArray(const char *pKeyName, const std::vector<std::string>& defaultValue)
{
    return ReadStringArray(wxConfigBase::Get(false), pKeyName, defaultValue);
}


std::vector<std::string> ReadStringArray(const wxConfigBase *pConfigFile, const char *pKeyName, const std::vector<std::string>& defaultValue)
{
    std::vector<std::string> tokens;
    std::string line = ReadString(pConfigFile, pKeyName);
    if (line.empty())
    {
        return defaultValue;
    }

    wxStringTokenizer tkz(line, wxT(","));
    while (tkz.HasMoreTokens())
    {
        std::string token = tkz.GetNextToken();
        boost::trim(token);
        tokens.push_back(token);
    }

    return tokens;
}


int32_t ReadInt(const char *pKeyName, int32_t defaultValue)
{
    return ReadInt(wxConfigBase::Get(false), pKeyName, defaultValue);
}


int32_t ReadInt(const wxConfigBase *pConfigFile, const char *pKeyName, int32_t defaultValue)
{
    int32_t result = defaultValue;
    if (pConfigFile)
    {
        if (pConfigFile->Read(pKeyName, &result, defaultValue))
        {
            return result;
        }
    }

    return result;
}


uint64_t ReadUInt64(const char *pKeyName, uint64_t defaultValue)
{
    return ReadUInt64(wxConfigBase::Get(false), pKeyName, defaultValue);
}


uint64_t ReadUInt64(const wxConfigBase *pConfigFile, const char *pKeyName, uint64_t defaultValue)
{
    uint64_t result = defaultValue;
    std::string resultStr = ReadString(pConfigFile, pKeyName, "");
    if (!resultStr.empty())
    {
        try
        {
            result = boost::lexical_cast<uint64_t>(resultStr);
        }
        catch(boost::bad_lexical_cast&)
        {
            // do nothing here but we still want to log the error here
            wxLogError("Could not convert '%s' to uint64 from ini path: '%s'.", resultStr.c_str(), pKeyName);
        }
    }
    return result;
}


float ReadFloat(const char *pKeyName, float defaultValue)
{
    return ReadFloat(wxConfigBase::Get(false), pKeyName, defaultValue);
}


float ReadFloat(const wxConfigBase *pConfigFile, const char *pKeyName, float defaultValue)
{
    double result = ReadDouble(pConfigFile, pKeyName, defaultValue);
    return static_cast<float>(result);
}


double ReadDouble(const char *pKeyName, double defaultValue)
{
    return ReadDouble(wxConfigBase::Get(false), pKeyName, defaultValue);
}


double ReadDouble(const wxConfigBase *pConfigFile, const char *pKeyName, double defaultValue)
{
    double result = defaultValue;
    if (pConfigFile)
    {
        if (pConfigFile->Read(pKeyName, &result, defaultValue))
        {
            return result;
        }
    }

    return result;
}




bool ReadBool(const wxConfigBase *pConfigFile, const char *pKeyName, bool defaultValue)
{
    bool result = defaultValue;
    if (pConfigFile)
    {
        if (pConfigFile->Read(pKeyName, &result, defaultValue))
        {
            return result;
        }
        else
        {
            // here we try to read false or true as a value
            wxString strValue;
            if (pConfigFile->Read(pKeyName, &strValue))
            {
                std::string value = (const char *)strValue.c_str();
                if (boost::algorithm::iequals(value, std::string("true")))
                {
                    result = true;
                }
                else if (boost::algorithm::iequals(value, std::string("false")))
                {
                    result = false;
                }
            }
        }
    }

    return result;
}


bool EnumerateGroup(const std::string& groupName, std::vector<std::string>& keyList)
{
    return EnumerateGroup(wxConfigBase::Get(false), groupName, keyList);
}


bool EnumerateGroup(const wxConfigBase *pConfigFile, const std::string& groupName, std::vector<std::string>& keyList)
{
    if (pConfigFile)
    {
        // we are casting const away here, but we do guarantee that we'll not
        // change the state of config objects after leaving this
        wxConfigBase *pLocalConfig = const_cast<wxConfigBase *>(pConfigFile);

        if (pLocalConfig->HasGroup(groupName))
        {
            pLocalConfig->SetPath(groupName);
            wxString entryName;
            long eIndex;

            if (pLocalConfig->GetFirstEntry(entryName, eIndex))
            {
                std::string tempBuffer = entryName.c_str();
                keyList.push_back(tempBuffer);
                while (pLocalConfig->GetNextEntry(entryName, eIndex))
                {
                    tempBuffer = entryName.c_str();
                    keyList.push_back(tempBuffer);
                }
            }

            pLocalConfig->SetPath(wxCONFIG_PATH_SEPARATOR);
        }
    }

    return !keyList.empty();
}


bool EnumerateSections(const std::string& groupName, std::vector<std::string>& sectionList)
{
    return EnumerateSections(wxConfigBase::Get(false), groupName, sectionList);
}


bool EnumerateSections(const wxConfigBase *pConfigFile, const std::string& groupName, std::vector<std::string>& sectionList)
{
    if (pConfigFile)
    {
        // we are casting const away here, but we do guarantee that we'll not
        // change the state of config objects after leaving this
        wxConfigBase *pLocalConfig = const_cast<wxConfigBase *>(pConfigFile);

        if (pLocalConfig->HasGroup(groupName))
        {
            pLocalConfig->SetPath(groupName);
            wxString entryName;
            long eIndex;

            if (pLocalConfig->GetFirstGroup(entryName, eIndex))
            {
                std::string tempBuffer = entryName.c_str();
                sectionList.push_back(tempBuffer);
                while (pLocalConfig->GetNextGroup(entryName, eIndex))
                {
                    tempBuffer = entryName.c_str();
                    sectionList.push_back(tempBuffer);
                }
            }

            pLocalConfig->SetPath(wxCONFIG_PATH_SEPARATOR);
        }
    }

    return !sectionList.empty();
}





void WriteV2(const char *pKeyName, const Vector2& value)
{
    WriteV2(wxConfigBase::Get(false), pKeyName, value);
}


void WriteV2(wxConfigBase *pConfigFile, const char *pKeyName, const Vector2& value)
{
    boost::format f("%1%, %2%");
    f % static_cast<int>(value.x);
    f % static_cast<int>(value.y);
    std::string stringValue = f.str();
    WriteString(pConfigFile, pKeyName, stringValue);
}

void WriteRect(const char *pKeyName, const wxRect& value)
{
    WriteRect(wxConfigBase::Get(false), pKeyName, value);
}


void WriteRect(wxConfigBase *pConfigFile, const char *pKeyName, const wxRect& value)
{
    std::string stringValue = boost::str(boost::format("%1%, %2%, %3%, %4%") % value.x % value.y % value.width % value.height);
    WriteString(pConfigFile, pKeyName, stringValue);
}

void WriteP2(const char *pKeyName, const wxPoint& value)
{
    WriteP2(wxConfigBase::Get(false), pKeyName, value);
}


void WriteP2(wxConfigBase *pConfigFile, const char *pKeyName, const wxPoint& value)
{
    WriteV2(pConfigFile, pKeyName, Vector2(value.x, value.y));
}


void WriteS2(const char *pKeyName, const wxSize& value)
{
    WriteS2(wxConfigBase::Get(false), pKeyName, value);
}


void WriteS2(wxConfigBase *pConfigFile, const char *pKeyName, const wxSize& value)
{
    WriteV2(pConfigFile, pKeyName, Vector2(value.x, value.y));
}


void WritePath(const char *pKeyName, const std::string& value)
{
    WritePath(wxConfigBase::Get(false), pKeyName, value);
}


void WritePath(wxConfigBase *pConfigFile, const char *pKeyName, const std::string& value)
{
    WriteString(pConfigFile, pKeyName, value);
}


void WriteString(const char *pKeyName, const std::string& value)
{
    WriteString(wxConfigBase::Get(false), pKeyName, value);
}


void WriteString(wxConfigBase *pConfigFile, const char *pKeyName, const std::string& value)
{
    if (pConfigFile)
    {
        pConfigFile->Write(pKeyName, value.c_str());
    }
}


void WriteBool(const char *pKeyName, bool value)
{
    WriteBool(wxConfigBase::Get(false), pKeyName, value);
}


void WriteBool(wxConfigBase *pConfigFile, const char *pKeyName, bool value)
{
    std::string stringValue = value ? "true" : "false";
    WriteString(pConfigFile, pKeyName, stringValue);
}


void WriteInt(const char *pKeyName, int32_t value)
{
    WriteInt(wxConfigBase::Get(false), pKeyName, value);
}


void WriteInt(wxConfigBase *pConfigFile, const char *pKeyName, int32_t value)
{
    if (pConfigFile)
    {
        pConfigFile->Write(pKeyName, value);
    }
}



} // namespace config
} // namespace eg

#endif // 0

