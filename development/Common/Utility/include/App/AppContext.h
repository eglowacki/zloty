/////////////////////////////////////////////////////////////////////////
// AppContext.h
//
//  Copyright 4/29/2009 Edgar Glowacki.
//
// NOTES:
//      Some helper stuff for managing app level object and logic.
//      This header is pretty heavy in includes.
//
//
// #include "App/AppContext.h"
//
/////////////////////////////////////////////////////////////////////////
//! \file

#ifndef APP_APP_CONTEXT_H
#define APP_APP_CONTEXT_H
#pragma once

#include "Plugin/IPluginObject.h"
#include "IdGameCache.h"
#include "Message/Dispatcher.h"
#include "File/VirtualFileSystem.h"
#include "File/AssetLoadSystem.h"
#include "Input/InputManager.h"
#include "Timer/Clock.h"
#include "Plugin/PluginManager.h"
#include "Script/ScriptAsset.h"
//#include "Profiler/Profiler.h"

namespace eg
{

    namespace app
    {
        struct Context
        {
            Context(VirtualFileSystem::SettingCallback_t settingCallback)
            : idCache()
            , dispatcher()
            , vfs(settingCallback)
            , als(vfs, idCache, dispatcher)
            , input(dispatcher, als, "KeyBindings")
            , clockManager()
            , pluginManager(&environment)
            , script(ScriptAsset::Register(als, dispatcher, input, pluginManager))
            //, profiler(input)
            {
                environment.vars["Script"] = script;
                environment.vars["als"] = &als;
            }

            ~Context()
            {
                // since some of the environment objects
                // might be shared pointer, we need to clear our
                // references here, specially before unloading any plugins.
                environment.vars.clear();

                dispatcher.Clear();
            }

            Environment environment;
            IdGameCache idCache;
            Dispatcher dispatcher;
            VirtualFileSystem vfs;
            AssetLoadSystem als;
            InputManager input;
            ClockManager clockManager;
            PluginManager pluginManager;
            IAsset::Token script;
            //Profiler profiler;
        };

    } // namespace app

} // namespace eg

#endif // APP_APP_CONTEXT_H

