/////////////////////////////////////////////////////////////////////////
// FileLoader.h
//
//  Copyright 7/10/2017 Edgar Glowacki.
//
// NOTES:
//      Async file loader
//
//
// #include "ThreadModel/FileLoader.h"
//
/////////////////////////////////////////////////////////////////////////
//! \file

#pragma once

#include "YagetCore.h"
#include "JobPool.h"
#include "Streams/Buffers.h"
#include <functional>
#include <memory>
#include <set>
#include <mutex>


namespace yaget
{
    class Application;

    namespace io
    {
        // Interface for getting and saving data blobs. Derived classes will provide connection to file system
        // or possibly to network/internet
        class DataLoader : public Noncopyable<DataLoader>
        {
        public:
            using DoneCallback_t = std::function<void(const io::Buffer& fileData, const std::string& fileName)>;

            void Load(const std::string& filePath, DoneCallback_t doneCallback) { Load(std::vector<std::string>{ filePath }, std::vector<DoneCallback_t>{ doneCallback }); };

            virtual void Load(const Strings& filePathList, const std::vector<DoneCallback_t>& doneCallbacks) = 0;
            virtual bool Save(const io::Buffer& dataBuffer, const std::string& fileName) = 0;
        };


        struct FileData;

        // Provides local disk file access
        class FileLoader : public DataLoader
        {
        public:
            FileLoader();
            ~FileLoader();

            void Start();

            void Load(const Strings& filePathList, const std::vector<DoneCallback_t>& doneCallbacks) override;
            bool Save(const io::Buffer& dataBuffer, const std::string& fileName) override;

            using FileDataPtr = std::unique_ptr<FileData>;

        private:
            void Stop();

            typedef void *Handle_t;
            Handle_t mIOPort = nullptr;

            // use set to quickly lookup by IO port key associated with this file request
            struct FileDataSorter
            {
                bool operator()(const FileDataPtr& lhs, const FileDataPtr& rhs) const;
            };
            using FilesToProcess = std::set<FileDataPtr, FileDataSorter>;

            FilesToProcess mFilesToProcess;
            std::mutex mListMutex;
            std::unique_ptr<mt::JobPool> mLoaderThread{ std::make_unique<mt::JobPool>("FileLoader", 1) };
            std::atomic_bool mQuit{ false };
        };

    } // namespace io
} // namespace yaget



