///////////////////////////////////////////////////////////////////////
// Asset.h
//
//  Copyright 11/22/2006 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      This is only used in helping to implemented asset derived classes
//      and this provides hash and name value, serializing and locking data
//
//
//  #include "Asset/Asset.h"
//
//////////////////////////////////////////////////////////////////////
//! \file Asset.h
//! \attention This file is only used for developers deriving new classes from IAsset interface,
//! which adds support for serialization using boost::archive serialization library.

#ifndef ASSET_ASSET_H
#define ASSET_ASSET_H
#pragma once

#include "IAsset.h"
#include "File/AssetLoadSystem.h"
#include "File/ArchiveHelpers.h"
#include "Registrate.h"
#include "Synchronization/ConditionObject.h"
#include "IdGameCache.h"
#include "StringHelper.h"
#include <sstream>
#include <boost/thread/mutex.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/signal.hpp>



namespace eg
{
    /*!
    Use this to automatically register all the needed callbacks.
    This assumes that MyAsset is derived from eg::Asset<TextureAsset> class
    \code
    AutoRegisterAssetFactory<MyAsset> myFactory;
    \endcode
    */
    template <typename T>
    struct AutoRegisterAssetFactory
    {
        AutoRegisterAssetFactory()
        {
            if (AssetLoadSystem *als = registrate::p_cast<AssetLoadSystem>("AssetLoadSystem"))
            {
                als->RegisterFactory(T::kType, T::NewInstance, T::OnLoad, T::OnSave);
            }
        }

        ~AutoRegisterAssetFactory()
        {
            if (AssetLoadSystem *als = registrate::p_cast<AssetLoadSystem>("AssetLoadSystem"))
            {
                als->UnregisterFactory(T::kType);
            }
        }
    };

    /*!
    use this in cpp file to add versioning into your class
    which is derived from Asset
    \code
    EG_SERIALIZE_ASSET_VERSION(MyAsset, 1);
    \endcode
    */
    #define EG_SERIALIZE_ASSET_VERSION(classType, versionNumber) \
        BOOST_CLASS_VERSION(classType, versionNumber) \
        BOOST_CLASS_VERSION(eg::internal::ArchiveValue<classType::U>, versionNumber)


    /*!
    \page AssetTutorialPage Asset Tutorial
    Basic skeleton for concrete implementation using this helper template is:
    \code
    #include <boost/archive/binary_oarchive.hpp>
    #include <boost/archive/binary_iarchive.hpp>

    class MyAsset : public eg::Asset<MyAsset>
    {
    public:
        typedef asset::UserData U;
        static const guid_t kType = <unique 32 bit guid>;

        MyAsset(const std::string& name) : eg::Asset<MyAsset>(name),
            mNum(0)
        {
        }

        // MyAsset specific methods
        // ...

    private:
        //! look for BOOST_CLASS_VERSION below to see which version is used
        friend class boost::serialization::access;

        // Split version of serialize methods
        template<class Archive>
        void save(Archive & ar, const unsigned int version) const
        {
            DataLocker lock(*this);
            ar & mNum;
        }

        template<class Archive>
        void load(Archive & ar, const unsigned int version)
        {
            DataLocker lock(*this);
            ar & mNum;
        }

        BOOST_SERIALIZATION_SPLIT_MEMBER()

        // Or single version of serialize method
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version)
        {
            DataLocker lock(*this);
            ar & mNum;
        }

        // Dummy variable showing saving and loading
        int mNum;
    };

    // outside any namespace
    EG_SERIALIZE_ASSET_VERSION(MyAsset, 1);
    \endcode
    */

    namespace asset
    {
        // Used as a defualt
        struct UserData
        {
            UserData() : Size(0) {}
            UserData(size_t size) : Size(size) {}

            friend class boost::serialization::access;
            template<class Archive>
            void serialize(Archive & ar, const unsigned int /*version*/) {ar & Size;}

            size_t Size;
        };


    } // namespace asset


    /*!
    Do not cast any IAssets to this helper Asset template
    To easy use of IAsset concrete implementation, derive from this class.
    For example see page \ref AssetTutorialPage "Asset Tutorial".
    */
    template <typename T, typename I = boost::archive::binary_iarchive, typename O = boost::archive::binary_oarchive>
    class Asset : public IAsset
    {
        // no copy semantics
        Asset(const Asset&);
        Asset& operator =(const Asset&);

    public:
        typedef boost::shared_ptr<T> Asset_t;

        //! Create new instance of this asset.
        Asset(const std::string& name);
        virtual ~Asset();

        // from IAsset
        virtual uint64_t GetId() const;
        virtual Locker_t Lock();
        virtual const std::string& Fqn() const {static std::string garbage; return garbage;}

        //! Triggered after loading is done
        //! Asset_t this object
        //! std::string name of the this object
        typedef boost::signal<void (Asset_t, const std::string&)> LoadFinishEvent_t;
        LoadFinishEvent_t sigLoadFinish;

        //! Connect slot to sigLoadFinish. If asset is ready it will still connect and trigger the slot
        //! Otherwise it will only connect. Slot may be triggered on a different thread,
        //! depends on what loading policy was specified.
        boost::signals::connection AddLoadFinishSlot(typename const LoadFinishEvent_t::slot_type& slot);

        //! Makes sure that all the data is loaded. This is blocking call until it finishes
        void Sync();

    protected:
        //@{
        //! Alias to archive types
        typedef I I;
        typedef O O;
        //@}
        //! Alias to derived class type. Used only to allows us to access private methods
        //! from within templatized code
        typedef Asset<T, I, O> AssetType;

        //! This is default deleter when one is not specified by the user
        struct DefaultDeleter
        {
            void operator()(void *p) {delete p;}
        };

        //! Helper method to return new io stream
        template <typename D>
        static VirtualFileFactory::iostream_t NewStream(D deleter);
        //! Wrapper method for the above one to allow defualt deleter to be used
        static VirtualFileFactory::iostream_t NewStream();

        //! Copy value of type T to ostream using archive support. This will add archive header.
        template <typename V>
        static void CopyArchiveValue(V value, std::ostream& ostream);

        /*!
        Used by derived classes to lock data for reading and/or writing
        Usage:
        \code
        DataLocker locker(*this);
        \endcode
        */
        struct DataLocker
        {
            DataLocker(const T& asset)
            : mLock(asset.mLoadDataMutex)
            {;}

        private:
            boost::try_mutex::scoped_lock mLock;
        };

        /*
        //! Use this whne you need to just use resource
        //! but can deal with when resource is null.
        // //! IOW, it uses try lock, so it does not block
        struct DataUser
        {
            DataUser(const T& asset)
            : mLock(asset.mLoadDataMutex)
            {;}

            bool locked() const
            {
                return mLock.locked();
            }

        private:
            boost::try_mutex::scoped_try_lock mLock;
        };
        */

        //! This might go away in the future, do not relay on it
        virtual const std::string& Name() const {return mName;}
        inline const std::string& streamName() const {return mCurrentStreamName;}
        inline std::string getAssetKey() const
        {
            std::string type = REGISTRATE(VirtualFileSystem).GetKey(Name());
            return type
        }

        bool isLocked() const { return mbLockedForAcess;}

    private:
        friend AutoRegisterAssetFactory<T>;

        //@{
        //! Derived class can adjust stream anyway it sees fit. including creating new stream
        //! In a defualt implementation it returns unmodified parameter <i|o>stream
        virtual VirtualFileFactory::istream_t OnAdjustDataStream(VirtualFileFactory::istream_t istream);
        virtual VirtualFileFactory::ostream_t OnAdjustDataStream(VirtualFileFactory::ostream_t ostream);
        //@}

        //! This are callbacks used for Factory to create, load and save asset
        static eg::IAsset *NewInstance(const std::string& name);
        static void OnLoad(eg::AssetLoadSystem::IAsset_t asset, eg::VirtualFileFactory::istream_t istream, const std::string&);
        static bool OnSave(eg::AssetLoadSystem::IAsset_t asset, eg::VirtualFileFactory::ostream_t ostream);

        std::string mName;
        uint64_t mId;
        volatile bool mbReady;
        //! \note this should start in non signal state
        //! and turn to signalled when finished loading or saving (will need to think about if mutex is needed for saving)
        //! can we use
        mutable boost::try_mutex mLoadDataMutex;
        mutable boost::try_mutex mQuitMutex;
        //! Used when we attaching to FinishedLoad signal
        mutable boost::mutex mSigLoadMutex;
        std::string mCurrentStreamName;

        //! This is just helper for derive classes to sanity check if user locked this asset
        //! before accessing any methods
        bool mbLockedForAcess;
    };


} // namespace eg


#define ASSET_ASSET_INCLUDE_IMPLEMENTATION
#include "AssetImpl.h"
#undef ASSET_ASSET_INCLUDE_IMPLEMENTATION

#endif // ASSET_ASSET_H

