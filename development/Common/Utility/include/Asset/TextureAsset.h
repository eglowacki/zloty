///////////////////////////////////////////////////////////////////////
// TextureAsset.h
//
//  Copyright 11/25/2006 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//
//
//  #include "Asset/TextureAsset.h"
//
//////////////////////////////////////////////////////////////////////
//! \file

#ifndef ASSET_TEXTURE_ASSET_H
#define ASSET_TEXTURE_ASSET_H
#pragma once

#if 0

#include "Asset/MediaAsset.h"
#include <ximage.h>


namespace eg
{
    //! If you use code like that below:
    //! AssetHandle<TextureAsset> hTextureAsset(wxT("nymphes.tga");
    //! include this in your files
    //! #include "Asset/AssetHandle.h"

    struct TextureAssetUserData : public eg::asset::UserData
    {
        TextureAssetUserData() : eg::asset::UserData(0), Format(CXIMAGE_FORMAT_UNKNOWN)
        {
        }
        TextureAssetUserData(size_t size) : eg::asset::UserData(size), Format(CXIMAGE_FORMAT_UNKNOWN)
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


    class TextureAsset : public eg::MediaAsset<TextureAsset, TextureAssetUserData>
    {
    public:
        static const guid_t kType = 0x32744f45;

        //! Pixel Format of this image
        enum PixelFormat {pfNone, pfLuminance, pfAlpha, pfRGB, pfRGBA};

        TextureAsset(const std::string& name);

        //! Set new size for this surface, destroying anything previous there
        void SetSize(uint32_t width, uint32_t height, uint32_t bpp);
        uint32_t GetWidth() const;
        uint32_t GetHeight() const;
        PixelFormat GetFormat() const;
        //! Return pointer to flat data in memory
        uint8_t *GetData() const;

        // from IAsset
        virtual guid_t Type() const {return TextureAsset::kType;}

    private:
        // From eg::MediaAsset
        virtual U GetUserData(size_t size) const;
        virtual void SaveBlob(uint8_t *&pData, size_t& size) const;
        virtual void LoadBlob(const uint8_t *pData, size_t size, const std::string& streamName);

        mutable CxImage mImage;
        //! Since alpha is split from rgb, this will hold all togother
        //! but only if there is an alpha
        mutable std::vector<uint8_t> mPixelData;
    };

} // namespace eg



BOOST_CLASS_VERSION(eg::TextureAssetUserData, 1);
EG_SERIALIZE_ASSET_VERSION(eg::TextureAsset, 1);

#endif // 0
#endif // ASSET_TEXTURE_ASSET_H

