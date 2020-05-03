#if 0
#include "Plugin/Loader.h"
#include <wx/confbase.h>
#include <wx/dynload.h>


namespace {

    bool LoadPlugin(const wxConfigBase *pConfigFile, const std::string& pluginName, bool required, eg::vPluginLibrary& plugins)
    {
        wxConfigPathChanger configPathChanger(pConfigFile, wxCONFIG_PATH_SEPARATOR);
        wxPluginLibrary *pPlugin = wxPluginManager::LoadLibrary(pluginName);
        if (!pPlugin && required)
        {
            // this is error, since this plugin is required
            wxLogError("Required Plugin '%s' failed to load.", pluginName.c_str());
            return false;
        }

        if (pPlugin)
        {
            plugins.push_back(pPlugin);
        }

        return true;
    }

}// namespace


namespace eg {

bool LoadPlugins(vPluginLibrary& plugins, ePluginType pluginType)
{
    return LoadPlugins(wxConfigBase::Get(false), plugins, pluginType);
}

bool LoadPlugins(const wxConfigBase *pConfigFile, vPluginLibrary& plugins, ePluginType pluginType)
{
    wxASSERT(pConfigFile);

    // what kind of plugin user requesting to load
    bool bPrerequisite = pluginType == eptPrerequisite;
    std::string pluginTypeSection = bPrerequisite ? "Plugins/Prerequisite" : "Plugins/Dynamic";

    // we are casting const away here, but we do guarantee that we'll not
    // change the state of config objects after leaving this
    wxConfigBase *pLocalConfig = const_cast<wxConfigBase *>(pConfigFile);

    if (pLocalConfig->HasGroup(pluginTypeSection))
    {
        pLocalConfig->SetPath(pluginTypeSection);
        wxString entryName;
        long eIndex;

        bool findResult = pLocalConfig->GetFirstEntry(entryName, eIndex);
        while (findResult)
        {
            wxString value = pLocalConfig->Read(entryName);
            // we can also specify to skip the plugin
            if (bPrerequisite || !value.IsSameAs("skip", false))
            {
                bool required =  bPrerequisite ? true : value.IsSameAs("required", false);
                if (!LoadPlugin(pLocalConfig, (const char *)entryName.c_str(), required, plugins))
                {
                    if (required)
                    {
                        // this is a fatal error, so we just crash and burn
                        wxLogFatalError("Could not load required '%s' plugin, quitting.", entryName);
                        // the above function call does not return, it just call exit/abort
                        // so if that ever changes, and we want to handle it more gracefully
                        // this should return false, maybe also to unload any plugins,
                        // and the caller should then clean up.
                        //return false;
                    }
                    wxLogWarning(tr_util, "Optional '%s' plugin was not loaded.", entryName);
                    continue;
                }
            }

            findResult = pLocalConfig->GetNextEntry(entryName, eIndex);
        }

        pLocalConfig->SetPath(wxCONFIG_PATH_SEPARATOR);
    }

    return true;
}

} // namespace eg

#endif // 0
