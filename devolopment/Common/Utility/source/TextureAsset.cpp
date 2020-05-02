#if 0 
#pragma warning(push)
#pragma warning (disable : 4244)  // '' : conversion from 'int' to 'unsigned short', possible loss of data
#pragma warning (disable : 4512)  // '' : assignment operator could not be generated
#include "Asset/TextureAsset.h"
#include "Message/Dispatcher.h"
#include "Registrate.h"
#include "File/VirtualFileSystem.h"
#include "File/AssetLoadSystem.h"
#pragma warning(pop)

namespace eg {

TextureAsset::TextureAsset(const std::string& name) : eg::MediaAsset<TextureAsset, TextureAssetUserData>(name)
{
}


TextureAsset::U TextureAsset::GetUserData(size_t size) const
{
    U userData(size);
    AssetLoadSystem& als = REGISTRATE(AssetLoadSystem);
    VirtualFileSystem& vfs = REGISTRATE(VirtualFileSystem);
    std::string type = vfs.GetKey(als.GetAssetName(this));
    if (type == "tga")
    {
        userData.Format = CXIMAGE_FORMAT_TGA;
    }
    else if (type == "bmp")
    {
        userData.Format = CXIMAGE_FORMAT_BMP;
    }
    else
    {
        //wxLogError("Loading image for '%s 'of '%s' type is not supported.", als.GetAssetName(this).c_str(), type.c_str());
    }

    return userData;
}


void TextureAsset::SaveBlob(uint8_t *&pData, size_t& size) const
{
    U userData = GetUserData(0);
    DataLocker lock(*this);
    mImage.Encode(pData, (long&)size, userData.Format);
}


void TextureAsset::LoadBlob(const uint8_t *pData, size_t size, const std::string& /*streamName*/)
{
    U userData = GetUserData(size);
    DataLocker lock(*this);
    mPixelData.clear();
    mImage.Decode((BYTE *)pData, static_cast<DWORD>(size), userData.Format);
}


uint8_t *TextureAsset::GetData() const
{
    DataLocker lock(*this);
    if (GetFormat() == pfRGBA)
    {
        // we have not yet created full rgba pixel data
        if (mPixelData.empty())
        {
            const uint32_t kPixelSize = 4;
            uint32_t width = GetWidth();
            uint32_t height = GetHeight();
            mPixelData.resize(width * height * kPixelSize, 0);

            for (uint32_t y = 0; y < height; y++)
            {
                uint8_t *rgb = mImage.GetBits(y);
                uint8_t *a = mImage.AlphaGetPointer(0, y);
                uint8_t *dest = &mPixelData[y * width * kPixelSize];

                for (uint32_t x = 0; x < width; x++)
                {
                    // format is bgra
                    dest[(4*x)] = rgb[3*x];
                    dest[(4*x)+1] = rgb[(3*x)+1];
                    dest[(4*x)+2] = rgb[(3*x)+2];
                    dest[(4*x)+3] = a[x];
                }
            }
        }

        return &mPixelData[0];
    }

    return mImage.GetBits();
}


void TextureAsset::SetSize(uint32_t width, uint32_t height, uint32_t bpp)
{
    DataLocker lock(*this);
    mImage.Create(width, height, bpp);
}


uint32_t TextureAsset::GetWidth() const
{
    DataLocker lock(*this);
    return mImage.GetWidth();
}


uint32_t TextureAsset::GetHeight() const
{
    DataLocker lock(*this);
    return mImage.GetHeight();
}


TextureAsset::PixelFormat TextureAsset::GetFormat() const
{
    DataLocker lock(*this);
    BYTE result = mImage.GetColorType();
    switch (result)
    {
        case 1:
            return pfLuminance;

        case 2:
            return pfRGB;

        case 4:
            return pfRGBA;
    }

    return pfNone;

}


} // namespace eg


#endif // 0