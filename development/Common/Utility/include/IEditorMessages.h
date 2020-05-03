//////////////////////////////////////////////////////////////////////
// IEditorMessages.h
//
//  Copyright 12/28/2006 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      Enums, typedefs and messages types for Editor functionality
//
//
//  #include "IEditorMessages.h"
//
//////////////////////////////////////////////////////////////////////
//! \file

#ifndef I_EDITOR_MESSAGES_H
#define I_EDITOR_MESSAGES_H
#pragma once

#include "Base.h"


namespace eg
{
    namespace mtype
    {
        //! Send by IEditorFrame to get pointer to main window, which will become
        //! parent of this IEditorFrame
        //! boost::any <wxWindows *>
        static const guid_t kGetValidWindow = 0xb17c6948;

        //! Used mostly by tools to find out it's posible to run
        // boost::any <bool> true we can run tools, otherwise false and tool should not be visible or use any resources
        static const guid_t kAllowTools = 0xb2832895;

        //! Send by plugin to be registered
        //! boost::any <IMetPlugin *> pointer to this tool instance
        //! After receiving this messaeg framework wil call IMetPlugin::Init(...) method
        static const guid_t kNewToolPlugin = 0xf2901766;

        //! Send by managed::FormLoader after all plugin objects created and initialized.
        //! boost::any <bool> not used
        static const guid_t kAllToolPluginInit = 0xf2bdb5f6;

        //! Send by main tool frame when it goes visible or hide
        //! boost::any <bool> true - show youself, false - hide youself
        //! After receiving this messaeg framework wil call IMetPlugin::Init(...) method
        //static const guid_t kToolPluginShow = 0x52990163;

        //! Send by instaining this object and calling Send on it
        //! boost::any <boost::shared_ptr<Command> >
        static const guid_t kNewCommand = 0x129d8b5a;

        /*!
        This will return list of choices in vector<std::string>
        In that order and values from 0 to n-1
        this uses std::pair<std::string, std::vector<std::string> >*
        where first is property name
        and second is vector of string representign each Label name
        */
        static const guid_t kGetChoiceLabels = 0x31eb2681;

        //! When yaget editor plugin is closed by user specifically
        //! it will send this message
        //! boost::any - not used
        static const guid_t kYepClosed = 0x15fce8af;

		//! Send to actually execute konsole command
		//! boost::any std::string
		static const guid_t kKonsoleExec = 0x160672eb;

		//! Send for every output of konsole from executing command
		//! boost::any std::string
		static const guid_t kKonsolePrint = 0x76067344;

		//! Show selected object in view
		//! boost::any ObjectId
		static const guid_t kViewSelectedObject = 0x9619beb8;

    } // namespace mtype


} // namespace eg

#endif // I_EDITOR_MESSAGES_H

