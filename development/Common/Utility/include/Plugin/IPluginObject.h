///////////////////////////////////////////////////////////////////////
// IPluginObject.h
//
//  Copyright 5/21/2006 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      This is base class for all the plugin objects provided by DLL's.
//      It provides basic Init, Reset and Tick methods
//
//
//  #include "Plugin/IPluginObject.h"
//
//////////////////////////////////////////////////////////////////////
//! \file

#ifndef PLUGIN_IPLUGIN_OBJECT_H
#define PLUGIN_IPLUGIN_OBJECT_H
#pragma once

#include "Exception/Exception.h"
#include <map>
#include <boost/any.hpp>
#include <boost/function.hpp>


namespace eg
{
    /*!
    Base plugin class from all of the dynamic dll objects are derived.
    It provides interface for PluginManager.
    */
    class IPluginObject
    {
        //@{
        //! no copy semantics
        IPluginObject(const IPluginObject&);
        IPluginObject& operator=(const IPluginObject&);
        //@}

    public:
        //!Return name of this plugin
        virtual const char *GetName() const = 0;

        //! Return Yaget engine version number used for this plugin
        virtual const Version GetVersion() const = 0;

    protected:
        IPluginObject() {}
        virtual ~IPluginObject() = 0;
    };

    inline IPluginObject::~IPluginObject() {};

    struct Environment
    {
        typedef std::map<std::string, boost::any> vars_t;
        vars_t vars;

        template <typename T>
        inline bool is(const std::string& name) const
        {
            vars_t::const_iterator it = vars.find(name);
            if (it != vars.end())
            {
                boost::any value = (*it).second;
                return value.type() == typeid(T);
            }

            return false;
        }

        template <typename T>
        inline T get(const std::string& name) const
        {
            vars_t::const_iterator it = vars.find(name);
            if (it != vars.end())
            {
                boost::any value = (*it).second;
                if (value.type() == typeid(T))
                {
                    T token = boost::any_cast<T>(value);
                    return token;
                }
            }

            throw ex::plugin("Could not access environment entry.");
        }

        template <typename T>
        inline T& get_ref(const std::string& name) const
        {
            return *get<T *>(name);
        }
    };


    //! yaget plugin exported functions
    //extern "C" __declspec(dllexport) IPluginObject *get_plugin(const Version& version, const Environment *environment)
    //extern "C" __declspec(dllexport) void free_plugin(IPluginObject *plugin)

} // namespace eg

#endif // PLUGIN_IPLUGIN_OBJECT_H

