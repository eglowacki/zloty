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


namespace yaget
{
    namespace image
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

    } // namespace image

} // namespace yaget

