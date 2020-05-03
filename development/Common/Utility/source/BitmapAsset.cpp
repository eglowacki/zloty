#if 0
#pragma warning(push)
#pragma warning (disable : 4244)  // '' : conversion from 'int' to 'unsigned short', possible loss of data
#pragma warning (disable : 4512)  // '' : assignment operator could not be generated
#pragma warning (disable : 4389)  // '!=' : signed/unsigned mismatch
#include "Asset/BitmapAsset.h"
#include "Message/Dispatcher.h"
#include "Registrate.h"
#include "File/VirtualFileSystem.h"
#include "File/AssetLoadSystem.h"
#include <wx/mstream.h>
#include <wx/gdicmn.h>
#include <wx/image.h>
#include <wx/bitmap.h>
#include <wx/object.h>
//#include <wx/wx.h>
#include <boost/algorithm/string.hpp>
#pragma warning(pop)


namespace eg {


BitmapAsset::BitmapAsset(const std::string& name) : eg::MediaAsset<BitmapAsset, BitmapAssetUserData>(name)
{
}


BitmapAsset::U BitmapAsset::GetUserData(size_t size) const
{
    U userData(size);
    AssetLoadSystem& als = registrate::ref_cast<AssetLoadSystem>("AssetLoadSystem");
    VirtualFileSystem& vfs = registrate::ref_cast<VirtualFileSystem>("VirtualFileSystem");
    std::string type = vfs.GetKey(als.GetAssetName(this));
    if (boost::algorithm::iequals(type, "tga"))
    {
        userData.Format = wxBITMAP_TYPE_TGA;
    }
    else if (boost::algorithm::iequals(type, "bmp"))
    {
        userData.Format = wxBITMAP_TYPE_BMP;
    }
    else if (boost::algorithm::iequals(type, "jpg"))
    {
        userData.Format = wxBITMAP_TYPE_JPEG;
    }
    else if (boost::algorithm::iequals(type, "ico"))
    {
        userData.Format = wxBITMAP_TYPE_ICON;
    }
    else if (boost::algorithm::iequals(type, "png"))
    {
        userData.Format = wxBITMAP_TYPE_PNG;
    }
    else
    {
        //wxLogError("Loading image for '%s 'of '%s' type is not supported.", als.GetAssetName(this).c_str(), type.c_str());
    }

    return userData;
}


void BitmapAsset::SaveBlob(uint8_t *& /*pData*/, size_t& /*size*/) const
{
    //U userData = GetUserData(0);
    //DataLocker lock(*this);
    //mImage.Encode(pData, (long&)size, userData.Format);
}


void BitmapAsset::LoadBlob(const uint8_t *pData, size_t size, const std::string& /*streamName*/)
{
    U userData = GetUserData(size);
    DataLocker lock(*this);
    wxMemoryInputStream inputStream(pData, size);
    wxImage image(inputStream, userData.Format);

    mBitmap.reset(new wxBitmap(image));
}

wxBitmap BitmapAsset::GetBitmap() const
{
    if (mBitmap)
    {
        return wxBitmap(*mBitmap.get());
    }

    return wxBitmap();
}


} // namespace eg

#endif // 0
