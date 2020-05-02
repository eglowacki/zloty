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

        //! Class to handle loading of data from disk (file access) into buffer and then calls Convertor passing buffer.
        //! It supports multiply files load requests. Internally it uses IO Completion ports. 
        //! Expect calls to Convertor/ErrorCallback to be done from different thread.
        class BlobLoader : public Noncopyable<BlobLoader>
        {
        public:
            using ErrorCallback = std::function<void(const std::string& filePathName, const std::string& errorMessage)>;
    
            BlobLoader(ErrorCallback errorCallback);
    
            using Convertor = std::function<void(const io::Buffer& fileData)>;
    
            bool Save(const io::Buffer& dataBuffer, const std::string& fileName);
            void AddTask(const Strings& fileNames, const std::vector<Convertor>& convertors);
            void AddTask(const std::string& fileName, Convertor convertor);
            size_t CurrentCounter() const { return mCounter; }
    
        private:
            // called by file loader when data is ready to be processed. This is called from different thread that this object was created on.
            void onDataPayload(const io::Buffer& dataBuffer, const std::string& fileName, Convertor convertor);// , );
    
            mt::JobPool mJobPool;
            std::unique_ptr<io::DataLoader> mFileLoader;
            ErrorCallback mErrorCallback;
            std::atomic_size_t mCounter{ 0 };
        };

    } // namespace io
} // namespace yaget
