//////////////////////////////////////////////////////////////////////
// Aplication.h
//
//  Copyright 3/18/2018 Edgar Glowacki
//
//  Maintained by: Edgar
//
//  NOTES:
//      Load blob data from some data source (most likely from disk) async
//      It does not do any processing on data, it delivers data
//
//
//  #include "VTS/BlobLoader.h"
//
//////////////////////////////////////////////////////////////////////
//! \file

#pragma once

#include "YagetCore.h"
#include "ThreadModel/FileLoader.h"
#include <functional>


namespace yaget
{
    namespace io
    {

        //! Class to handle loading of data from from some persistent storage (like file or network) into buffer and then calls Convertor with buffer as a parameter.
        //! The actual loading and saving is done by DataLoader derived class.
        //! Expect calls to Convertor/ErrorCallback to be done from different thread.
        class BlobLoader : public Noncopyable<BlobLoader>
        {
        public:
            using ErrorCallback = std::function<void(const std::string& filePathName, const std::string& errorMessage)>;
            using Convertor = std::function<void(const io::Buffer& fileData)>;

            BlobLoader(bool loadAllFiles, ErrorCallback errorCallback);
            ~BlobLoader();

            bool Save(const io::Buffer& dataBuffer, const std::string& fileName);

            // Process all fileNames and call converter for each one. fileNames.size() == convertors.size()
            void AddTask(const Strings& fileNames, const std::vector<Convertor>& convertors);
            // Allows to have just one converter applied to all file names
            void AddTask(const Strings& fileNames, Convertor convertor);
            // Process one file and apply converter
            void AddTask(const std::string& fileName, Convertor convertor);

            size_t CurrentCounter() const { return mCounter; }

        private:
            // called by file loader when data is ready to be processed. This is called from different thread that this object was created on.
            void onDataPayload(const io::Buffer& dataBuffer, const std::string& fileName, Convertor convertor);// , );

            ErrorCallback mErrorCallback;
            std::atomic_size_t mCounter{ 0 };
            mt::JobPool mJobPool;
            std::unique_ptr<io::DataLoader> mFileLoader;
            // on destruction, if true, it will process all the files before fully exiting.
            const bool mLoadAllFiles = false;
        };

    } // namespace io
} // namespace yaget
