///////////////////////////////////////////////////////////////////////
// IMetPlugin.h
//
//  Copyright 8/6/2006 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      Interface to create and manage component UI panels.
//
//
//  #include "IMetPlugin.h"
//
//////////////////////////////////////////////////////////////////////
//! \file

#ifndef I_MET_PLUGIN_H
#define I_MET_PLUGIN_H
#pragma once


#include "Base.h"

class wxWindow;

namespace eg
{

    class IMetPlugin
    {
    public:
        virtual ~IMetPlugin() = 0 {}

        /*!
        Call this to create and initialize this particular UI panel.
        \param pParentWindow parent window which owns this panel.
        \return pointer to it's window, which will be used in dock manger
        */
        virtual wxWindow *Init(wxWindow *pParentWindow) = 0;

        //! Return name of this net plugin
        virtual const char *GetName() const = 0;
    };

} // namespace eg

#endif // I_MET_PLUGIN_H

