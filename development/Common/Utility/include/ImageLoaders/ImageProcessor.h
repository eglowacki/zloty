/////////////////////////////////////////////////////////////////////////
// ImageProcessor.h
//
//  Copyright 07/0/2018 Edgar Glowacki.
//
// NOTES:
//     Wrappers classes for loading variuos formats of image files
//
//
// #include "ImageLoaders/ImageProcessor.h"
//
/////////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "YagetCore.h"
#include "Streams/Buffers.h"
#include "VTS/VirtualTransportSystem.h"


namespace yaget::image
{
    struct Header
    {
        int mSizeX = 0;
        int mSizeY = 0;
        int mComponents = 0;    // number of components in image (rgba = 4, Single = 1)
        size_t GetImageSize() const { return static_cast<size_t>(mSizeX * mComponents * mSizeY); }
        int GetStride() const { return mSizeX * mComponents; }
        size_t GetPixelBits() const { return static_cast<size_t>(mComponents) * 8; }
    };

    enum class ImageType { PNG, JPG, BMP, TGA };

    // return pixel data converted from actual image loaded 
    // format is:
    // 1. Header with image info (pixel data size below is Header::GetImageSize())
    // 2. Pixel data in format RGBARGBA...RGBA (this is based on components in header)
    // where:
    //  1 - grey
    //  2 - grey, alpha
    //  3 - red, green, blue
    //  4 - red, green, blue, alpha
    io::Buffer GetImage(const io::VirtualTransportSystem::Section& section, io::VirtualTransportSystem& vts);
    io::Buffer GetImage(const io::Buffer& imageData);
    Header GetImageInfo(const io::Buffer& imageData);

    // Convert raw pixel data into image format ready to be saved
    // pixelData is the same format as the one returned by GetImage
    io::Buffer SaveImage(const io::Buffer& pixelData, ImageType imageType);
    io::Buffer SaveImage(const io::BufferView& pixels, const Header& header, ImageType imageType);

} // namespace yaget::image

