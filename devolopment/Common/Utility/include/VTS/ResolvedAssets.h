//////////////////////////////////////////////////////////////////////
// ResolvedAssets.h
//
//  Copyright 6/24/2018 Edgar Glowacki
//
//  Maintained by: Edgar
//
//  NOTES:
//      Contains derived classes from io::Asset for resolver support
//
//
//  #include "VTS/ResolvedAssets.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "VTS/VirtualTransportSystem.h"
#include "App/FileUtilities.h"
#include "ImageLoaders/ImageProcessor.h"
#include "Json/JsonHelpers.h"
#include "Logger/YLog.h"


namespace yaget::io
{
    //-------------------------------------------------------------------------------------------------------------------------------
    class JsonAsset : public Asset
    {
    public:
        JsonAsset(const io::Tag& tag, io::Buffer buffer, const io::VirtualTransportSystem& vts) : Asset(tag, buffer, vts)
        {
            // We want ability to create instance of json asset type without actual data buffer. This is in essence a default ctor.
            if (mBuffer.second)
            {
                file::imemstream memStream(reinterpret_cast<const char*>(mBuffer.first.get()), mBuffer.second);

                try
                {
                    memStream >> root;
                }
                catch (const nlohmann::detail::exception& ex)
                {
                    YLOG_ERROR("ASET", "Unable to process json stream: '%s' with size: '%d'. Error: %s.", tag.ResolveVTS().c_str(), mBuffer.second, ex.what());
                    mValid = false;
                }
            }
        }

        nlohmann::json root;
    };


    //PixelType mColorType = PixelType::None;
    //DataType mDataType = DataType::None;

    //-------------------------------------------------------------------------------------------------------------------------------
    class ImageAsset : public Asset
    {
    public:
        ImageAsset(const io::Tag& tag, io::Buffer buffer, const io::VirtualTransportSystem& vts)
            : Asset(tag, buffer, vts) 
            , mHeader{ image::Header::PixelType::None, image::Header::DataType::RT }
            , mPixels(buffer.second ? image::Process(mBuffer, &mHeader) : io::CreateBuffer(0))
        {
            int z = 0;
            z;
        }

        image::Header mHeader;
        io::Buffer mPixels;
    };

    //-------------------------------------------------------------------------------------------------------------------------------
    template<typename T>
    inline std::shared_ptr<yaget::io::Asset> ResolveAsset(const io::Buffer& dataBuffer, const io::Tag& requestedTag, const io::VirtualTransportSystem& vts)
    {
        auto asset = std::make_shared<T>(requestedTag, dataBuffer, vts);
        return asset->IsValid() ? asset : nullptr;
    }

} // namespace yaget::io
