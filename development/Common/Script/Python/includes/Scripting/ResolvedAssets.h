//////////////////////////////////////////////////////////////////////
// ResolvedAssets.h
//
//  Copyright 4/22/2020 Edgar Glowacki
//
//  Maintained by: Edgar
//
//  NOTES:
//      missing_notes
//
//
//  #include "Scripting/ResolvedAssets.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "VTS/VirtualTransportSystem.h"
#include "App/FileUtilities.h"
#include "ImageLoaders/ImageProcessor.h"
#include "Json/JsonHelpers.h"
#include "Logger/YLog.h"


namespace yaget::io::scripting
{
    //-------------------------------------------------------------------------------------------------------------------------------
    class PythonAsset : public Asset
    {
    public:
        PythonAsset(const io::Tag& tag, io::Buffer buffer, const io::VirtualTransportSystem& vts) : Asset(tag, buffer, vts)
        {
        }
    };


    ////PixelType mColorType = PixelType::None;
    ////DataType mDataType = DataType::None;

    ////-------------------------------------------------------------------------------------------------------------------------------
    //class ImageAsset : public Asset
    //{
    //public:
    //    ImageAsset(const io::Tag& tag, io::Buffer buffer, const io::VirtualTransportSystem& vts)
    //        : Asset(tag, buffer, vts)
    //        , mHeader{ image::Header::PixelType::None, image::Header::DataType::RT }
    //        , mPixels(buffer.second ? image::Process(mBuffer, &mHeader) : io::CreateBuffer(0))
    //    {
    //        int z = 0;
    //        z;
    //    }

    //    image::Header mHeader;
    //    io::Buffer mPixels;
    //};

    ////-------------------------------------------------------------------------------------------------------------------------------
    //template<typename T>
    //inline std::shared_ptr<yaget::io::Asset> ResolveAsset(const io::Buffer& dataBuffer, const io::Tag& requestedTag, const io::VirtualTransportSystem& vts)
    //{
    //    auto asset = std::make_shared<T>(requestedTag, dataBuffer, vts);
    //    return asset->IsValid() ? asset : nullptr;
    //}

} // namespace yaget::io::scripting
