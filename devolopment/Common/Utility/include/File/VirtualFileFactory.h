///////////////////////////////////////////////////////////////////////
// VirtualFileFactory.h
//
//  Copyright 11/12/2006 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      This provides unterface for factories implementation
//      handlong specific type of files, like disk, memory, zip, network, etc
//
//
//  #include "File/VirtualFileFactory.h"
//
//////////////////////////////////////////////////////////////////////
//! \file

#ifndef FILE_VIRTUAL_FILE_FACTORY_H
#define FILE_VIRTUAL_FILE_FACTORY_H
#pragma once


#include "Base.h"
#include <vector>
#include <boost/shared_ptr.hpp>
#pragma warning(push)
#pragma warning (disable : 4244)  // '' : conversion from '' to '', possible loss of data
#pragma warning (disable : 4512)  // '' : assignment operator could not be generated
#include <boost/signals.hpp>
#include <boost/function.hpp>
#pragma warning(pop)


namespace eg
{

    /*!
    This factory can represent any kind of underling storage
    */
    class VirtualFileFactory
    {
        // no copy semantics
        VirtualFileFactory(const VirtualFileFactory&);
        VirtualFileFactory& operator=(const VirtualFileFactory&);

    public:
        //! Typedef representing streams using shared pointers
        typedef boost::shared_ptr<std::istream> istream_t;
        typedef boost::shared_ptr<std::ostream> ostream_t;
        typedef boost::shared_ptr<std::iostream> iostream_t;

        //! when watched file changes, it will trigger this kind of signal
        //! \param string name of a stream which changed
        //! \param string partial path up to stream name
        //typedef boost::function<void (const std::string&, const std::string&)> fFileStreamChanged;

        boost::signal<void (const std::string& name, VirtualFileFactory& vff)> StreamChangedEvent;

        VirtualFileFactory() {}
        virtual ~VirtualFileFactory() {}

        /*!
        Return list of all files matching filter
        */
        virtual std::vector<std::string> GetFileList(const std::string& filter) const = 0;

        //! Return fully qualified path to this resource, otherwise return empty string
        virtual std::string GetFqn(const std::string& name) const = 0;

        //! Return name of this file factory
        virtual const char *Name() const = 0;
        //@{
        //! Return istream_t and ostream_t representing this object
        virtual istream_t GetFileStream(const std::string& name) const = 0;
        virtual ostream_t AttachFileStream(const std::string& name) = 0;
        //@}

        //! Return true if this file stream exist, otherwise false
        virtual bool IsFileStreamExist(const std::string& name) const = 0;

        //! Return TRUE if this FileFactory can provide file for writing
        virtual bool CanAttachFileStream(const std::string& name) const = 0;

        //! Watch stream for any outside changes to it and trigger fileStreamChanged notification
        //virtual bool WatchFileStream(const std::string& name, fFileStreamChanged fileStreamChanged) = 0;
        //virtual void UnwatchFileStream(const std::string& name) = 0;

        //! Return Prority level for this factory, 0xFFFFFFFF is used to specify the lowest one
        virtual uint32_t Prority() const = 0;
    };

} // namespace eg

#endif // FILE_VIRTUAL_FILE_FACTORY_H

