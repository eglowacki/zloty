#include "Plugin/PluginManager.h"
#include "Message/Dispatcher.h"
#include "Registrate.h"
#include "Plugin/IPluginObject.h"
#include "Logger/Log.h"
#include "Script/ScriptAsset.h"

#include <boost/bind.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>

namespace bfs = boost::filesystem;

namespace
{
/*
    // predicate for find algorithm
    struct Finder
    {
        Finder(const std::string& name) : pluginName(name) {}

        bool operator()(const boost::shared_ptr<eg::IPluginObject>& item) const
        {
            if (boost::algorithm::iequals(item->GetName(), pluginName))
            {
                return true;
            }

            return false;
        }

        std::string pluginName;
    };

    std::string FormatVersion(const eg::Version& pluginVersion)
    {
        return boost::str(boost::format("%1%.%2%.%3%") % pluginVersion.Major % pluginVersion.Minor % pluginVersion.Build);
    }



    struct PluginManagerData
    {
        eg::IPluginObject *FindPlugin(const wxString& pluginName)
        {
            typedef PluginList_t::const_iterator cit_PL;
            cit_PL it = std::find_if(mPlugins.begin(), mPlugins.end(), Finder(pluginName));
            if (it != mPlugins.end())
            {
                return (*it).get();
            }

            it = std::find_if(mPrerequisitePlugins.begin(), mPrerequisitePlugins.end(), Finder(pluginName));
            if (it != mPrerequisitePlugins.end())
            {
                return (*it).get();
            }

            return 0;
        }
    };
*/

} // namespace




namespace eg {

PluginManager::PluginManager(const Environment *environment)
: mEnvironment(environment)
{
    log_trace(tr_plugin) << "PluginManager object created.";
}


PluginManager::~PluginManager()
{
    while (!mPlugins.empty())
    {
        std::string name = (*mPlugins.begin()).first;
        unload(name);
    }

    log_trace(tr_plugin) << "PluginManager object deleted.";
}



#if 0
void PluginManager::OnMessage(const Message& /*msg*/)
{
}


void PluginManager::StartPlugins()
{
    // first we need to load all of the Prerequisite plugins and initialize all
    // of the registered objects
    bool result = false;//LoadPlugins(mDynamicPluginLibraries, eptPrerequisite);
    //wxASSERT_MSG(result, "Could not load all of the Prerequisite Plugins.");
    // we now have all the registered objects to initialize for Prerequisite libraries
    // so we need to force all of the registered plugins to be Initialize
    InitPluginObjects(mPrerequisitePlugins);
    // clear all the registered names for it
    mPluginNames.clear();

    // and here we just load dynamic libraries and let them register their
    // objects, but Initialization will happened on InitPluginObjects() call
    result = false;//LoadPlugins(mDynamicPluginLibraries, eptDynamic);
    //wxASSERT_MSG(result, "Could not load all of the Dynamic Plugins.");
}


void PluginManager::InitPluginObjects(PluginList_t& initializedPlugins)
{
    typedef PluginNameList_t::const_iterator it_PNL;
    // load registered plugins first
    for (it_PNL it = mPluginNames.begin(); it != mPluginNames.end(); ++it)
    {
        if (PluginObject_t pluginObject = createPluginObject(*it))
        {
            initializedPlugins.push_back(pluginObject);
        }
    }

    // next we need to initialize it
    PluginList_t currInitList = initializedPlugins;
    int32_t initPass = 0;

    // we do this initialization until all of plugins return ircDone or ircError
    while (InitPluginsPass(currInitList, initPass))
    {
        initPass++;
    }

    // and since we finished, run one more time on all plugins uaing -1 as a pass value
    currInitList = initializedPlugins;
    if (InitPluginsPass(currInitList, -1))
    {
        //wxLogError("Some of the Plugins requested more Init Passes for -1 Pass.");
    }
}


bool PluginManager::InitPluginsPass(PluginList_t& pluginList, int32_t initPass)
{
    typedef PluginList_t::iterator it_PL;

    PluginList_t nextInitList;
    for (it_PL it = pluginList.begin(); it != pluginList.end(); ++it)
    {
        IPluginObject::eInitReturnCode returnCode = (*it).get()->OnInit(initPass);
        if (returnCode == IPluginObject::ircMore)
        {
            // this plugin needs more passes at initialization
            nextInitList.push_back(*it);
        }
        else if (returnCode == IPluginObject::ircError)
        {
            // plugin object was not able to initialize
            //wxLogError("Plugin '%s' failed to initialize.", (*it)->GetName().c_str());
            // Remove this object from our list
            RemovePluginObject(*it);
        }
        else if (returnCode == IPluginObject::ircUnload)
        {
            // Plugin Object did not want to initialize.
            RemovePluginObject(*it);
        }
    }

    if (initPass > 10)
    {
        //wxLogError("Some of the Plugins failed to initialize after %d passes.", initPass);

        std::for_each(nextInitList.begin(), nextInitList.end(), boost::bind(&PluginManager::RemovePluginObject, this, _1));
        pluginList.clear();
        return false;
    }

    pluginList = nextInitList;
    // if this returns TRUE, we need to run init pass again
    return !pluginList.empty();
}


void PluginManager::RemovePluginObject(const PluginObject_t& pluginObject)
{
    mPlugins.erase(std::remove(mPlugins.begin(), mPlugins.end(), pluginObject), mPlugins.end());
}


PluginManager::PluginObject_t PluginManager::createPluginObject(const std::string& /*name*/)
{
    /*
    if (wxObject *pPlugin = wxCreateDynamicObject(name.c_str()))
    {
        PluginObject_t pluginObject(wxDynamicCast(pPlugin, IPluginObject));
        if (pluginObject)
        {
            // check for version number
            const Version& pluginVersion = pluginObject->GetVersion();
            if (YagetVersion.Major >= pluginVersion.Major)
            {
                wxLogTrace(tr_util, "Plugin Object '%s' version is '%s' and Yaget Engine is %s", name.c_str(), FormatVersion(pluginVersion), FormatVersion(YagetVersion));
                return pluginObject;
            }
            else
            {
                wxLogError("Plugin Object '%s' has %s version and Yaget Engine is %s", name.c_str(), FormatVersion(pluginVersion), FormatVersion(YagetVersion));
            }
        }
        else
        {
            wxLogError("Plugin Object '%s' could not be cast to IPluginObject.", name.c_str());
        }
    }
    */

    return PluginObject_t();
}


IPluginObject *PluginManager::ptr(const std::string& name)
{
    PluginList_t::const_iterator it = std::find_if(mPlugins.begin(), mPlugins.end(), Finder(name));
    if (it != mPlugins.end())
    {
        return (*it).get();
    }

    // ok we did not find existing object, let's try to create one now
    if (PluginObject_t pluginObject = createPluginObject(name))
    {
        PluginList_t initializedPlugins;
        initializedPlugins.push_back(pluginObject);

        // next we need to initialize it
        PluginList_t currInitList = initializedPlugins;
        int32_t initPass = 0;

        // we do this initialization until all of plugins return ircDone or ircError
        while (InitPluginsPass(currInitList, initPass))
        {
            initPass++;
        }

        // and since we finished, run one more time on all plugins using -1 as a pass value
        currInitList = initializedPlugins;
        if (InitPluginsPass(currInitList, -1))
        {
            //wxLogError("Some of the Plugins requested more Init Passes for -1 Pass.");
        }

        std::copy(initializedPlugins.begin(), initializedPlugins.end(), std::back_inserter(mPlugins));

        return pluginObject.get();
    }

    return 0;
}
#endif // 0


IPluginObject& PluginManager::ref(const std::string& name)
{
    return dynamic_cast<IPluginObject&>(*ptr(name));
}


#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace
{
    void logError(const std::string& name)
    {
        LPVOID lpMsgBuf;
        DWORD dw = GetLastError();

        ::FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            dw,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPTSTR) &lpMsgBuf,
            0, NULL );

        log_error << "Could not initialize plugin '" << name << "'. Error(" << logs::hex<uint32_t>(dw) << "): '" << (const char *)lpMsgBuf << "'.";
        ::LocalFree(lpMsgBuf);
    }
} // namespace

void *PluginManager::findFunction(const std::string& name, const std::string& funcName)
{
    Plugins_t::const_iterator it = mPlugins.find(name);
    if (it != mPlugins.end())
    {
        void *pFunction = ::GetProcAddress((HMODULE)(*it).second.handle, funcName.c_str());
        return pFunction;
    }

    HMODULE handle = 0;
    BOOST_FOREACH(const std::string& folder, mFolders)
    {
        bfs::path path(folder);
        path /= name;

        if (bfs::exists(path))
        {
            // now let' try to loaded, find exported function for getting plugin
            // object and then call it to instantiate it.
            handle = ::LoadLibrary(path.string().c_str());
            break;
        }
    }

    // looks like we did not find it in user provided folders,
    // so give it to os
    if (!handle)
    {
        handle = LoadLibrary(name.c_str());
    }

    if (handle)
    {
        typedef IPluginObject *(*pluginFunction)(const Version&, const Environment *);
        if (pluginFunction pFunction = (pluginFunction)::GetProcAddress(handle, "get_plugin"))
        {
            if (IPluginObject *plugin = pFunction(YagetVersion, mEnvironment))
            {
                log_trace(tr_plugin) << "Loaded Plugin '" << name << "', version: " << plugin->GetVersion();

                mPlugins.insert(std::make_pair(name, Plugin(plugin, handle)));
                void *pFunction = ::GetProcAddress(handle, funcName.c_str());
                return pFunction;
            }
        }

        if (!funcName.empty())
        {
            mPlugins.insert(std::make_pair(name, Plugin(0, handle)));
            void *pFunction = ::GetProcAddress(handle, funcName.c_str());
            return pFunction;
        }
    }

    return 0;
}


IPluginObject *PluginManager::ptr(const std::string& name)
{
    Plugins_t::const_iterator it = mPlugins.find(name);
    if (it != mPlugins.end())
    {
        return (*it).second.plugin;
    }

    HMODULE handle = 0;

    BOOST_FOREACH(const std::string& folder, mFolders)
    {
        bfs::path path(folder);
        path /= name;

        if (bfs::exists(path))
        {
            // now let' try to loaded, find exported function for getting plugin
            // object and then call it to instantiate it.
            handle = ::LoadLibrary(path.string().c_str());
            break;
        }
    }

    // looks like we did not find it in user provided folders,
    // so give it to os
    if (!handle)
    {
        handle = LoadLibrary(name.c_str());
    }

    typedef IPluginObject *(*pluginFunction)(const Version&, const Environment *);
    if (pluginFunction pFunction = (pluginFunction)::GetProcAddress(handle, "get_plugin"))
    {
        if (IPluginObject *plugin = pFunction(YagetVersion, mEnvironment))
        {
            log_trace(tr_plugin) << "Loaded Plugin '" << name << "', version: " << plugin->GetVersion();

            mPlugins.insert(std::make_pair(name, Plugin(plugin, handle)));
            return plugin;
        }
    }

    logError(name);
    ::FreeLibrary(handle);
    return 0;
}


void PluginManager::unload(const std::string& name)
{
    Plugins_t::const_iterator it = mPlugins.find(name);
    if (it == mPlugins.end())
    {
        log_error << "There is no plugin '" << name << "' loaded.";
        return;
    }

    typedef void (*pluginFunction)(IPluginObject *);
    Plugin plugin = (*it).second;
    mPlugins.erase(it);

    if (pluginFunction pFunction = (pluginFunction)::GetProcAddress((HMODULE)plugin.handle, "free_plugin"))
    {
        pFunction(plugin.plugin);
    }

    if (!::FreeLibrary((HMODULE)plugin.handle))
    {
        logError(name);
        return;
    }

    log_trace(tr_plugin) << "Unloaded Plugin '" << name << "'.";
}


} // namespace eg
