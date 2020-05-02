//////////////////////////////////////////////////////////////////////
// AssetHelpers.h
//
//  Copyright 12/25/2006 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      Helpers classes and functions for asset releated stuff
//
//
//  #include "Asset/AssetHelpers.h"
//
//////////////////////////////////////////////////////////////////////
//! \file

#ifndef ASSET_ASSET_HELPERS_H
#define ASSET_ASSET_HELPERS_H
#pragma once

#include "Asset/AssetHandle.h"
#include "Asset/BitmapAsset.h"
#include "File/VirtualFileSystem.h"

namespace eg
{
    //-----------------------------------------------------------------------------------------------
    //! This will make sure that image has specific size
    inline wxBitmap SetSize(const wxBitmap& bitmap, const wxSize& size)
    {
        if (bitmap.IsOk())
        {
            if (bitmap.GetWidth() != size.x || bitmap.GetHeight()!= size.y)
            {
                wxImage image = bitmap.ConvertToImage();
                return wxBitmap(image.Scale(size.x, size.y));
            }
        }

        return bitmap;
    }

    //-----------------------------------------------------------------------------------------------
    /*!
    This will create image list of icons at the specific size, scaling if necessary
    */
    class ImageList
    {
    public:
        ImageList(const wxSize& size);
        /*!
        Load all icons based on name filter (using old style dos wildcard)
        \param nameFilter file filter
        \param size size of each icon in the list. This will scale loaded image if needed
        \param defaultIcon which icon to return if there is not requested one
        */
        ImageList(const std::string& nameFilter, const wxSize& size, const std::string& defaultIcon = wxEmptyString);

        //! Add single icon
        void Add(const std::string& name);
        //! Find image index for this name. If it doe snot exist return for defaultIcon, otherwise return -1
        int FindIndex(const std::string& name) const;
        //! Full constructed and filled image list
        wxImageList *GetImageList() const;

    private:
        //! mapping from component type name icon index
        std::map<std::string, int> mIconIndex;
        wxImageList *mpImageList;
        wxSize mImageSize;
        std::string mNameFilter;
        std::string mDefaultIcon;
    };



    //-----------------------------------------------------------------------------------------------
    inline ImageList::ImageList(const wxSize& size) :
        mpImageList(new wxImageList(size.x, size.y, false)),
        mImageSize(size)
    {
    }

    //-----------------------------------------------------------------------------------------------
    inline ImageList::ImageList(const std::string& nameFilter, const wxSize& size, const std::string& defaultIcon) :
        mpImageList(new wxImageList(size.x, size.y, false)),
        mImageSize(size),
        mNameFilter(nameFilter),
        mDefaultIcon(defaultIcon)
    {
        VirtualFileSystem& vfs = REGISTRATE(VirtualFileSystem);
        std::vector<std::string> iconFiles = vfs.GetFileList(mNameFilter);
        for (std::vector<std::string>::const_iterator it = iconFiles.begin(); it != iconFiles.end(); ++it)
        {
            Add(*it);
        }
    }

    //-----------------------------------------------------------------------------------------------
    inline void ImageList::Add(const std::string& name)
    {
        typedef asset::AssetHandle<BitmapAsset, asset::AutoLock<BitmapAsset>, asset::LoadTypeBlock> BitmapAsset_t;

        BitmapAsset_t bitmap(name);
        //bitmap.Sync();
        if(BitmapAsset::Asset_t asset = bitmap())
        {
            wxBitmap bit = asset->GetBitmap();
            if (bit.IsOk())
            {
                bit = SetSize(bit, mImageSize);
                int imageIndex = mpImageList->Add(bit);
                // we need to strip 'Icon.bmp' from file name and use that as our icon index mapping
                std::string iconName = name;
                if (!mNameFilter.empty())
                {
                    iconName.erase(iconName.size() - (mNameFilter.size() - 1));
                }
                mIconIndex.insert(std::make_pair(iconName, imageIndex));
            }
        }
    }

    //-----------------------------------------------------------------------------------------------
    inline int ImageList::FindIndex(const std::string& name) const
    {
        std::map<std::string, int>::const_iterator it = mIconIndex.find(name);
        if (it != mIconIndex.end())
        {
            return (*it).second;
        }

        if (!mDefaultIcon.empty() && name != mDefaultIcon)
        {
            return FindIndex(mDefaultIcon);
        }

        return -1;
    }

    //-----------------------------------------------------------------------------------------------
    inline wxImageList *ImageList::GetImageList() const {return mpImageList;}

} // namespace eg

#endif // ASSET_ASSET_HELPERS_H

