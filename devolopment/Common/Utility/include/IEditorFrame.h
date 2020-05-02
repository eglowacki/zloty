///////////////////////////////////////////////////////////////////////
// IEditorFrame.h
//
//  Copyright 3/30/2006 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      Interface for editor frame which houses editor controls
//      object properties.
//
//
//  #include "IEditorFrame.h"
//
//////////////////////////////////////////////////////////////////////
//! \file

#ifndef I_EDITOR_FRAME_H
#define I_EDITOR_FRAME_H
#pragma once

#include "IEditorMessages.h"
#include "IMetPlugin.h"
#include "Plugin/IPluginObject.h"
#include "Message/Dispatcher.h"
#include "Property/PropertyData.h"
#include "File/VirtualFileSystem.h"
#include "File/ArchiveHelpers.h"
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>


namespace eg
{

    /*!
    Interface for Editor frame. This also houses all the tabs
    for controls, like scene viewer, object properties, templates, etc.
    */
    class DLLIMPEXP_UTIL_CLASS IEditorFrame : public IPluginObject
    {
    public:
    };


    namespace config
    {

        /*!
        This class saves and loads state of the window (pos, size, visibility, etc)
        \code
        // to load
        config::PanelUI panelUI("Viewer");
        // will contain last save values or safe defualt ones
        // and can be used in creation of window
        panelUI.Pos,
        panelUI.Size,

        // to save
        pWindow = // ... window we want to save state for
        config::PanelUI panelUI("Viewer", pWindow);
        \endcode
        */
        class PanelUI
        {
        public:
            /*!
            This one ctor is used for both loading and saving of ui config data
            \param pName name of this panel
            \param pToolFrame frame window which owns this tool panel. If NULL, then load config file
                   otherwise if false, extract needed data from pToolFrame and save data to config file.
            */
            PanelUI(const char *pName, wxWindow *pToolFrame, VirtualFileSystem& vfs);

            wxPoint Pos;
            wxSize Size;

        private:
            PropertySet mData;
        };

        //--------------------------------------------------------------------
        /*!
        Generic UI settings loading and saving
        */
        template <typename T, typename E>
        class SettingsUI
        {
        public:
            //! load file into this object
            //! config::ToolBarUI myData("MyData");
            //! mSomeValueToSave = myData.Value();
            SettingsUI(const std::string& name, VirtualFileSystem& vfs)
            {
                std::string fileName = name + E::Type();
                if (VirtualFileFactory::istream_t file = vfs.GetFileStream(fileName))
                {
                    LoadFromArchive<boost::archive::text_iarchive>(*this, *file.get());
                }
            }

            //! save this object to the file
            //! config::ToolBarUI("MyData", mSomeValueToSave);
            SettingsUI(const std::string& name, const T& value, VirtualFileSystem& vfs) : mValue(value)
            {
                std::string fileName = name + E::Type();
                if (VirtualFileFactory::ostream_t file = vfs.AttachFileStream(fileName))
                {
                    SaveToArchive<boost::archive::text_oarchive>(*this, *file.get());
                }
            }

            const T& Value() const {return mValue;}

        private:
            friend class boost::serialization::access;
            template<class Archive>
            void serialize(Archive & ar, const unsigned int version)
            {
                ar & mValue;
            }

            T mValue;
        };

    } // namespace config



    //-------------------------------------------------------------------------------------------
    inline config::PanelUI::PanelUI(const char *pName, wxWindow *pToolFrame, VirtualFileSystem& vfs)
    : Pos(wxDefaultPosition), Size(wxDefaultSize)
    {
        mData.SetGroupName(pName);
        mData.AddProperty<wxPoint, wxPoint>("Pos", &Pos, false);
        mData.AddProperty<wxSize, wxSize>("Size", &Size, false);

        if (pName)
        {
            std::string name(pName);
            std::string fileName = name + std::string(".pcf"); // (P)anel (C)onfig (F)ile
            if (pToolFrame)
            {
                // saving panel data to config file
                Pos = pToolFrame->GetPosition();
                Size = pToolFrame->GetSize();

                // saving panel data to config file
                if (VirtualFileFactory::ostream_t panelFile = vfs.AttachFileStream(fileName))
                {
                    SaveToArchive<boost::archive::text_oarchive>(mData, *panelFile.get());
                }
            }
            else
            {
                // loading panel data from config file
                if (VirtualFileFactory::istream_t panelFile = vfs.GetFileStream(fileName))
                {
                    LoadFromArchive<boost::archive::text_iarchive>(mData, *panelFile.get());
                }
            }
        }
    }

} // namespace eg

#endif // I_EDITOR_FRAME_H


