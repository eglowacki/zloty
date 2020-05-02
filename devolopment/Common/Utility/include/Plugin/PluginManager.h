///////////////////////////////////////////////////////////////////////
// PluginManager.h
//
//  Copyright 5/21/2006 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      This manages all the plugins
//
//
//  #include "Plugin/PluginManager.h"
//
//////////////////////////////////////////////////////////////////////
//! \file

#ifndef PLUGIN_PLUGIN_MANAGER_H
#define PLUGIN_PLUGIN_MANAGER_H
#pragma once

#include "MessageInterface.h"
#include "Plugin/Loader.h"
#include <vector>
#include <boost/shared_ptr.hpp>
#pragma warning(push)
#pragma warning (disable : 4512)  // '' : assignment operator could not be generated
    #include <boost/signals.hpp>
#pragma warning(pop)


namespace eg
{
    class IPluginObject;
    struct Environment;

    /*!
    This object is used internally to load and initialize all plugins.
    It responds to several messages
    kRegisterPlugin - add new plugin name to register list
    kShutdownBegin - delete all plugin objects and unload all plugins
   */
    class PluginManager// : public boost::signals::trackable
    {
        //@{
        //! no copy semantics
        PluginManager(const PluginManager&);
        PluginManager& operator=(const PluginManager&);
        //@}

    public:
        PluginManager(const Environment *environment);
        ~PluginManager();

        //! Load all plugins specified in configuration file
        //!
        //! \return bool true all required plugins where loaded, but optional may or may
        //!              not have been loaded. If false, then required plugin failed and
        //!              all plugins are unloaded.
        //bool Load();

        //! If plugin object does not exist,
        //! throw ex::plugin exception
        IPluginObject& ref(const std::string& name);
        //! This also can be used to check for existence
        IPluginObject *ptr(const std::string& name);
        //! Unload module out of memory, User must assure that
        //! all resource from that module must be freed already.
        void unload(const std::string& name);

        //! Helper template to cast to desired type
        template <typename T>
        T& ref_cast(const std::string& name);

        template <typename T>
        T *p_cast(const std::string& name);

        template <typename T>
        T f_cast(const std::string& name, const std::string& funcName);

    private:
        void *findFunction(const std::string& name, const std::string& funcName);

        std::vector<std::string> mFolders;

        struct Plugin
        {
            typedef void *HANDLE;

            Plugin() : plugin(0), handle(0) {}
            Plugin(IPluginObject *plugin, HANDLE handle) : plugin(plugin), handle(handle) {}

            IPluginObject *plugin;
            HANDLE handle;
        };

        typedef std::map<std::string, Plugin> Plugins_t;
        Plugins_t mPlugins;
        const Environment *mEnvironment;

#if 0
        typedef boost::shared_ptr<IPluginObject> PluginObject_t;
        typedef std::vector<std::string> PluginNameList_t;
        typedef std::vector<PluginObject_t> PluginList_t;

        //! Helper method to create plugin object residing in one of loaded dll's
        // using it's registered name. This does not initialize object.
        PluginObject_t createPluginObject(const std::string& name);

        void StartPlugins();
        void InitPluginObjects(PluginList_t& initializedPlugins);
        //! Helper method to initialize all the need plugins in pluginList
        //! pluginList will also be adjusted based if Plugin Object need to have more init passes
        bool InitPluginsPass(PluginList_t& pluginList, int32_t initPass);
        void RemovePluginObject(const PluginObject_t& pluginObject);

        //! Connected to Dispacher
        void OnMessage(const Message& msg);

        // registered plugin names
        PluginNameList_t mPluginNames;

        // fully initialized plugin objects
        PluginList_t mPlugins;
        PluginList_t mPrerequisitePlugins;

        //vPluginLibrary mDynamicPluginLibraries;
#endif // 0
    };

    template <typename T>
    inline T& PluginManager::ref_cast(const std::string& name)
    {
        return dynamic_cast<T&>(ref(name));
    }

    template <typename T>
    inline T *PluginManager::p_cast(const std::string& name)
    {
        return dynamic_cast<T*>(ptr(name));
    }

    template <typename T>
    inline T PluginManager::f_cast(const std::string& name, const std::string& funcName)
    {
        if (void *pFunction = findFunction(name, funcName))
        {
            return (T)pFunction;
        }

        return 0;
    }

} // namespace eg


#define tr_plugin "plugin"

#endif // PLUGIN_PLUGIN_MANAGER_H

