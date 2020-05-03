#include "VTS/BlobLoader.h"
#include "Exception/Exception.h"
#include "Fmt/ostream.h"
#include "Metrics/Concurrency.h"
#include "Platform/Support.h"
#include "Debugging/DevConfiguration.h"
#include "Metrics/Concurrency.h"

#include <filesystem>
namespace fs = std::filesystem;


yaget::io::BlobLoader::BlobLoader(ErrorCallback errorCallback)
    : mJobPool("BlobLoader", dev::CurrentConfiguration().mDebug.mThreads.Blob)
    , mFileLoader(std::make_unique<io::FileLoader>())
    , mErrorCallback(errorCallback ? errorCallback : [](const std::string&, const std::string&) {})
{}


void yaget::io::BlobLoader::AddTask(const Strings& fileNames, const std::vector<Convertor>& convertors)
{
    mCounter += fileNames.size();

    std::vector<io::DataLoader::DoneCallback_t> adjustedConverters;
    auto fn = fileNames.begin();
    for (const auto& it : convertors)
    {
        auto func = [this, it](auto&& param1, auto&& param2) { onDataPayload(param1, param2, it); };
        adjustedConverters.push_back(func);
    }

    mFileLoader->Load(fileNames, adjustedConverters);
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
        metrics::Channel channel(fs::path(fileName).filename().generic_string().c_str(), YAGET_METRICS_CHANNEL_FILE_LINE);

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
