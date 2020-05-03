///////////////////////////////////////////////////////////////////////
// Loader.h
//
//  Copyright 4/22/2006 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//
//
//  #include "Plugin/Loader.h"
//
//////////////////////////////////////////////////////////////////////
//! \file

#ifndef PLUGIN_LOADER_H
#define PLUGIN_LOADER_H
#pragma once

#if 0
#include "Base.h"
#include <vector>

// Forward declarations
class wxConfigBase;
class wxPluginLibrary;


namespace eg
{

    typedef std::vector<wxPluginLibrary *> vPluginLibrary;
    enum ePluginType
    {
        eptPrerequisite = 1,    ///< This plugins are required to be loaed and intialized before any other plugins are used
        eptDynamic              ///< any secondary plugins to load
    };

    /*!
    This will load all plugins specified by configuration file.
    \param pConfigFile configuration file to read for which plugins to load
    \param plugins [OUT] this will contain all the loaded plugins
    \return FALSE if it could not load all required plugins, otherwise TRUE

    section for the configuration file
    [Plugins/Prerequisite]
    Objects

    [Plugins/Dynamic]
    Render = required
    Gui = optional
    Editor = skip
    */
    bool LoadPlugins(const wxConfigBase *pConfigFile, vPluginLibrary& plugins, ePluginType pluginType);
    bool LoadPlugins(vPluginLibrary& plugins, ePluginType pluginType);

} // namespace eg

#endif // 0
#endif // PLUGIN_LOADER_H

