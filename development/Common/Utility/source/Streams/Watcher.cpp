#include "Streams/Watcher.h"
#include "Platform/Support.h"
#include "StringHelpers.h"
#include "Metrics/Gather.h"

#include "Platform/WindowsLean.h"
#include <algorithm>

namespace fs = std::filesystem;


#if YAGET_WATCHER_ENABLED == 1
    YAGET_COMPILE_GLOBAL_SETTINGS("IO Watcher Included")
#else
    YAGET_COMPILE_GLOBAL_SETTINGS("IO Watcher NOT Included")
#endif // YAGET_WATCHER_ENABLED



#if YAGET_WATCHER_ENABLED == 1

namespace
{
    // milliseconds
    const yaget::time::TimeUnits_t DefaultCleanupWait = 250;

} // namespace

yaget::io::Watcher::Watcher()
    : mObserver("io.Watcher", 1)
{
    mObserver.AddTask([this]() { Observe(); });
}


yaget::io::Watcher::~Watcher()
{
    mQuitRequested = true;
    {
        // there may be outstanding requests to remove watched file (engine shutdown may bunch up closing all threads closely together)
        //
        auto message = fmt::format("Cleaning '{}' left over file watches", GetWatchedFiles().size());
        metrics::TimeScoper<time::kMilisecondUnit> cleanupTimer(message.c_str(), YAGET_LOG_FILE_LINE_FUNCTION);
        auto endTime = platform::GetRealTime(time::kMilisecondUnit) + DefaultCleanupWait;
        platform::Sleep([this, endTime]()
        {
            if (GetWatchedFiles().empty() || endTime < platform::GetRealTime(time::kMilisecondUnit))
            {
                return false;
            }

            return true;
        });
    }

    mQuit = true;

    // we should have not mTickets here. If we do, then race condition, between waiting
    // for cleanup and quit set to true. We should generate error for this.
    YLOG_CERROR("WATC", GetWatchedFiles().empty(), "There are still '%d' files left in Watched List. [%s].", GetWatchedFiles().size(), conv::Combine(GetWatchedFiles(), "], [").c_str());
    YLOG_CERROR("WATC", static_cast<Tickets>(mTickets).empty(), "There are '%d' outstanding new tickets.", static_cast<Tickets>(mTickets).size());
}


void yaget::io::Watcher::Observe()
{
    const time::Milisecond_t interval = 100;
    time::Milisecond_t currentInterval = interval;
    Tickets watchedTickets;

    while (true)
    {
        time::Milisecond_t beginFrameTime = platform::GetRealTime(time::kMilisecondUnit);
        platform::Sleep(1, time::kMilisecondUnit);

        if (mQuit)
        {
            break;
        }

        // processed first any new incoming ticket requests
        if (Tickets peekTickets = mTickets.Swap({}); !peekTickets.empty())
        {
            for (auto& it : peekTickets)
            {
                // if we already have this file to watch we simply replace it with incoming one (only callback)
                auto existingTicket = std::find_if(std::begin(watchedTickets), std::end(watchedTickets), [ownerId = it.mOwnerId](const auto& watched)
                {
                    return watched.mOwnerId == ownerId;
                });

                // we already have this ticket, if ticket has only owner id field valid, and name and callbacks are null,
                // then it means to remove this watcher
                if (existingTicket != std::end(watchedTickets))
                {
                    if (it.mChangedCallback && !it.mFileName.empty())
                    {
                        (*existingTicket).mChangedCallback = it.mChangedCallback;
                    }
                    else
                    {
                        watchedTickets.erase(existingTicket);
                    }
                }
                else if (it.mChangedCallback && !it.mFileName.empty() && mQuitRequested == false)
                {
                    fs::path p = it.mFileName;
                    std::error_code ec;
                    it.mModTime = fs::last_write_time(p, ec);
                    if (ec)
                    {
                        YLOG_ERROR("WATC", "Could not get last write time for watched file: '%s'.", it.mFileName.c_str());
                        it.mModTime = {};
                    }

                    watchedTickets.push_back(it);
                }
            }
        }

        // on some cadence, let's check time stamps and trigger callback
        if (currentInterval <= 0 || mQuitRequested == true)
        {
            // since it may take a long time to process notifications, we want to make sure
            // that currentInterval will be above 0;
            while (currentInterval <= 0)
            {
                currentInterval += interval;
            }

            Strings watchedFiles;

            for (auto& it : watchedTickets)
            {
                if (mQuitRequested == false)
                {
                    fs::path p = it.mFileName;
                    std::error_code ec;
                    ModTime modTime = fs::last_write_time(p, ec);
                    if (ec)
                    {
                        YLOG_WARNING("WATC", "Could not get last write time for watched ticket file: '%s'.", it.mFileName.c_str());
                        modTime = {};
                    }

                    // check if file was written to it since last time we checked
                    if (modTime > it.mModTime)
                    {
                        // first, let check to see if we can open the file. There can be spurious calls to this event
                        HANDLE testFile = ::CreateFileA(it.mFileName.c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
                        if (testFile != INVALID_HANDLE_VALUE)
                        {
                            ::CloseHandle(testFile);

                            YLOG_INFO("WATC", "File: '%s' change detected.", it.mFileName.c_str());
                            it.mModTime = modTime;
                            it.mChangedCallback();
                        }
                        else
                        {
                            YLOG_INFO("WATC", "Spurious call to File: '%s', ignoring.", it.mFileName.c_str());
                            it.mModTime = modTime;
                        }

                    }
                }

                watchedFiles.push_back(it.mFileName);
            }

            std::sort(watchedFiles.begin(), watchedFiles.end());
            mWatchedFiles = watchedFiles;
        }

        time::Milisecond_t endFrameTime = platform::GetRealTime(time::kMilisecondUnit);
        currentInterval -= endFrameTime - beginFrameTime;
    }

    int zx = 0;
    zx;
}

yaget::Strings yaget::io::Watcher::GetWatchedFiles() const
{
    return mWatchedFiles;
}

void yaget::io::Watcher::Add(uint64_t ownerId, const std::string& fileName, ChangedCallback changedCallback)
{
    if (mQuit)
    {
        return;
    }

    Tickets newTickets = mTickets;
    newTickets.emplace_back(Ticket{ fileName, changedCallback, ownerId });

    mTickets = newTickets;
}

void yaget::io::Watcher::Remove(uint64_t ownerId)
{
    Add(ownerId, "", nullptr);
}

#endif // YAGET_WATCHER_ENABLED
