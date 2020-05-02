///////////////////////////////////////////////////////////////////////
// VirtualFileSystem.h
//
//  Copyright 11/11/2006 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      This provides virtual file system
//
//
//  #include "File/VirtualFileSystem.h"
//
//////////////////////////////////////////////////////////////////////
//! \file

#ifndef FILE_VIRTUAL_FILE_SYSTEM_H
#define FILE_VIRTUAL_FILE_SYSTEM_H
#pragma once

#include "VirtualFileFactory.h"
#include "Exception/Exception.h"
#include "Logger/Log.h"
#include "Tree/Tree.h"
#include <map>
#pragma warning(push)
#pragma warning (disable : 4244)  // '' : conversion from '' to '', possible loss of data
#include <boost/thread/mutex.hpp>
#pragma warning(pop)


namespace eg
{
    class Dispatcher;

    //! This allows us to create configuration settings
    //! for initialization of VirtualFileSystem
    namespace config
    {
        //! This structure can be used to leverage helper function
        //! to create fully functional VirtualFileSystem.
        //! It represents one VirtualFileFactory tied to specific type.
        //! which has one or more folders to look for requested file.
        struct VirtualFileSetting
        {
            VirtualFileSetting() : priority(0xFFFFFFFF) {}
            //! File type like tga, x, fx.
            std::string type;
            //! Factory name, this is mostly user define and
            //! a way map to C++ class
            std::string factory;
            //! Where in search order this factory should be
            //! defualts to last one to search (fist add, fist search for the same tag)
            uint32_t priority;
            //! list of folder path to look in for requested files
            std::vector<std::string> folders;
        };

        typedef std::vector<VirtualFileSetting> VirtualFileCollection_t;


    } // namespace config

    /*!
    This singleton object provides loading of data independent of system and file storage.
    Opening stream for reading or writing is supported by registered factories
    for specific tag type. There can be more then one Factory for each tag type.
    First registered, first to get a chance of creating a stream. This class does not store
    created streams and for each request, it will create new stream. This could be as light
    as opening another handle to file or as heavy as creating data and allocating memory.
    */
    class VirtualFileSystem
    {
        // no copy semantics
        VirtualFileSystem(const VirtualFileSystem&);
        VirtualFileSystem& operator=(const VirtualFileSystem&);

    public:
        typedef boost::shared_ptr<VirtualFileFactory> Factory_t;

        typedef VirtualFileFactory::istream_t istream_t;
        typedef VirtualFileFactory::ostream_t ostream_t;

        typedef boost::function<void (VirtualFileSystem& vfs)> SettingCallback_t;
        VirtualFileSystem();
        VirtualFileSystem(SettingCallback_t settings);
        ~VirtualFileSystem();

        /*!
        This will return shared pointer, which will be alive for as long as client holds reference to it.
        For every call with the same name, it may or may not return new stream and it's up to implementation
        to decide how to implement it. If there is no persistent storage of this data already
        (file on disk, etc) it may return NULL.
        \param name file name of requested stream. Tag is deduced from name, which can be provided in 2 ways
               <tag_name>#<file_name.ext> or <file_name.ext>,
               tag_name - which factory will provide this stream
               file_name.ext - file name of requested stream. ext will be used for a Tag
                              if there is no #.
        \return stream ready for reading from it only
        */
        VirtualFileFactory::istream_t GetFileStream(const std::string& name) const;

        /*!
        This will return attached stream for writing to it. This will save data to
        FileFactory provided storage (disk, network drive, memory, null, etc).
        \param name file name of requested stream. Tag rules apply the same as in GetFileStream(...)
        \return stream ready for writing to it only
        */
        VirtualFileFactory::ostream_t AttachFileStream(const std::string& name);

        //! Return true if stream with name exist, false otherwise
        bool IsFileStreamExist(const std::string& name) const;

        /*!
        Query method to check if we can write file out
        \param name name of file to check if we can write to it
        \return TRUE we can, otherwise FALSE. Some FileFactories might not support writing to a file
        */
        bool CanAttachFileStream(const std::string& name) const;

        /*!
        Return list of all files matching filter. Filter should have valid key in it
        For key look up GetFileStream(...) comments
        */
        std::vector<std::string> GetFileList(const std::string& filter) const;

        /*!
        Register factory for specific key with singleton. It will take ownership of this
        factory and will delete on exit.
        \todo add sorting mechanism for factories for the same tag type
        */
        void RegisterFactory(const std::string& key, Factory_t virtualFileFactory);

        // clients can just connect and disconnect to this event/slot
        // boost::signals::connection =
        // boost::signals::scoped_connection =
        boost::signal<void (const std::string& name, VirtualFileSystem& vfs)> StreamChangedEvent;

        //! This will return key for this name (type)
        std::string GetKey(const std::string& name) const;

        //! This will return name of file without any extension or key
        std::string GetCleanName(const std::string& name) const;

        //! Return full qualified path to this stream if one is valid for this type of
        //! stream, otherwise it will return name
        std::string GetFqn(const std::string& name) const;

        //! This represent full file tree representation
        //! Root nodes will contain directory path to tag or tags
        typedef tcl::tree<std::string> FileTree_t;

        //! Return full asset tree based on filter
        //! if filter is "" then return files for all tags
        FileTree_t GetFileTree(const std::string& filter) const;

        //-------------------------------------------------------------------------------------
        //! This is only for testing where it's triggered from script at to force reloading
        void triggerFile(const std::string& name);

    private:
        // any changes to streams will trigger this callback, which in turn will signal StreamChangedEvent
        void onStreamChanged(const std::string& name, VirtualFileFactory& vff);

        //! Return valid name (stripping any # section)
        std::string normalizeName(const std::string& name) const;

        //! This will store path for each type of registered type
        //! first - registered type
        //! second object of registered path to look for asset of first type
        typedef std::map<std::string, std::vector<Factory_t> > Factories_t;
        Factories_t mRegisteredFactories;

        mutable boost::mutex mFileMutex;

        typedef std::vector<Factory_t> KeyFactories_t;
        KeyFactories_t findKeyFactories(const std::string& key) const;
    };


    namespace config
    {
        /*!
         \code
            config::VirtualFileSetting setting;
            setting.folders.push_back("c:/foo");

            setting.type = "lua";
            config::AddVirtualFile<DiskFileFactory>(vfs, setting);
          \endcode
         */

        //! Helper function to add new factory to VirtualFileSystem
        template <typename T>
        inline void AddVirtualFile(VirtualFileSystem& vfs, const VirtualFileSetting& settings)
        {
            VirtualFileSystem::Factory_t factory(new T(settings.type, settings.priority, settings.folders));
            vfs.RegisterFactory(settings.type, factory);
        }

    } // namespace config


    //@{
    //! Return file size in bytes
    size_t GetIStreamSize(std::istream& istream);
    size_t GetOStreamSize(std::ostream& ostream);
    //@}

    //! Copy input stream to output stream
    void CopyStream(std::istream& istream, std::ostream& ostream);


    namespace stream
    {
        inline size_t size(std::istream& stream) {return GetIStreamSize(stream);}

        inline size_t size(std::ostream& stream) {return GetOStreamSize(stream);}

        //! Use stream operators on std::strem objects but save and load as a binary format
        template <typename S>
        struct stream_addapter
        {
            stream_addapter(S& stream) : stream(stream)
            {}
            S& stream;

            operator S&()
            {
                return stream;
            }

            template <typename T>
            stream_addapter& operator<<(const T& v)
            {
                stream.write((const char *)&v, sizeof(T));
                return *this;
            }

            template <typename T>
            stream_addapter& operator>>(T& v)
            {
                stream.read((char *)&v, sizeof(T));
                return *this;
            }

            template <>
            stream_addapter& operator<<(const std::string& v)
            {
                size_t size = v.size();
                stream.write((const char *)&size, sizeof(size_t));
                if (size)
                {
                    stream.write((const char *)v.c_str(), size);
                }
                return *this;
            }

            template <>
            stream_addapter& operator>>(std::string& v)
            {
                const uint32_t kMaxSize = 1 * 1024 * 1024; // no more then 1Mb of string data
                size_t size = 0;
                stream.read((char *)&size, sizeof(size_t));
                if (size)
                {
                    if (size > kMaxSize)
                    {
                        throw ex::serialize("String stream operator ecounter size larger then kMaxSize.");
                    }
                    std::vector<char> buffer(size);
                    stream.read((char *)&buffer[0], size);
                    v.assign(buffer.begin(), buffer.end());
                }
                return *this;
            }
        };

        /*!
         * This class is used while saving data to stream to allow us
         * to encode size of the saved data. This is used
         * in LoadMarker which will ensure that stream position
         * will end up right after beginBlockPos + blockSize.
         */
        class SaveMarker
        {
            //! no copy semantics
            SaveMarker(const SaveMarker&);
            SaveMarker& operator=(const SaveMarker&);

        public:
            static const uint32_t kMarkerSize = 11;

            SaveMarker(std::ostream& ostream)
            : mStream(ostream)
            , mMarkerPos(ostream.tellp())
            {
                // if this is just beginning of the stream, then
                // it will return -1, but we'll set it to 0
                mMarkerPos = mMarkerPos == -1 ? 0 : mMarkerPos;
                // write out dummy marker hex value
                // which well write out the actual size of saved block
                // in dtor
                mStream.write(" 0xFFFFFFFF", kMarkerSize);
            }

            ~SaveMarker()
            {
                // get size of data just saved, so we can write it into marker pos
                int32_t currPos = mStream.tellp();
                int32_t blockSize = currPos - (mMarkerPos + kMarkerSize);

                mStream.seekp(mMarkerPos);
                // convert blockSize int hex representation
                //std::string hexNumber = boost::str(boost::format(" 0x%08X") % blockSize);
                char hexNumber[12] = {0};
                sprintf(hexNumber, " 0x%08X", blockSize);
                mStream.write(hexNumber, kMarkerSize);
                // put stream position to it's original position
                mStream.seekp(currPos);
            }

        private:
            std::ostream& mStream;
            int32_t mMarkerPos;
        };



        /*!
         * Second part of save marker. This is used when loading data
         * from stream and will assure that it will only read original
         * size of memory, regardless if under or over read.
         *
         */
        class LoadMarker
        {
            //! no copy semantics
            LoadMarker(const LoadMarker&);
            LoadMarker& operator=(const LoadMarker&);

        public:
            LoadMarker(std::istream& istream)
            : mStream(istream)
            , mBlockSize(0)
            , mBlockStartPos(0)
            {
                // read block size as string formed into hex number
                char blockSizeBuffer[SaveMarker::kMarkerSize + 1] = {0};
                mStream.read(blockSizeBuffer, SaveMarker::kMarkerSize);
                std::istringstream iss(blockSizeBuffer);
                iss >> std::hex >> mBlockSize;
                //grab begging of this stream position
                // so we can compare it on dtor to make
                // sure that only mBlockSize of data was red.
                mBlockStartPos = mStream.tellg();
            }

            ~LoadMarker()
            {
                int32_t desiredPos = mBlockStartPos + mBlockSize;
                int32_t currPos = mStream.tellg();

                if (currPos == -1 || desiredPos < currPos)
                {
                    // this is more serious here, since data was
                    // consumed more that it was originally saved.
                    if (currPos == -1)
                    {
                        mStream.clear();
                    }

                    mStream.seekg(desiredPos);

                    log_error << "More stream data (" << currPos - mBlockStartPos << ") was consumed then provided (" << mBlockSize << ") difference of (" << (currPos - mBlockStartPos)-mBlockSize << ")";
                    throw ex::serialize("Read more stream data then provided.");
                }
                else if (desiredPos > currPos)
                {
                    // not all of the data was red
                    // just issue warning and set the get cursor
                    // to desiredPos
                    log_warning << "Less stream data (" << currPos - mBlockStartPos << ") was consumed then provided (" << mBlockSize << ") difference of (" << (currPos - mBlockStartPos)-mBlockSize << ")";
                    mStream.seekg(desiredPos);
                }
            }

        private:
            std::istream& mStream;
            int32_t mBlockSize;
            int32_t mBlockStartPos;
        };


    } // namespace stream

} // namespace eg


#endif // FILE_VIRTUAL_FILE_SYSTEM_H

