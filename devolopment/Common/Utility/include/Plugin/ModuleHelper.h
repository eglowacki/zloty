//////////////////////////////////////////////////////////////////////
// ModuleHelper.h
//
//  Copyright 12/16/2007 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//
//
//  #include "Plugin/ModuleHelper.h"
//
//////////////////////////////////////////////////////////////////////
//! \file

#ifndef _PLUGIN_MODULE_HELPER_H
#define _PLUGIN_MODULE_HELPER_H
#pragma once

#include "Base.h"
#include <string>
#include <map>

namespace eg
{
    namespace module
    {
        //! Function called when component is being registared
        //! with ObjectManager
        typedef bool (*RegisterComponentType_t)(void);
        bool RegisterComponent(const std::string& name, RegisterComponentType_t registerFunction);

        //! Read settings from config file applying wild card
        void ActivateComponents(const std::string& name);

        //#define MUCK_QUOTE(name) #name
        //#define REGISTRATE(name) eg::registrate::ref_cast<##name##>(MUCK_QUOTE(name))

        #define EG_MODULE_REGISTER_COMPONENT(classType) static bool dummy##classType = eg::module::RegisterComponent(#classType, &classType::RegisterComponentType)

    } // namespace module


} // namespace eg


#endif // _PLUGIN_MODULE_HELPER_H

