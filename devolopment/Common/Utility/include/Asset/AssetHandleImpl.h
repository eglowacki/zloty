///////////////////////////////////////////////////////////////////////
// AssetHandleImpl.h
//
//  Copyright 6/16/2006 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//
//
//  #include "AssetHandleImpl.h"
//
//////////////////////////////////////////////////////////////////////
//! \file

#ifndef ASSET_ASSET_HANDLE_IMPL_H
#define ASSET_ASSET_HANDLE_IMPL_H

#ifndef ASSET_ASSET_HANDLE_INCLUDE_IMPLEMENTATION
    #error "Do not include this file explicitly."
#endif // ASSET_ASSET_HANDLE_INCLUDE_IMPLEMENTATION

namespace eg
{
    // inline implementation
    // ----------------------------------------------------------------------
    template <typename T, typename P, typename L>
    inline asset::AssetHandle<T, P, L>::AssetHandle()
    : mId(0)
    {
    }

    // ----------------------------------------------------------------------
    template <typename T, typename P, typename L>
    inline asset::AssetHandle<T, P, L>::~AssetHandle()
    {
    }

    // ----------------------------------------------------------------------
    template <typename T, typename P, typename L>
    inline asset::AssetHandle<T, P, L>::AssetHandle(uint64_t aId)
    : mId(aId)
    {
        *this = aId;
    }

    // ----------------------------------------------------------------------
    template <typename T, typename P, typename L>
    inline asset::AssetHandle<T, P, L>::AssetHandle(const std::string& resourceName)
    {
        *this = resourceName;
    }

    // ----------------------------------------------------------------------
    template <typename T, typename P, typename L>
    inline asset::AssetHandle<T, P, L>::AssetHandle(const AssetHandle<T, P, L>& source)
    {
        *this = source;
    }

    // ----------------------------------------------------------------------
    template <typename T, typename P, typename L>
    inline asset::AssetHandle<T, P, L>& asset::AssetHandle<T, P, L>::operator =(const AssetHandle<T, P, L>& source)
    {
        AssetLoadSystem& als = registrate::ref_cast<AssetLoadSystem>("AssetLoadSystem");
        std::string name = als.GetAssetName(source.mId);
        *this = name;

        return *this;
    }

    // ----------------------------------------------------------------------
    template <typename T, typename P, typename L>
    inline asset::AssetHandle<T, P, L>& asset::AssetHandle<T, P, L>::operator =(uint64_t aId)
    {
        if (T::Asset_t lockedAsset = (*this)())
        {
            AssetLoadSystem& als = registrate::ref_cast<AssetLoadSystem>("AssetLoadSystem");
            (*this) = als.GetAssetName(lockedAsset->GetId());
        }

        return *this;
    }

    // ----------------------------------------------------------------------
    template <typename T, typename P, typename L>
    inline asset::AssetHandle<T, P, L>& asset::AssetHandle<T, P, L>::operator =(const std::string& resourceName)
    {
        AssetLoadSystem& als = registrate::ref_cast<AssetLoadSystem>("AssetLoadSystem");
        if (T::Asset_t asset = als.GetAsset<T>(resourceName, L::Type(resourceName)))
        {
            mId = asset->GetId();
            mpResource = asset;
        }
        else
        {
            mpResource.reset();
            mId = 0;
        }

        return *this;
    }

    // ----------------------------------------------------------------------
    template <typename T, typename P, typename L>
    inline typename T::Asset_t asset::AssetHandle<T, P, L>::operator()()
    {
        T::Asset_t asset(P::Lock(mpResource));
        return asset;
    }

    // ----------------------------------------------------------------------
    template <typename T, typename P, typename L>
    inline void asset::AssetHandle<T, P, L>::Sync()
    {
        if (T::Asset_t asset = (*this)())
        {
            asset->Sync();
        }
    }

    // ----------------------------------------------------------------------
    template <typename T, typename P, typename L>
    inline void asset::AssetHandle<T, P, L>::Save()
    {
        if (T::Asset_t lockedAsset = (*this)())
        {
            AssetLoadSystem& als = registrate::ref_cast<AssetLoadSystem>("AssetLoadSystem");
            als.SaveAsset<T>(lockedAsset->GetId());
        }
    }

} // namespace eg

#endif // ASSET_ASSET_HANDLE_IMPL_H

