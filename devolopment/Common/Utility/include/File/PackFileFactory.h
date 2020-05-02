///////////////////////////////////////////////////////////////////////
// PackFileFactory.h
//
//  Copyright 11/17/2006 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      This provides factories for pack type of files (zip, etc)
//
//
//  #include "File/PackFileFactory.h"
//
//////////////////////////////////////////////////////////////////////
//! \file

#ifndef PACK_FILE_FACTORY_H
#define PACK_FILE_FACTORY_H
#pragma once

#if 0
    #include "File/VirtualFileFactory.h"
    #include "Compression/unzip.h"

class wxString;

namespace eg
{
    class Dispatcher;

    /*!
    Zip file factory. It only provides GetFileStream (read only) support.
    */
    class PackFileFactory : public VirtualFileFactory
    {
    public:
        PackFileFactory(const std::string& type, uint32_t prority, const std::vector<std::string>& folders);
        virtual ~PackFileFactory();

        // from VirtualFileFactory
        virtual std::vector<std::string> GetFileList(const std::string& filter) const;
        virtual std::string GetFqn(const std::string& name) const;
        virtual const char *Name() const { return "Pack";}
        virtual istream_t GetFileStream(const std::string& name) const;
        virtual ostream_t AttachFileStream(const std::string& name);
        virtual bool IsFileStreamExist(const std::string& name) const;
        virtual bool CanAttachFileStream(const std::string& name) const;
        virtual bool WatchFileStream(const std::string& name, fFileStreamChanged fileStreamChanged);
        virtual void UnwatchFileStream(const std::string& name);
        virtual uint32_t Prority() const {return mPrority;}

    private:
        //wxPathList mPathList;
        std::string mType;
        std::vector<HZIP> mPackFileHandles;

        uint32_t mPrority;
    };


} // namespace eg
#endif // 0

#endif // PACK_FILE_FACTORY_H

