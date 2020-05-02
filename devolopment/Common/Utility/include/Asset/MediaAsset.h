///////////////////////////////////////////////////////////////////////
// MediaAsset.h
//
//  Copyright 11/24/2006 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      Another helper class for creation and managing of assets
//      This represents some asset loaded from disk, like image,
//      where that image data is stored on it's own original format
//      but when we load that data into asset, it is treated as archive
//
//
//  #include "Asset/MediaAsset.h"
//
//////////////////////////////////////////////////////////////////////
//! \file

#ifndef ASSET_MEDIA_ASSET_H
#define ASSET_MEDIA_ASSET_H
#pragma once

#pragma warning (disable : 4267) // var' : conversion from 'size_t' to 'type', possible loss of data
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#pragma warning (default : 4267)
#include "Asset.h"
#include "File/VirtualFileSystem.h"
#include <boost/serialization/version.hpp>

namespace eg
{

    /*!
    This class manages all the input and output and provides needed media asset
    data in a blob format with size of blob up front
    <U = asset::UserData><blob>

    minimum implemenetation in derived class is:
    \code
    class MyAsset : public MediaAsset<MyAsset>
    {
    public:
        static const guid_t kType = 0x926f7d1f;

        MyAsset(const std::string& name) : MediaAsset<MyAsset>(name)
        {
        }

    private:
        //! look for BOOST_CLASS_VERSION below to see which version is used
        friend class boost::serialization::access;

        template<class Archive>
        void save(Archive & ar, const unsigned int version) const
        {
            DataLocker lock(*this);

            std::vector<BYTE> dataBuffer = ;  // should contain blob data
            U userData = GetUserData(dataBuffer.size());
            ar & userData;
            if (size)
            {
                ar.save_binary(&dataBuffer[0], dataBuffer.size());
            }
        }

        template<class Archive>
        void load(Archive & ar, const unsigned int version)
        {
            U userData;
            ar & userData;
            if (userData.Size)
            {
                std::vector<BYTE> dataBuffer(userData.Size);
                ar.load_binary(&dataBuffer[0], userData.Size);
                DataLocker lock(*this);
                // &dataBuffer[0] will point to blob data...
            }
        }

        BOOST_SERIALIZATION_SPLIT_MEMBER()

        // Optional
        virtual U GetUserData(size_t size) const
        {
            U userData(size);
            // add needed extra initialization here...
            return userData;
        }

    };
    \endcode
    */
    template <typename T, typename U = asset::UserData, typename I = boost::archive::binary_iarchive, typename O = boost::archive::binary_oarchive>
    class MediaAsset : public Asset<T, I, O>
    {
    public:
        typedef U U;

        MediaAsset(const std::string& name);

    protected:
        //! This returns default user data. It can be overriden in derive class to provide
        //! UserData object with extra data
        virtual U GetUserData(size_t size) const;

    private:
        friend class boost::serialization::access;
        friend Asset_t;
        //friend AssetType;

        /*!
        Called when loading data from disk
        so we need to
        * get incoming file size
        * create new stream
        * add UserData (in base it adds size of blob data)
        * copy incoming stream to our new one
        * return newly created stream
        */
        virtual VirtualFileFactory::istream_t OnAdjustDataStream(VirtualFileFactory::istream_t istream);

        //! Create new stream with our custom deleter
        virtual VirtualFileFactory::ostream_t OnAdjustDataStream(VirtualFileFactory::ostream_t ostream);

        template<class Archive>
        void save(Archive & ar, const unsigned int version) const;

        //! This gets called for you to fill pData with exact disk image
        //! of asset data
        //! \param [OUT] pData Allocate buffer with pData = new uint8_t[needed_size];
        //! \param [OUT] size needed_size to store data
        virtual void SaveBlob(uint8_t *&pData, size_t& size) const = 0;

        template<class Archive>
        void load(Archive & ar, const unsigned int version);

        //! This gets called with exact disk image of original asset
        //! \param pData pointer to falt memory data
        //! \param size size of provided data
        virtual void LoadBlob(const uint8_t *pData, size_t size, const std::string& streamName) = 0;

        //! Size of the header in front of blob data, which get's
        //! passes when writing file out to storage
        size_t mHeaderSize;

        BOOST_SERIALIZATION_SPLIT_MEMBER()
    };

} // namespace eg


BOOST_CLASS_VERSION(eg::asset::UserData, 1);


#define MEDIA_ASSET_INCLUDE_IMPLEMENTATION
#include "MediaAssetImpl.h"
#undef MEDIA_ASSET_INCLUDE_IMPLEMENTATION

#endif // ASSET_MEDIA_ASSET_H

