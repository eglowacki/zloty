#include "ThreadModel/FileLoader.h"

#include <comdef.h>

#include "fmt/printf.h"
#include "App/AppUtilities.h"
#include "App/Application.h"
#include "Platform/WindowsLean.h"
#include "Debugging/Assert.h"
#include "Metrics/Concurrency.h"
#include "StringHelpers.h"
#include "Platform/Support.h"
#include "App/FileUtilities.h"

#include <filesystem>
#include <xutility>

#include "Core/ErrorHandlers.h"
namespace fs = std::filesystem;

namespace yaget::io
{
    //-------------------------------------------------------------------------------------------------
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


uint32_t yaget::io::FileData::mCounter = 0;

//-------------------------------------------------------------------------------------------------
bool yaget::io::FileLoader::FileDataSorter::operator()(const FileLoader::FileDataPtr& lhs, const FileLoader::FileDataPtr& rhs) const
{
    return lhs->mKey < rhs->mKey;
}


//-------------------------------------------------------------------------------------------------
yaget::io::FileData::FileData(const std::string& name, HANDLE port, FileLoader::DoneCallback_t doneCallback)
    : mName(name)
    , mPort(port)
    , mKey(++mCounter)
    , mDoneCallback(std::move(doneCallback))
    , mTimeSpan(yaget::meta::pointer_cast(this), fs::path(name).filename().generic_string())
{
    mDataBuffer = io::CreateBuffer(fs::file_size(mName));
    mHandle = ::CreateFile(mName.c_str(), FILE_READ_DATA, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, nullptr);
    error_handlers::ThrowOnError(mHandle != INVALID_HANDLE_VALUE, fmt::format("Did not open file '{}'.", mName));

    const HANDLE ioPort = ::CreateIoCompletionPort(mHandle, mPort, mKey, 0);
    error_handlers::ThrowOnError(ioPort != nullptr, fmt::format("Did not create io port for file '{}'.", mName));

    const bool bResult = ::ReadFile(mHandle, mDataBuffer.first.get(), static_cast<DWORD>(mDataBuffer.second), nullptr, &mOverlapped) != 0;
    error_handlers::ThrowOnError(bResult, fmt::format("ReadFile for '{}' failed.", mName));
}


//-------------------------------------------------------------------------------------------------
yaget::io::FileData::FileData(uint32_t key)
    : mKey(key)
    , mTimeSpan(0, "")
{
}


//-------------------------------------------------------------------------------------------------
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

                //DWORD bytesCopied = 0;
                //ULONG_PTR completionKey = 0;
                //OVERLAPPED* overlappedPointer = nullptr;
                //const bool bResult = ::GetQueuedCompletionStatus(mPort, &bytesCopied, &completionKey, &overlappedPointer, INFINITE) != 0;
                //error_handlers::ThrowOnError(bResult, fmt::format("FileData CancelIoEx dtor failed."));
            }
        }

        const bool bResult = ::CloseHandle(mHandle) != 0;
        error_handlers::ThrowOnError(bResult, fmt::format("FileData CloseHandle dtor failed."));
    }
}


//-------------------------------------------------------------------------------------------------
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


//-------------------------------------------------------------------------------------------------
yaget::io::FileLoader::FileLoader()
    : mIOPort(::CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0))    // note: last param represents how many threads to create for io (0 os is managing)
{
    error_handlers::ThrowOnError(mIOPort != nullptr, "Did not create IO Completion Port for File Loader.");
    mLoaderThread->AddTask([this]() { StartLoader(); });
}


//-------------------------------------------------------------------------------------------------
void yaget::io::FileLoader::StopLoader()
{
    mQuit = true;
    [[maybe_unused]] bool bResult = ::PostQueuedCompletionStatus(mIOPort, 0, io::FileData::kStopKey, nullptr) != 0;
    mLoaderThread.reset();
    metrics::UniqueLock locker(mListMutex, "Stop Loader");
    mFilesToProcess.clear();
}


//-------------------------------------------------------------------------------------------------
yaget::io::FileLoader::~FileLoader()
{
    StopLoader();
    [[maybe_unused]] bool bResult = ::CloseHandle(mIOPort) != 0;
}


//-------------------------------------------------------------------------------------------------
void yaget::io::FileLoader::StartLoader()
{
    metrics::Channel channel("io.file");

    [[maybe_unused]] constexpr bool tryNewOverlap = true;
    while (true)
    {
        std::vector<std::pair<ULONG_PTR, DWORD>> completionKeys;

        {
            constexpr int entriesSize = 20;
            OVERLAPPED_ENTRY overlappedEntries[entriesSize] = {};
            ULONG numEntriesRemoved = 0;

            metrics::Channel channelStatus("L.Waiting...");
            const bool bResult = ::GetQueuedCompletionStatusEx(mIOPort, &overlappedEntries[0], entriesSize, &numEntriesRemoved, INFINITE, false) != 0;
            if (mQuit)
            {
                return;
            }

            error_handlers::ThrowOnError(bResult, fmt::format("Did not get queued io port for file to load assets from."));

            if (numEntriesRemoved)
            {
                for (ULONG i = 0; i < numEntriesRemoved; ++i)
                {
                    const auto& entry = overlappedEntries[i];

                    if (entry.lpCompletionKey == io::FileData::kStopKey && entry.dwNumberOfBytesTransferred == 0)
                    {
                        return;
                    }

                    completionKeys.push_back({ entry.lpCompletionKey, entry.dwNumberOfBytesTransferred });
                }


                channel.AddMessage(fmt::format("Got '{}' files.", numEntriesRemoved), metrics::MessageScope::Thread);
            }
        }

        for (const auto& key : completionKeys)
        {
            if (mQuit)
            {
                return;
            }

            FileLoader::FileDataPtr fileDataQuery = std::make_unique<io::FileData>(static_cast<uint32_t>(key.first));

            const io::FileData *nextFile = nullptr;
            {
                metrics::UniqueLock locker(mListMutex, "Files To Process");
                FilesToProcess::iterator it = mFilesToProcess.find(fileDataQuery);
                if (it != mFilesToProcess.end())
                {
                    nextFile = (*it).get();
                }
            }

            if (nextFile)
            {
                metrics::Channel span(fmt::format("Processing {} b", conv::ToThousandsSep(key.second)));

                // if Process returns false, no need for more processing, otherwise, do not remove it from mFilesToProcess
                if (!nextFile->Process(key.second))
                {
                    nextFile = nullptr;

                    metrics::UniqueLock locker(mListMutex, "Erase File");
                    mFilesToProcess.erase(fileDataQuery);
                }
            }
            else
            {
                YAGET_ASSERT(false, "How this got here...");
            }
        }
    }
}


//-------------------------------------------------------------------------------------------------
void yaget::io::FileLoader::Load(const Strings& filePathList, const std::vector<DoneCallback_t>& doneCallbacks)
{
    YAGET_ASSERT((filePathList.size() == doneCallbacks.size()) || (filePathList.size() > 1 && doneCallbacks.size() == 1),
        "File names and doneCallbacks arrays did not match. Both must be the same size OR doneCallbacks must be 1. FileNames: '%d', DoneCallbacks: '%d'", filePathList.size(), doneCallbacks.size());

    if (!doneCallbacks.empty())
    {
        metrics::Channel channel(fmt::format("FileLoader got '{}' files", filePathList.size()));

        auto callback = doneCallbacks.begin();
        const bool isOneCallback = doneCallbacks.size() == filePathList.size() ? false : true;
        for (const auto& it : filePathList)
        {
            YAGET_ASSERT(io::file::IsFileExists(it), "File: '%s' does not exist.", it.c_str());

            metrics::UniqueLock locker(mListMutex, "Adding File");
            FileDataPtr fileData = std::make_unique<io::FileData>(it, mIOPort, *callback);
            mFilesToProcess.insert(std::move(fileData));

            callback = isOneCallback ? callback : ++callback;
        }
    }

    mLoaderThread->UnpauseAll();
}


//-------------------------------------------------------------------------------------------------
bool yaget::io::FileLoader::Save(const io::Buffer& dataBuffer, const std::string& fileName)
{
    //::PostQueuedCompletionStatus(mIOPort, 0, io::FileData::kSaveFileKey, nullptr);

    const auto& [result, errorMessage] = io::file::SaveFile(fileName, dataBuffer);
    return result;
}


//-------------------------------------------------------------------------------------------------
yaget::io::NetworkLoader::NetworkLoader()
{
}


//-------------------------------------------------------------------------------------------------
yaget::io::NetworkLoader::~NetworkLoader()
{
}


//-------------------------------------------------------------------------------------------------
void yaget::io::NetworkLoader::Load(const Strings& /*filePathList*/, const std::vector<DoneCallback_t>& /*doneCallbacks*/)
{
}


//-------------------------------------------------------------------------------------------------
bool yaget::io::NetworkLoader::Save(const io::Buffer& /*dataBuffer*/, const std::string& /*fileName*/)
{
    return false;
}
