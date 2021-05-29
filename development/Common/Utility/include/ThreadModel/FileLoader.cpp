#include "ThreadModel/FileLoader.h"
#include "Fmt/printf.h"
#include "App/AppUtilities.h"
#include "App/Application.h"
#include "Platform/WindowsLean.h"
#include "Debugging/Assert.h"
#include "Metrics/Concurrency.h"
#include "StringHelpers.h"
#include "Platform/Support.h"
#include "App/FileUtilities.h"

#include <filesystem>
namespace fs = std::filesystem;

namespace yaget::io
{
    struct FileData
    {
        static const uint32_t kStopKey = static_cast<uint32_t>(-1);
        FileData(const std::string& name, HANDLE port, yaget::io::FileLoader::DoneCallback_t doneCallback);
        FileData(uint32_t key);
        ~FileData();

        // Return true if we still need more data to process,
        // otherwise return false when we are done with results
        bool Process(uint32_t bytesCopied) const;

        std::string mName;
        HANDLE mPort = nullptr;
        HANDLE mHandle = nullptr;
        uint32_t mKey = 0;
        OVERLAPPED mOverlapped = { 0 };
        yaget::io::Buffer mDataBuffer;
        mutable uint32_t mBytesCopied = 0;
        yaget::io::FileLoader::DoneCallback_t mDoneCallback;

        yaget::metrics::TimeSpan mTimeSpan;

        static uint32_t mCounter;
    };

} // yaget

uint32_t yaget::io::FileData::mCounter = 1;


bool yaget::io::FileLoader::FileDataSorter::operator()(const FileLoader::FileDataPtr& lhs, const FileLoader::FileDataPtr& rhs) const
{
    return lhs->mKey < rhs->mKey;
}

yaget::io::FileData::FileData(const std::string& name, HANDLE port, FileLoader::DoneCallback_t doneCallback)
    : mName(name)
    , mPort(port)
    , mKey(mCounter++)
    , mDoneCallback(doneCallback)
    , mTimeSpan(yaget::meta::pointer_cast(this), "file_disk_read", YAGET_METRICS_CHANNEL_FILE_LINE)
{
    mDataBuffer = io::CreateBuffer(fs::file_size(mName));
    mHandle = ::CreateFile(mName.c_str(), FILE_READ_DATA, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, 0);
    YAGET_UTIL_THROW_ON_RROR(mHandle != INVALID_HANDLE_VALUE, fmt::format("Did not open file '{}'.", mName));

    HANDLE ioPort = ::CreateIoCompletionPort(mHandle, mPort, mKey, 0);
    YAGET_UTIL_THROW_ON_RROR(ioPort != nullptr, fmt::format("Did not create io port for file '{}'.", mName));

    bool bResult = ::ReadFile(mHandle, mDataBuffer.first.get(), static_cast<DWORD>(mDataBuffer.second), 0, &mOverlapped) != 0;
    YAGET_UTIL_THROW_ON_RROR(bResult, fmt::format("ReadFile for '{}' failed.", mName));
}

yaget::io::FileData::FileData(uint32_t key)
    : mKey(key)
    , mTimeSpan(0, "", nullptr, 0)
{
}

yaget::io::FileData::~FileData()
{
    if (mHandle)
    {
        if (mBytesCopied < mDataBuffer.second)
        {
            // we are still waiting for data, cancel our request
            bool bResult = ::CancelIoEx(mHandle, &mOverlapped) != 0;
            if (bResult)
            {
                // Wait for the I/O subsystem to acknowledge our cancellation.
                // Depending on the timing of the calls, the I/O might complete with a
                // cancellation status, or it might complete normally (if the ReadFile was
                // in the process of completing at the time CancelIoEx was called, or if
                // the device does not support cancellation).
                // This call specifies TRUE for the bWait parameter, which will block
                // until the I/O either completes or is canceled, thus resuming execution, 
                // provided the underlying device driver and associated hardware are functioning 
                // properly. If there is a problem with the driver it is better to stop 
                // responding here than to try to continue while masking the problem.

                DWORD bytesCopied = 0;
                bResult = ::GetOverlappedResult(mHandle, &mOverlapped, &bytesCopied, TRUE) != 0;
                YAGET_UTIL_THROW_ON_RROR(bResult, fmt::format("FileData GetOverlappedResult dtor failed."));
            }
        }

        bool bResult = ::CloseHandle(mHandle) != 0;
        YAGET_UTIL_THROW_ON_RROR(bResult, fmt::format("FileData CloseHandle dtor failed."));
    }
}

bool yaget::io::FileData::Process(uint32_t bytesCopied) const
{
    mBytesCopied += bytesCopied;
    YAGET_ASSERT(mBytesCopied <= mDataBuffer.second, "Received more bytes then initial file size. Total received: '%d', total allocated: '%d', last bytes: '%d'.", mBytesCopied, mDataBuffer.second, bytesCopied);
    if (mBytesCopied == mDataBuffer.second)
    {
        mTimeSpan.AddMessage("File fully loaded");
        mDoneCallback(mDataBuffer, mName);
        // we are done with data processing
        return false;
    }

    // call us again, since we did not get everything
    return true;
}

yaget::io::FileLoader::FileLoader()
    : mIOPort(::CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0))
{
    YAGET_UTIL_THROW_ON_RROR(mIOPort != nullptr, "Did not create IO Completion Port for File Loader.");
    mLoaderThread->AddTask([this]() { Start(); });
}

void yaget::io::FileLoader::Stop()
{
    mQuit = true;
    bool bResult = ::PostQueuedCompletionStatus(mIOPort, 0, io::FileData::kStopKey, nullptr) != 0; bResult;
    mLoaderThread.reset();
    std::unique_lock<std::mutex> locker(mListMutex);
    mFilesToProcess.clear();
}

yaget::io::FileLoader::~FileLoader()
{
    Stop();
    bool bResult = ::CloseHandle(mIOPort) != 0; bResult;
}

void yaget::io::FileLoader::Start()
{
    metrics::Channel channel("io.file", YAGET_METRICS_CHANNEL_FILE_LINE);

    while (true)
    {
        DWORD bytesCopied = 0;
        ULONG_PTR completionKey = 0;
        OVERLAPPED* overlappedPointer = nullptr;

        {
            metrics::Channel channel("L.Waiting...", YAGET_METRICS_CHANNEL_FILE_LINE);
            bool bResult = ::GetQueuedCompletionStatus(mIOPort, &bytesCopied, &completionKey, &overlappedPointer, INFINITE) != 0;
            if (mQuit)
            {
                break;
            }

            YAGET_UTIL_THROW_ON_RROR(bResult, fmt::format("Did not get queued io port for file to load assets from."));
            if (completionKey == io::FileData::kStopKey && bytesCopied == 0 && !overlappedPointer)
            {
                break;
            }
        }

        FileLoader::FileDataPtr fileDataQuery = std::make_unique<io::FileData>(static_cast<uint32_t>(completionKey));

        const io::FileData *nextFile = nullptr;
        {
            metrics::Channel channel("L.Finding...", YAGET_METRICS_CHANNEL_FILE_LINE);
            std::unique_lock<std::mutex> locker(mListMutex);
            FilesToProcess::iterator it = mFilesToProcess.find(fileDataQuery);
            if (it != mFilesToProcess.end())
            {
                nextFile = (*it).get();
            }
        }

        if (nextFile)
        {
            metrics::Channel span(fmt::format("Processing {} b", conv::ToThousandsSep(bytesCopied)).c_str(), YAGET_METRICS_CHANNEL_FILE_LINE);

            // if Process returns false, no need for more processing, otherwise, do not remove it from mFilesToProcess
            if (!nextFile->Process(bytesCopied))
            {
                nextFile = nullptr;

                std::unique_lock<std::mutex> locker(mListMutex);
                mFilesToProcess.erase(fileDataQuery);
            }
        }
        else
        {
            YAGET_ASSERT(false, "How this got here...");
        }
    }
}

void yaget::io::FileLoader::Load(const Strings& filePathList, const std::vector<DoneCallback_t>& doneCallbacks)
{
    YAGET_ASSERT((filePathList.size() == doneCallbacks.size()) || (filePathList.size() > 1 && doneCallbacks.size() == 1),
        "File names and doneCallbacks arrays did not match. Both must be the same size OR doneCallbacks must be 1. FileNames: '%d', DoneCallbacks: '%d'", filePathList.size(), doneCallbacks.size());

    if (!doneCallbacks.empty())
    {
        std::unique_lock<std::mutex> locker(mListMutex);
        auto callback = doneCallbacks.begin();
        const bool isOneCallback = doneCallbacks.size() == filePathList.size() ? false : true;
        for (const auto& it : filePathList)
        {
            YAGET_ASSERT(io::file::IsFileExists(it), "File: '%s' does not exist.", it.c_str());
            FileDataPtr fileData = std::make_unique<io::FileData>(it, mIOPort, *callback);

            callback = isOneCallback ? callback : ++callback;
            mFilesToProcess.insert(std::move(fileData));
        }
    }
}

bool yaget::io::FileLoader::Save(const io::Buffer& dataBuffer, const std::string& fileName)
{
    //::PostQueuedCompletionStatus(mIOPort, 0, io::FileData::kSaveFileKey, nullptr);

    const auto& [result, errorMessage] = io::file::SaveFile(fileName, dataBuffer);
    return result;
}
