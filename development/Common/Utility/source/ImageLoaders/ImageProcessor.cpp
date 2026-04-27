#include "ImageLoaders/ImageProcessor.h"
#include "VTS/ResolvedAssets.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"


namespace
{
    void image_write_func(void* context, void* data, int size)
    {
        auto contextBuffer = static_cast<yaget::io::MessagingBuffer*>(context);
        contextBuffer->AssureWriteSize(size);
        contextBuffer->WriteDataChunk(data, size);
    }
}


//-------------------------------------------------------------------------------------------------
yaget::io::Buffer yaget::image::GetImage(const io::VirtualTransportSystem::Section& section, io::VirtualTransportSystem& vts)
{
    io::SingleBLobLoader<io::BinAsset> loader(vts, section);
    if (auto asset = loader.GetAsset(); asset->IsValid())
    {
        const auto& imageBuffer = asset->mBuffer;
        return GetImage(imageBuffer);
    }

    return {};
}


//-------------------------------------------------------------------------------------------------
yaget::io::Buffer yaget::image::GetImage(const io::Buffer& imageData)
{
    if (auto header = GetImageInfo(imageData); header.GetImageSize())
    {
        int requiredComp = header.mComponents == 3 ? 4 : header.mComponents;
        int x = 0;
        int y = 0;
        int comp = 0;
        if (auto imageMemory = stbi_load_from_memory(io::cast_data<const stbi_uc>(imageData), io::size_data<io::Buffer, int>(imageData), &x, &y, &comp, requiredComp))
        {
            header = { .mSizeX = x, .mSizeY = y, .mComponents = requiredComp };
            auto imagePixels = io::CreateBuffer(header.GetImageSize() + sizeof(header));

            size_t writeOffset = 0;
            std::memcpy(io::cast_data<char>(imagePixels) + writeOffset, &header, sizeof(header));
            writeOffset += sizeof(header);
            std::memcpy(io::cast_data<char>(imagePixels) + writeOffset, imageMemory, header.GetImageSize());

            stbi_image_free(imageMemory);

            return imagePixels;
        }
    }

    return {};
}


//-------------------------------------------------------------------------------------------------
yaget::image::Header yaget::image::GetImageInfo(const io::Buffer& imageData)
{
    int x = 0;
    int y = 0;
    int comp = 0;
    if (stbi_info_from_memory(io::cast_data<const stbi_uc>(imageData), io::size_data<io::Buffer, int>(imageData), &x, &y, &comp))
    {
        return { .mSizeX = x, .mSizeY = y, .mComponents = comp };
    }

    return {};
}


//-------------------------------------------------------------------------------------------------
yaget::io::Buffer yaget::image::SaveImage(const io::Buffer& pixelData, ImageType imageType)
{
    const auto header = io::cast_data<Header>(pixelData);
    const auto pixels = io::cast_to_view(pixelData, sizeof(Header));
    return SaveImage(pixels, *header, imageType);
}


//-------------------------------------------------------------------------------------------------
yaget::io::Buffer yaget::image::SaveImage(const io::BufferView& pixels, const Header& header, ImageType imageType)
{
    io::MessagingBuffer imageBuffer;

    switch (imageType)
    {
    case ImageType::PNG:
        {
            stbi_write_png_to_func(&image_write_func, &imageBuffer, header.mSizeX, header.mSizeY, header.mComponents, io::cast_data<void>(pixels), header.GetStride());
        }
        break;
    case ImageType::JPG:
        {
            stbi_write_jpg_to_func(&image_write_func, &imageBuffer, header.mSizeX, header.mSizeY, header.mComponents, io::cast_data<void>(pixels), 100);
        }
        break;
    case ImageType::BMP:
        {
            stbi_write_bmp_to_func(&image_write_func, &imageBuffer, header.mSizeX, header.mSizeY, header.mComponents, io::cast_data<void>(pixels));
        }
        break;
    case ImageType::TGA:
        {
            stbi_write_tga_to_func(&image_write_func, &imageBuffer, header.mSizeX, header.mSizeY, header.mComponents, io::cast_data<void>(pixels));
        }
        break;
    default:
        YLOG_ERROR("IMG", "Unsupported image type provided for saving image data.");
        break;
    }

    auto result = imageBuffer.mBuffer;
    result.second = imageBuffer.mWriteOffset;
    return result;
}
