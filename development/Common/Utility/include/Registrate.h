///////////////////////////////////////////////////////////////////////
// Registrate.h
//
//  Copyright 11/26/2006 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      Accessor functions and macros for getting factories instances
//      It also provides entry points for getting registered objects
//      and some helpers for returning app name and executable path.
//
//  #include "Registrate.h"
//
//////////////////////////////////////////////////////////////////////
//! \file

#ifndef REGISTRATE_H
#define REGISTRATE_H
#pragma once

#if 0

#include "Base.h"
#include <string>
#include <boost/any.hpp>


//! \namespace
namespace eg
{
    //! \namespace
    namespace registrate
    {
        /*!
        Return pointer of type T or NULL, if empty or not the right type
        \code
        boost::any someValue = <...>;   // value to expect pointer of type T from
        Foo *pFoo = registrate::p_cast<Foo>(someValue);
        // if (pFoo) ... do somthing
        \endcode
        */
        template <typename T>
        inline T *p_cast(boost::any value)
        {
            if (value.empty())
            {
                return 0;
            }

            try
            {
                return boost::any_cast<T *>(value);
            }
            catch (const boost::bad_any_cast& /*er*/)
            {
                return 0;
            }
        }

        /*!
        Return pointer of type T or NULL, if empty or not the right type
        \code
        Foo *pFoo = registrate::p_cast<Foo>("Foo");
        if (pFoo) ... do something
        \endcode
        */
        template <typename T>
        inline T *p_cast(const char *pName)
        {
            return p_cast<typename T>(registrate::GetObject(pName));
        }

        /*!
        Return reference of type T. Object must exist and be of correct type
        in Registrate. There is no error or validation checking. Use only if
        you are sure that this object exist.
        \code
        Foo& foo = registrate::ref_cast<Foo>("Foo");
        // ... do somthing
        \endcode
        */
        template <typename T>
        inline T& ref_cast(const char *pName)
        {
            T *pObject = p_cast<T>(registrate::GetObject(pName));
            return *pObject;
        }

        /*!
        This will return object in boost::any which is associated with pName.
        It is up to a client to cast it to correct type, IOW client needs to
        know type if queried object.
        */
        //YAGET_GLOBAL_FUNCTION boost::any GetObject(const char *pName);

        //! This will register or unregister object.
        //! If object is empty, then unregister it
        YAGET_BASE_SYMBOL void RegisterObject(boost::any object, const char *pName);

        //! This is new functions for registrate objects
        //! There is no implementation in Utility library, but in Registrate dll
        //! so you need to link to both
        YAGET_BASE_SYMBOL boost::any Get(const char *name);
        YAGET_BASE_SYMBOL void Set(const char *name, boost::any value);


    }  // namespace registrate


    //! Just the helper to actually use const char * to string
    #define MUCK_QUOTE(name) #name
    //! We provide simple macro to easy of getting of registered factories
    //! It will return reference, so make sure that factory already exists
    //! Dispatcher& disp = REGISTRATE(Dispatcher);
    #define REGISTRATE(name) eg::registrate::ref_cast<##name##>(MUCK_QUOTE(name))

} // namespace eg

#endif // 0
#endif // REGISTRATE_H

