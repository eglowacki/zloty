///////////////////////////////////////////////////////////////////////
// BitmapAsset.h
//
//  Copyright 11/25/2006 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      This provides bitmap
//
//
//  #include "Asset/BitmapAsset.h"
//
//////////////////////////////////////////////////////////////////////
//! \file


#ifndef ASSET_BITMAP_ASSET_H
#define ASSET_BITMAP_ASSET_H
#pragma once

#if 0

#include "Asset/MediaAsset.h"
#include <ximage.h>
//#pragma warning (disable : 4267) // var' : conversion from 'size_t' to 'type', possible loss of data
//#include <boost/archive/binary_oarchive.hpp>
//#include <boost/archive/binary_iarchive.hpp>
//#pragma warning (default : 4267)
#include <boost/serialization/base_object.hpp>

class wxBitmap;


namespace eg
{
    //! If you use code like that below:
    //! AssetHandle<BitmapAsset> hBitmapAsset(wxT("nymphes.tga");
    //! include this in your files
    //! #include "Asset/AssetHandle.h"

    struct BitmapAssetUserData : public eg::asset::UserData
    {
        BitmapAssetUserData() : eg::asset::UserData(0), Format(0)//wxBITMAP_TYPE_INVALID)
        {
        }
        BitmapAssetUserData(size_t size) : eg::asset::UserData(size), Format(0)//wxBITMAP_TYPE_INVALID)
        {
        }

        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int /*version*/)
        {
            ar & boost::serialization::base_object<eg::asset::UserData>(*this);
            ar & Format;
        }

        long Format;
    };


    class BitmapAsset : public eg::MediaAsset<BitmapAsset, BitmapAssetUserData>
    {
    public:
        static const guid_t kType = 0x52906b71;

        BitmapAsset(const std::string& name);

        //! Return const reference to image
        wxBitmap GetBitmap() const;

        // from IAsset
        virtual guid_t Type() const {return BitmapAsset::kType;}

    private:
        // From eg::MediaAsset
        virtual U GetUserData(size_t size) const;
        virtual void SaveBlob(uint8_t *&pData, size_t& size) const;
        virtual void LoadBlob(const uint8_t *pData, size_t size, const std::string& streamName);

        mutable boost::shared_ptr<wxBitmap> mBitmap;
    };

} // namespace eg



BOOST_CLASS_VERSION(eg::BitmapAssetUserData, 1);
EG_SERIALIZE_ASSET_VERSION(eg::BitmapAsset, 1);

#endif // 0
#endif // ASSET_BITMAP_ASSET_H

