///////////////////////////////////////////////////////////////////////
// CacheWatcher.h
//
//  Copyright 01/19/2026 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//
//  #include "Render/Cache/CacheWatcher.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "AssetCache.h"
#include "Streams/Buffers.h"
#include "Streams/Watcher.h"
#include "VTS/VirtualTransportSystem.h"


namespace yaget::render
{
    //-------------------------------------------------------------------------------------------------
    class CacheWatcher
    {
    public:
        CacheWatcher(io::VirtualTransportSystem& vts, io::VirtualTransportSystem::Section fileName);
        ~CacheWatcher();

    protected:
        // will add to watch file changes if the file is not already watched
        void AssureTagWatch(const yaget::io::Tag& tag, std::function<void(const yaget::io::Tag&)> callback);
        void ClearTagWatch(const yaget::io::Tag& tag);

        io::VirtualTransportSystem& mVTS;
        yaget::render::AssetCache mCache;
        io::Watcher mWatcher;
        std::set<io::Tag> mWatchedTags;
    };

}
