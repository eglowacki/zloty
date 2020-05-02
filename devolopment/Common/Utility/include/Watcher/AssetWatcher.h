///////////////////////////////////////////////////////////////////////
// AssetWatcher.h
//
//      Copyright 11/4/2006 Edgar Glowacki.
//
//      Maintained by: Edgar
//
//  NOTES:
//      Object which allows us ot watch specific files for changes
//      and call slot on event
//
//
//  #include "Watcher/AssetWatcher.h"
//
//////////////////////////////////////////////////////////////////////
//! \file

#ifndef WATCHER_ASSET_WATCHER_H
#define WATCHER_ASSET_WATCHER_H
#pragma once

#include "Synchronization/JobQueue.h"
#include <istream>
#include <boost/shared_ptr.hpp>
#pragma warning (disable : 4512)  // '' : assignment operator could not be generated
    #include <boost/signals.hpp>
#pragma warning(pop)


namespace eg
{
    namespace asset
    {
        //! This represents our asset tag which was changed
        struct Tag
        {
            Tag();
            Tag(const std::string& name, const std::string& key);
            //! Return true if both mName and mKey are same with source
            bool IsSame(const Tag& source) const;
            bool operator==(const Tag& source) const;
            bool operator!=(const Tag& source) const;

            std::string mName; ///< name of this asset, including any extension
            std::string mKey;  ///< key of storage type. This can be file path, zip, network, etc
            boost::shared_ptr<std::istream> mData;
        };

    } // namespace asset

    class AssetWatcher// : protected JobQueue<asset::Tag>
    {
    public:
        AssetWatcher();
        virtual ~AssetWatcher();
        //! Stop watching any files and quit watching loop
        void Stop();

        //! This allows to add which file we are interested in
        void AddFile(const std::string& fileName);
        //! Remove file from watch
        void RemoveFile(const std::string& fileName);

        /*!
        This signal is triggered every time there is a file ready to be updated.
        This will be called in the context of this object thread
        */
        boost::signal<void (asset::Tag)> sigAssetReady;

    private:
        // from JobQueue
        virtual void OnExit();
        // ! Called by FileWatcher for every file changed
        void OnFileChange(const std::string& fileName, const std::string& key);
        //! Called by this thread when asset is ready to be updated
        void OnAssetUpdate(asset::Tag tag);
        //! Called for every asset to check if its ready to be updated
        bool OnAssetReady(asset::Tag& tag);
        //FileWatcher mFileWatcher;
    };


    inline asset::Tag::Tag()
    {
    }

    inline asset::Tag::Tag(const std::string& name, const std::string& key) :
        mName(name),
        mKey(key)
    {
    }

    inline bool asset::Tag::IsSame(const asset::Tag& source) const
    {
        return mName == source.mName && mKey == source.mKey;
    }

    inline bool asset::Tag::operator==(const asset::Tag& source) const
    {
        return IsSame(source);
    }

    inline bool asset::Tag::operator!=(const asset::Tag& source) const
    {
        return !IsSame(source);
    }



} // namespace eg

#endif // WATCHER_ASSET_WATCHER_H

