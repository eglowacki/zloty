#include "VTS/BlobLoader.h"
#include "Debugging/DevConfiguration.h"
#include "Exception/Exception.h"
#include "Metrics/Concurrency.h"
#include "Platform/Support.h"

#include <filesystem>
namespace fs = std::filesystem;


yaget::io::BlobLoader::BlobLoader(bool loadAllFiles, ErrorCallback errorCallback)
    : mErrorCallback(errorCallback ? errorCallback : [](const std::string&, const std::string&) {})
    , mJobPool("BlobLoader", dev::CurrentConfiguration().mDebug.mThreads.Blob)
    , mFileLoader(std::make_unique<io::FileLoader>())
    , mLoadAllFiles(loadAllFiles)
{}


yaget::io::BlobLoader::~BlobLoader()
{
    if (mLoadAllFiles)
    {
        const double MaxTimeToWait = 5.0;

        double startDestroyTime = platform::GetRealTime();
        platform::Sleep([this, startDestroyTime, MaxTimeToWait]()
        {
            const auto counter = CurrentCounter();
                const double nowTime = platform::GetRealTime();
            if (nowTime - startDestroyTime > MaxTimeToWait)
            {
                YAGET_ASSERT(false, "Waiting for BlobLoader to finish on files: '%d'.", counter);
                return false;
            }

            return counter > 0;
        });
    }
}


void yaget::io::BlobLoader::AddTask(const Strings& fileNames, const std::vector<Convertor>& convertors)
{
    YAGET_ASSERT_ERROR((fileNames.size() == convertors.size()) || (fileNames.size() > 1 && convertors.size() == 1),
        "File names and converters arrays did not match. Both must be the same size OR converter must be 1. FileNames: '%d', Converters: '%d'", fileNames.size(), convertors.size());

    mCounter += convertors.empty() ? 0 : fileNames.size();

    std::vector<io::DataLoader::DoneCallback_t> adjustedConverters;
    for (const auto& it : convertors)
    {
        auto func = [this, it](auto&& param1, auto&& param2) { onDataPayload(param1, param2, it); };
        adjustedConverters.push_back(func);
    }

    mFileLoader->Load(fileNames, adjustedConverters);
}


void yaget::io::BlobLoader::AddTask(const Strings& fileNames, Convertor convertor)
{
    AddTask(fileNames, std::vector<Convertor>{ convertor });
}


void yaget::io::BlobLoader::AddTask(const std::string& fileName, Convertor convertor)
{
    AddTask(std::vector<std::string>{ fileName }, std::vector<Convertor>{ convertor });
}


void yaget::io::BlobLoader::onDataPayload(const io::Buffer& dataBuffer, const std::string& fileName, Convertor convertor)
{
    // Since actual processing of the buffer data might take a while, we farm that to another thread, which in turn will call convertor from that thread
    mJobPool.AddTask([this, dataBuffer, fileName, convertor]()
    {
        metrics::Channel channel(fs::path(fileName).filename().generic_string());

        try
        {
            convertor(dataBuffer);
        }
        catch (const yaget::ex::standard& e)
        {
            std::string message = fmt::format("Data Stream '{}' did not get converted. '{}'.", fileName.c_str(), e.what());
            mErrorCallback(fileName, message);
        }

        mCounter--;
    });
}


bool yaget::io::BlobLoader::Save(const io::Buffer& dataBuffer, const std::string& fileName)
{
    return mFileLoader->Save(dataBuffer, fileName);
}
