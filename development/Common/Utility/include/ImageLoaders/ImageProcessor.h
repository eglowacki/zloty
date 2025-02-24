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


namespace yaget::image
{
    //--------------------------------------------------------------------------------------------------
    struct Header
    {
        using Size = std::pair<uint32_t, uint32_t>;
        enum class PixelType { None, Single, RGB, RGBA };
        enum class DataType { None, PNG, RAW, DDS, RT };

        PixelType mColorType = PixelType::None;
        DataType mDataType = DataType::None;
        uint32_t mBitDepth = 0;         // for now supports only 8 bits per color
        uint32_t mNumComponents = 0;    // rgba = 4, Single = 1
        Size mSize;
        size_t mNumMipMaps = 0;         // number of mipmaps included in this image

        uint32_t PixelSize() const { return mNumComponents; }
    };

    //! Return image info data
    Header GetHeader(const io::Buffer& buffer);

    // returned processed imaged data in buffer into raw linear format RGBARGBA...RGBA
    io::Buffer Process(const io::Buffer& buffer, Header* header);

    //typedef enum LodePNGColorType
    //{
    //    LCT_GREY = 0, /*greyscale: 1,2,4,8,16 bit*/
    //    LCT_RGB = 2, /*RGB: 8,16 bit*/
    //    LCT_PALETTE = 3, /*palette: 1,2,4,8 bit*/
    //    LCT_GREY_ALPHA = 4, /*greyscale with alpha: 8,16 bit*/
    //    LCT_RGBA = 6 /*RGB with alpha: 8,16 bit*/
    //} LodePNGColorType;
    using pixel_byte = unsigned char;
    bool EncodeSave(const std::string& filename, const std::vector<pixel_byte>& in, uint32_t w, uint32_t h, int colortype = 6 /*LodePNGColorType LCT_RGBA*/, uint32_t bitdepth = 8);

} // namespace yaget::image

