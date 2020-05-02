///////////////////////////////////////////////////////////////////////
// DiskFileFactory.h
//
//  Copyright 11/12/2006 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      This provides factories for disk type of files
//
//
//  #include "File/DiskFileFactory.h"
//
//////////////////////////////////////////////////////////////////////
//! \file

#ifndef DISK_FILE_FACTORY_H
#define DISK_FILE_FACTORY_H
#pragma once

#include "File/VirtualFileFactory.h"
#include <istream>
#include <boost/shared_ptr.hpp>

namespace eg
{
    class Message;
    class Dispatcher;

    /*!
    Basic disk file factory
    */
    class DiskFileFactory : public VirtualFileFactory
    {
    public:
        DiskFileFactory(const std::string& type, uint32_t prority, const std::vector<std::string>& folders);
        virtual ~DiskFileFactory();

        // from VirtualFileFactory
        virtual std::vector<std::string> GetFileList(const std::string& filter) const;
        virtual std::string GetFqn(const std::string& name) const;
        virtual const char *Name() const { return "Disk";}
        virtual istream_t GetFileStream(const std::string& name) const;
        virtual ostream_t AttachFileStream(const std::string& name);
        virtual bool IsFileStreamExist(const std::string& name) const;
        virtual bool CanAttachFileStream(const std::string& name) const;
        //virtual bool WatchFileStream(const std::string& name, fFileStreamChanged fileStreamChanged);
        //virtual void UnwatchFileStream(const std::string& name);
        virtual uint32_t Prority() const {return mPrority;}

    private:
        void onFileChanged(const std::string& name, const std::string& path);

        //! This will return full file path if name exists as a file in matching folder
        //! if folder is 'c:/assets/texture' and there is a physical file at 'c:/assets/texture/house/brick.tga'
        //! then refering to name as 'house/brick.tga' will find that file.
        std::string findFullFilePath(const std::string& name) const;

        std::string mType;
        //std::map<std::string, boost::shared_ptr<FileWatcher> > mFileWatchers;

        uint32_t mPrority;
        std::vector<std::string> mFolderList;
   };


} // namespace eg

#endif // DISK_FILE_FACTORY_H

