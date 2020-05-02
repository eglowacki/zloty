///////////////////////////////////////////////////////////////////////
// MemoryFileFactory.h
//
//  Copyright 11/15/2006 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      This provides file object like streams using memory
//
//
//  #include "File/MemoryFileFactory.h"
//
//////////////////////////////////////////////////////////////////////
//! \file

#ifndef MEMORY_FILE_FACTORY_H
#define MEMORY_FILE_FACTORY_H
#pragma once

#include "File/VirtualFileFactory.h"
#include <map>
#include <boost/algorithm/string.hpp>


namespace eg
{
    class Dispatcher;

    /*!
    Basic memory file factory. Data only persist during program runtime.
    It is usefully for temporary results or storage.
    */
    class MemoryFileFactory : public VirtualFileFactory
    {
    public:
        MemoryFileFactory(const std::string& type, uint32_t prority, const std::vector<std::string>& folders);
        virtual ~MemoryFileFactory();

        // from VirtualFileFactory
        virtual std::vector<std::string> GetFileList(const std::string& filter) const;
        virtual std::string GetFqn(const std::string& name) const;
        virtual const char *Name() const { return "Memory";}
        virtual istream_t GetFileStream(const std::string& name) const;
        virtual ostream_t AttachFileStream(const std::string& name);
        virtual bool IsFileStreamExist(const std::string& name) const;
        virtual bool CanAttachFileStream(const std::string& name) const;
        //virtual bool WatchFileStream(const std::string& name, fFileStreamChanged fileStreamChanged);
        //virtual void UnwatchFileStream(const std::string& name);
        virtual uint32_t Prority() const {return mPrority;}

    private:
        typedef boost::shared_ptr<std::stringstream> iostream_t;
        //! map of available memory data streams
        //! key - name of stream
        //! value = memory io stream. This is main stream from which we always make copy of
        //! when user request
        typedef std::map<std::string, iostream_t> DataStream_t;
        //! map off all files (stream of data) available to this factory
        DataStream_t mStreamFiles;
        std::string mType;

        uint32_t mPrority;
    };

} // namespace eg

#endif // MEMORY_FILE_FACTORY_H

