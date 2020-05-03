//////////////////////////////////////////////////////////////////////
// Watcher.h
//
//  Copyright 8/4/2019 Edgar Glowacki
//
//  Maintained by: Edgar
//
//  NOTES:
//      
//
//
//  #include "Streams/Watcher.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "YagetCore.h"
#include "ThreadModel/JobPool.h"
#include "ThreadModel/Variables.h"
#include <functional>
#include <filesystem>


namespace yaget
{
    namespace io
    {
#if YAGET_WATCHER_ENABLED == 1

        // Provides simple "polling" for file changes. 
        // If file was modified, calls ChangedCallback
        class Watcher : public Noncopyable<Watcher>
        {
        public:
            using ChangedCallback = std::function<void()>;

            Watcher();
            ~Watcher();

            void Add(uint64_t ownerId, const std::string& fileName, ChangedCallback changedCallback);
            void Remove(uint64_t ownerId);

            Strings GetWatchedFiles() const;

        private:
            void Observe();

            using ModTime = std::filesystem::file_time_type;
            struct Ticket
            {
                std::string mFileName;
                ChangedCallback mChangedCallback;
                uint64_t mOwnerId;
                ModTime mModTime{};
            };

            std::atomic_bool mQuit{ false };
            std::atomic_bool mQuitRequested{ false };

            using Tickets = std::vector<Ticket>;
            mt::Variable<Tickets> mTickets;
            mt::Variable<Strings> mWatchedFiles;    // used bu imgui to display which one are watched and provide user trigger
            mt::JobPool mObserver;
        };

#else

        class Watcher : public Noncopyable<Watcher>
        {
        public:
            using ChangedCallback = std::function<void()>;

            Watcher() {}
            ~Watcher() {}

            void Add(uint64_t /*ownerId*/, const std::string& /*fileName*/, ChangedCallback /*changedCallback*/) {}
            void Remove(uint64_t /*ownerId*/) {}
        };

#endif // YAGET_WATCHER_ENABLED
        
    } // namespace io
} // namespace yaget
