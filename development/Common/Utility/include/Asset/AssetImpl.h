///////////////////////////////////////////////////////////////////////
// AssetImpl.h
//
//  Copyright 11/22/2006 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//
//
//  #include "AssetImpl.h"
//
//////////////////////////////////////////////////////////////////////
//! \file

#ifndef ASSET_ASSET_IMPL_H
#define ASSET_ASSET_IMPL_H

#ifndef ASSET_ASSET_INCLUDE_IMPLEMENTATION
    #error "Do not include this file explicitly."
#endif // ASSET_ASSET_INCLUDE_IMPLEMENTATION


namespace eg
{

    namespace internal
    {
        /*!
        This is used with archive methods to allow us generate fake
        serializer header and archive value of type T. It will also trigger
        of serialization of type T, which need to provide boost::serialization
        */
        template <typename T>
        class ArchiveValue
        {
        public:
            ArchiveValue(const T& value) : mValue(value) {}
            //! Return
            const T& GetValue() const {return mValue;}

        private:
            friend class boost::serialization::access;
            template<class Archive>
            void serialize(Archive & ar, const unsigned int /*version*/)
            {
                ar & mValue;
            }

            T mValue;
        };


        /*!
        \brief Lock token to control lifespan

        This is used by derived classes to allow scoped locking of this asset data.
        It's upto derived class to decide which data to lock
        It will try to lock and return immediately.
        */
        class Locker
        {
        public:
            Locker(boost::try_mutex& mutex, bool& bLockedForAcess);
            ~Locker();
            bool IsLocked() const;

        private:
            boost::try_mutex::scoped_try_lock mLock;
            bool& mbLockedForAcess;
        };


        /*!
        This is used by derived classes to allow scoped locking of this asset data.
        It's upto derived class to decide which data to lock
        It will lock and return only after successful locking (it waits).
        */
        class WaitLocker
        {
        public:
            WaitLocker(boost::try_mutex& mutex);
            bool IsLocked() const;

        private:
            boost::try_mutex::scoped_lock mLock;
        };
    } // namespace internal


    // -------------------------------------------------------------------------
    // Locker BEGIN
    // -------------------------------------------------------------------------
    inline internal::Locker::Locker(boost::try_mutex& mutex, bool& bLockedForAcess)
    : mLock(mutex)
    , mbLockedForAcess(bLockedForAcess)
    {
        mbLockedForAcess = true;
    }

    inline internal::Locker::~Locker()
    {
        mbLockedForAcess = false;
    }

    // -------------------------------------------------------------------------
    inline bool internal::Locker::IsLocked() const
    {
        return mLock.owns_lock(); // newest drop from boost
        //return mLock.locked();
    }
    // -------------------------------------------------------------------------
    // Locker END
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    // WaitLocker BEGIN
    // -------------------------------------------------------------------------
    inline internal::WaitLocker::WaitLocker(boost::try_mutex& mutex)
    : mLock(mutex)//, boost::try_to_lock_t())// newest drop from boost
    {
    }

    // -------------------------------------------------------------------------
    inline bool internal::WaitLocker::IsLocked() const
    {
        return mLock.owns_lock(); // newest drop from boost
        //return mLock.locked();
    }
    // -------------------------------------------------------------------------
    // WaitLocker END
    // -------------------------------------------------------------------------

    // -------------------------------------------------------------------------
    // Asset BEGIN
    // -------------------------------------------------------------------------
    template <typename T, typename I, typename O>
    inline Asset<T, I, O>::Asset(const std::string& name)
    : mName(name)
    , mId(idspace::get_burnable())
    , mbReady(false)
    , mbLockedForAcess(false)
    {
        //wxLogTrace(tr_util, "Asset '%s' created.", name.c_str());
    }

    // -------------------------------------------------------------------------
    template <typename T, typename I, typename O>
    inline Asset<T, I, O>::~Asset()
    {
        internal::WaitLocker locker(mQuitMutex);
        //wxLogTrace(tr_util, "Asset '%s' destroyed.", mName.c_str());
    }

    // -------------------------------------------------------------------------
    template <typename T, typename I, typename O>
    inline uint64_t Asset<T, I, O>::GetId() const
    {
        return mId;
    }

    // -------------------------------------------------------------------------
    template <typename T, typename I, typename O>
    inline IAsset::Locker_t Asset<T, I, O>::Lock()
    {
        if (mbReady)
        {
            IAsset::Locker_t locker(new internal::Locker(mLoadDataMutex, mbLockedForAcess));
            if (locker->IsLocked())
            {
                return locker;
            }
        }

        return IAsset::Locker_t();
    }

    // -------------------------------------------------------------------------
    namespace internal
    {
        template <typename T>
        struct FinishLoading
        {

            void Finished(typename T::Asset_t asset, const std::string& name)
            {
                //wxLogTrace(tr_util, "Got Notification for '%s' asset.", name.c_str());
                mCondition.Quit();
            }

            Condition mCondition;
        };

    } // namespace internal

    // -------------------------------------------------------------------------
    template <typename T, typename I, typename O>
    inline void Asset<T, I, O>::Sync()
    {
        if (!mbReady)
        {
            internal::FinishLoading<T> finishLoading;
            boost::signals::scoped_connection c = sigLoadFinish.connect(boost::bind(&internal::FinishLoading<T>::Finished, &finishLoading, _1, _2));
            // trigger bool operator
            if (finishLoading.mCondition)
            {
            }
        }
        else
        {
            internal::WaitLocker locker(mLoadDataMutex);
        }
    }

    // -------------------------------------------------------------------------
    template <typename T, typename I, typename O>
    template <typename V>
    inline void Asset<T, I, O>::CopyArchiveValue(V value, std::ostream& ostream)
    {
        internal::ArchiveValue<V> archiveValue(value);
        SaveToArchive<O>(archiveValue, ostream);
    }

    // -------------------------------------------------------------------------
    template <typename T, typename I, typename O>
    inline VirtualFileFactory::istream_t Asset<T, I, O>::OnAdjustDataStream(VirtualFileFactory::istream_t istream)
    {
        return istream;
    }

    // -------------------------------------------------------------------------
    template <typename T, typename I, typename O>
    inline VirtualFileFactory::ostream_t Asset<T, I, O>::OnAdjustDataStream(VirtualFileFactory::ostream_t ostream)
    {
        return ostream;
    }

    // -------------------------------------------------------------------------
    template <typename T, typename I, typename O>
    inline eg::IAsset *Asset<T, I, O>::NewInstance(const std::string& name)
    {
        // we do not allow creation of assets without name
        if (!name.empty())
        {
            T *pAsset = new T(name);
            return pAsset;
        }

        return 0;
    }

    // -------------------------------------------------------------------------
    template <typename T, typename I, typename O>
    template <typename D>
    inline VirtualFileFactory::iostream_t Asset<T, I, O>::NewStream(D deleter)
    {
        typedef std::stringstream::char_type char_type;
        std::stringstream *pNewStream = new std::stringstream(std::ios_base::in|std::ios_base::out|std::stringstream::binary);
        if (typeid(deleter) == typeid(DefaultDeleter))
        {
            return VirtualFileFactory::iostream_t(pNewStream);
        }

        return VirtualFileFactory::iostream_t(pNewStream, deleter);
    }

    // -------------------------------------------------------------------------
    template <typename T, typename I, typename O>
    inline VirtualFileFactory::iostream_t Asset<T, I, O>::NewStream()
    {
        return NewStream(DefaultDeleter());
    }

    template <typename T, typename I, typename O>
    inline boost::signals::connection Asset<T, I, O>::AddLoadFinishSlot(typename const LoadFinishEvent_t::slot_type& slot)
    {
        if (mbReady)
        {
            // this is somewhat cumbersome here, since we need to pass shared_ptr to slot function,
            // we need to coerce this pointer into shared_ptr, but we can not let it delete
            // so we'll use no-op Deleter.
            //
            struct NoOpDeleter
            {
                void operator()(void *) {/*Do nothing here, since we do not want to delete pointer*/}
            };

            boost::shared_ptr<IAsset> thisAsset(this, NoOpDeleter());
            Asset_t asset(boost::dynamic_pointer_cast<T>(thisAsset));
            slot.get_slot_function()(asset, Name());
        }

        boost::mutex::scoped_lock sigLoadLocker(mSigLoadMutex);
        boost::signals::connection c = sigLoadFinish.connect(slot);
        return c;
    }


    // -------------------------------------------------------------------------
    /*static*/
    template <typename T, typename I, typename O>
    inline void Asset<T, I, O>::OnLoad(eg::AssetLoadSystem::IAsset_t asset, eg::VirtualFileFactory::istream_t istream, const std::string& streamName)
    {
        eg::VirtualFileFactory::istream_t adjusted_istream = (boost::dynamic_pointer_cast<AssetType>(asset))->OnAdjustDataStream(istream);

        Asset_t derivedAsset = boost::dynamic_pointer_cast<T>(asset);
        bool dummyBool;
        internal::Locker locker(derivedAsset->mQuitMutex, dummyBool);
        if (locker.IsLocked())
        {
            derivedAsset->mCurrentStreamName = streamName;
            if (false == (adjusted_istream && derivedAsset && eg::LoadFromArchive<I>(*derivedAsset.get(), *adjusted_istream.get())))
            {
                // if there is no istream, then that is desired result in some circumstances
                // something went wrong with loading asset from storage
                // if adjusted_istream is valid stream then loading itself went bad
                // \note add exception handling here to make sure that loading can not
                // crash program
                if (adjusted_istream)
                {
                    // there was loading data error
                    //wxLogError("Loading data for '%s' asset failed.", derivedAsset->Name().c_str());
                }
            }

            // We are going to set ready flag to TRUE, whether loading was successful or not,
            // to make this asset into initial ready state. This will only happen
            // if this asset is wild filter and this is the last pass of filtered files,
            // or this is just single asset.
            if ((IsWildString(derivedAsset->Name()) && !istream) || !IsWildString(derivedAsset->Name()))
            {
                boost::mutex::scoped_lock sigLoadLocker(derivedAsset->mSigLoadMutex);
                derivedAsset->mbReady = true;
                derivedAsset->sigLoadFinish(derivedAsset, derivedAsset->mCurrentStreamName);
            }
        }
    }

    // -------------------------------------------------------------------------
    /*static*/
    template <typename T, typename I, typename O>
    inline bool Asset<T, I, O>::OnSave(eg::AssetLoadSystem::IAsset_t asset, eg::VirtualFileFactory::ostream_t ostream)
    {
        eg::VirtualFileFactory::ostream_t adjusted_ostream = boost::dynamic_pointer_cast<AssetType>(asset)->OnAdjustDataStream(ostream);

        Asset_t derivedAsset = boost::dynamic_pointer_cast<T>(asset);
        bool dummyBool;
        internal::Locker locker(derivedAsset->mQuitMutex, dummyBool);
        if (locker.IsLocked())
        {
            if ((adjusted_ostream && derivedAsset && eg::SaveToArchive<O>(*derivedAsset.get(), *adjusted_ostream.get())) == false)
            {
                // if there is no ostream, then that is desired result in some circumstances
                if (!adjusted_ostream)
                {
                    // there was saving data error
                    //wxLogError("Saving data for '%s' asset failed.", derivedAsset->Name().c_str());
                    return false;
                }
            }

            return true;
        }

        //wxLogError("Could not lock '%s' asset to save.", derivedAsset->Name().c_str());
        return false;
    }
    // -------------------------------------------------------------------------
    // Asset END
    // -------------------------------------------------------------------------

} // namespace eg

#endif // ASSET_ASSET_IMPL_H

