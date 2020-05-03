///////////////////////////////////////////////////////////////////////
// AssetHandle.h
//
//  Copyright 11/6/2005 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      Asset handle which provides opaque
//      creation, managing and destruction of asset
//
//
//  #include "Asset/AssetHandle.h"
//
//////////////////////////////////////////////////////////////////////
//! \file

#ifndef ASSET_ASSET_HANDLE_H
#define ASSET_ASSET_HANDLE_H
#pragma once

#include "Base.h"
#include "File/AssetLoadSystem.h"
#include "Hash/Hash.h"
#include "Registrate.h"

namespace eg
{
    namespace asset
    {
        //! Policy for only locking asset in object operator
        //! and then for duration of the scope of lock token (returned value from object operator)
        template <typename T> struct RequestLock
        {
            typedef boost::weak_ptr<T> Handle_t;

            static boost::shared_ptr<T> Lock(Handle_t handle)
            {
                boost::shared_ptr<T> asset(handle.lock());
                return asset;
            }
        };

        //! Lock Policy to lock asset for the entire duration
        //! of AssetHandle object
        template <typename T> struct AutoLock
        {
            typedef boost::shared_ptr<T> Handle_t;

            static boost::shared_ptr<T> Lock(Handle_t handle)
            {
                return handle;
            }
        };

        //! Policy for loading async
        struct LoadTypeAsynch
        {
            static AssetLoadSystem::Load Type(const std::string& /*assetName*/) {return AssetLoadSystem::eLoadAsync;}
        };

        //! Policy for loading to block on creation
        struct LoadTypeBlock
        {
            static AssetLoadSystem::Load Type(const std::string& /*assetName*/) {return AssetLoadSystem::eLoadBlocking;}
        };

        //! Policy for loading to decide at run time from AssetLoadSystem
        struct LoadTypeDynamic
        {
            static AssetLoadSystem::Load Type(const std::string& assetName) {return AssetLoadSystem::LoadType(assetName);}
        };


        /*!
        This represents asset. By leveraging this template
        it will manage life span for us automatically
        \param T - type of asset
        \param P - What kind of locking
                   RequestLock - user requests lock explicitly by using function object operator
                   AutoLock - lock for the entire life span of this handle
        \param L - load type, blocking, async or Dynamic
        */
        template <typename T, typename P, typename L>
        class AssetHandle
        {
        public:
            typedef typename P::Handle_t Handle_t;

            AssetHandle();
            explicit AssetHandle(const std::string& resourceName);
            explicit AssetHandle(uint64_t aid);
            ~AssetHandle();

            //! We need to call asset load system to account for our own reference
            //! since in dtor we'll call it free
            AssetHandle(const AssetHandle& source);
            AssetHandle<T, P, L>& operator =(const AssetHandle& source);

            //! This will assign new resource based on a hash
            AssetHandle<T, P, L>& operator =(uint64_t aid);

            //! This will assign new resource based on a name
            AssetHandle<T, P, L>& operator =(const std::string& resourceName);

            //! Return valid asset and locks it but in a sense to lock resource
            //! but to return shared_ptr. Depends on lock policy, this shared_ptr
            //! will be only valid for the duration of scope (if lock RequestLock)
            //! or it will always contains valid shared_ptr (if lock AutoLock)
            //! Otherwise null.
            typename T::Asset_t operator()();

            //! Sync all data for this asset. In most cases data will be raw disk file
            //! and this method will ensure that file is loaded. This is blocking call
            //! meaning it will wait until file is loaded
            void Sync();

            //! Trigger saving of this asset to persistent storage
            //! It depends on implementation specifics how if at all data is saved.
            void Save();

        private:
            //! Helper method to call asset load system to request freeing resource
            //void FreeResource();

            Handle_t mpResource;
            uint64_t mId;
        };

        //! This will make sure that asset is sync in AssetLocker class before using
        struct SyncAssetPolicy
        {
            template <typename T>
            static void Sync(T asset) {asset.Sync();}
        };

        //! Do not try to sync asset in AssetLocker. Use this only
        //! if you are sure that asset is already loaded and synced.
        struct NoSyncAssetPolicy
        {
            template <typename T>
            static void Sync(T /*asset*/) {}
        };

        //! Simplified locker class for AssetHandle
        //! \code
        //! typedef asset::AssetHandle<GeometryAsset, asset::AutoLock<GeometryAsset>, asset::LoadTypeDynamic> GeometryAsset_t;
        //! GeometryAsset_t myGeomHandle;
        //! asset::AssetLocker<GeometryAsset, asset::NoSyncAssetPolicy> geom(myGeomHandle());
        //! if (geom)
        //! {
        //!     BoundingBox_t bb = geom->BoundingBox();
        //! }
        //! \endcode
        template <typename T, typename SyncPolicy>
        class AssetLocker
        {
        public:
            typedef asset::AssetHandle<T, asset::AutoLock<T>, asset::LoadTypeBlock> T_t;

            AssetLocker(T_t assetHandle)
            : mAssetHandle(assetHandle)
            {
                SyncPolicy::Sync(mAssetHandle);
                mAsset = mAssetHandle();
                mLocker = mAsset->Lock();
            }

            AssetLocker(const std::string& assetName)
            : mAssetHandle(assetName)
            {
                SyncPolicy::Sync(mAssetHandle);
                mAsset = mAssetHandle();
                mLocker = mAsset->Lock();
            }

            typename T::Asset_t operator ->()
            {
                if (mLocker)
                {
                    return mAsset;
                }

                AssetLoadSystem& als = registrate::ref_cast<AssetLoadSystem>("AssetLoadSystem");
                std::string assetName = als.GetAssetName(mAsset ? mAsset->GetId() : 0);
                wxLogWarning("AssetHandle for '%s' is not locked, can not return asset.", assetName.c_str());
                return T::Asset_t();
            }

            operator T*() const
            {
                if (mLocker)
                {
                    return mAsset.get();
                }

                AssetLoadSystem& als = registrate::ref_cast<AssetLoadSystem>("AssetLoadSystem");
                std::string assetName = als.GetAssetName(mAsset ? mAsset->GetId() : 0);
                wxLogWarning("AssetHandle for '%s' is not locked, can not return asset.", assetName.c_str());
                return 0;
            }

        private:
            T_t mAssetHandle;
            typename T::Asset_t mAsset;
            typename T::Locker_t mLocker;
        };

		//! Simple try locker for assset
		template <typename T>
		class TryLocker
		{
		public:
			TryLocker(typename T::Asset_t asset)
				: mAsset(asset)
			{
				mLocker = mAsset ? mAsset->Lock() : T::Locker_t();
			}

			typename T::Asset_t operator ->()
			{
				return mLocker ? mAsset : T::Asset_t();
			}

			typename T::Asset_t operator*()
			{
				return mLocker ? mAsset : T::Asset_t();
			}

			operator bool() const
			{
				return mLocker ? true : false;
			}

		private:
			typename T::Asset_t mAsset;
			typename T::Locker_t mLocker;
		};


    } // namespace asset
} // namespace eg


#define ASSET_ASSET_HANDLE_INCLUDE_IMPLEMENTATION
#include "AssetHandleImpl.h"
#undef ASSET_ASSET_HANDLE_INCLUDE_IMPLEMENTATION


#endif // ASSET_ASSET_HANDLE_H

