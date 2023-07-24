#include "ImageLoaders/ImageProcessor.h"
#include "VTS/ResolvedAssets.h"
#include "Debugging/Assert.h"
#include "lodepng.h"
//#include <DirectXTex.h>

namespace
{
    //--------------------------------------------------------------------------------------------------
    bool IsPNG(const yaget::io::Buffer& buffer)
    {
        uint32_t width = 0, height = 0;
        lodepng::State imageState;
        uint32_t result = lodepng_inspect(&width, &height, &imageState, buffer.first.get(), buffer.second);
        return result == 0;
    }

    struct DDSHeader
    {
        uint32_t MagicValue;
        uint32_t Header;
    };

    bool IsDDS(const yaget::io::Buffer& buffer)
    {
        const DDSHeader ddsHeader{ 0x20534444, 124 };
        const DDSHeader* bufferHeader = reinterpret_cast<const DDSHeader*>(buffer.first.get());

        return ddsHeader.MagicValue == bufferHeader->MagicValue && ddsHeader.Header == bufferHeader->Header;
    }

    //--------------------------------------------------------------------------------------------------
    LodePNGColorType ConvertFrom(yaget::image::Header::PixelType pixelType)
    {
        switch (pixelType)
        {
        case yaget::image::Header::PixelType::RGBA:
            return LCT_RGBA;
            break;

        case yaget::image::Header::PixelType::Single:
            return LCT_GREY;
            break;
        }

        return LCT_RGBA;
    }

} // namespace


//--------------------------------------------------------------------------------------------------
yaget::image::Header yaget::image::GetHeader(const io::Buffer& /*buffer*/)
{
    YAGET_UTIL_THROW("REND", "DX11 DDS disabeld for now...");
    image::Header header;

#if 0 // disable this code for now to move it to renderer

    if (IsDDS(buffer))
    {
        DirectX::TexMetadata metadata;
        HRESULT hr = DirectX::GetMetadataFromDDSMemory(buffer.first.get(), buffer.second, 0, metadata);
        YAGET_UTIL_THROW_ON_RROR(hr, "Could not load DDS header/metadata.");

        header.mDataType = Header::DataType::DDS;
        header.mSize = std::make_pair(static_cast<uint32_t>(metadata.width), static_cast<uint32_t>(metadata.height));
        header.mNumMipMaps = static_cast<uint32_t>(metadata.mipLevels);

        header.mBitDepth = static_cast<uint32_t>(DirectX::BitsPerPixel(metadata.format));
        header.mNumComponents = header.mBitDepth / static_cast<uint32_t>(DirectX::BitsPerColor(metadata.format));

        switch (header.mNumComponents)
        {
        case 1:
            header.mColorType = image::Header::PixelType::Single;
            break;

        case 3:
            header.mColorType = image::Header::PixelType::RGBA;
            break;

        case 4:
            header.mColorType = image::Header::PixelType::RGBA;
            break;

        default:
            YAGET_UTIL_THROW_ASSERT("REND", false, fmt::format("Image Colortype: '{}' does not supported format: '{}'.", static_cast<int>(header.mColorType), static_cast<int>(metadata.format)));
        }
    }
    else if (IsPNG(buffer))
    {
        uint32_t width = 0, height = 0;

        lodepng::State imageState;
        uint32_t result = lodepng_inspect(&width, &height, &imageState, buffer.first.get(), buffer.second);
        YAGET_ASSERT(!result, "Did not load header png stream. %s.", lodepng_error_text(result));

        header.mDataType = Header::DataType::PNG;
        header.mSize = std::make_pair(width, height);

        switch (imageState.info_png.color.colortype)
        {
        case LCT_RGB:
        case LCT_RGBA:
            header.mColorType = image::Header::PixelType::RGBA;
            header.mBitDepth = imageState.info_png.color.bitdepth;
            header.mNumComponents = 4;
            break;

        case LCT_GREY:
            header.mColorType = image::Header::PixelType::Single;
            header.mBitDepth = imageState.info_png.color.bitdepth;
            header.mNumComponents = 1;
            break;

        case LCT_PALETTE:
            header.mColorType = image::Header::PixelType::RGBA;
            header.mBitDepth = 8;
            header.mNumComponents = 4;
            break;

        default:
            YAGET_ASSERT(false, "Image Colortype: '%d' is not supported.", imageState.info_png.color.colortype);
        }
    }
#endif

    return header;
}


//--------------------------------------------------------------------------------------------------
yaget::io::Buffer yaget::image::Process(const io::Buffer& buffer, Header* header)
{
    io::Buffer processedData;
    image::Header currentHeader = image::GetHeader(buffer);

    if (IsDDS(buffer))
    {
        processedData = buffer;
    }
    else if (IsPNG(buffer))
    {
        unsigned char* out = nullptr;
        uint32_t width = 0, height = 0;
        currentHeader.mDataType = image::Header::DataType::RAW;

        uint32_t result = lodepng_decode_memory(&out, &width, &height, buffer.first.get(), buffer.second, ConvertFrom(currentHeader.mColorType), currentHeader.mBitDepth);
        YAGET_ASSERT(!result, "Did not load processed png stream. %s.", lodepng_error_text(result));

        processedData.first = std::shared_ptr<uint8_t>(out, [](uint8_t* b)
        {
            free(b);
        });

        processedData.second = currentHeader.mSize.first * currentHeader.mSize.second * currentHeader.mNumComponents;
    }

    if (header)
    {
        *header = currentHeader;
    }

    return processedData;
}

bool yaget::image::EncodeSave(const std::string& filename, const std::vector<pixel_byte>& in, uint32_t w, uint32_t h, int colortype /*= 6 LodePNGColorType LCT_RGBA*/, uint32_t bitdepth /*= 8*/)
{
    if (filename.empty())
    {
        return false;
    }

    auto result = lodepng_encode_file(filename.c_str(), in.data(), w, h, static_cast<LodePNGColorType>(colortype), bitdepth);

    return result == 0;
}
