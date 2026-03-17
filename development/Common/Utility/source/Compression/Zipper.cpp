#include "Compression/Zipper.h"
#include "zip-utils/XZip.h"
#include "zip-utils/XUnzip.h"
#include "zip-utils/XZresult.h"


//   HZIP hz = CreateZip(0,100000, 0);
//   // adding a conventional file...
//   ZipAdd(hz,"src1.txt",  "c:\\src1.txt");
//   // adding something from memory...
//   char buf[1000]; for (int i=0; i<1000; i++) buf[i]=(char)(i&0x7F);
//   ZipAdd(hz,"file.dat",  buf,1000);
//   // adding something from a pipe...
//   HANDLE hread,hwrite; CreatePipe(&hread,&hwrite,NULL,0);
//   HANDLE hthread = CreateThread(0,0,ThreadFunc,(void*)hwrite,0,0);
//   ZipAdd(hz,"unz3.dat",  hread,1000);  // the '1000' is optional.
//   WaitForSingleObject(hthread,INFINITE);
//   CloseHandle(hthread); 
//   CloseHandle(hread);
//   ... meanwhile DWORD WINAPI ThreadFunc(void *dat)
//                 { HANDLE hwrite = (HANDLE)dat;
//                   char buf[1000]={17};
//                   DWORD writ; WriteFile(hwrite,buf,1000,&writ,NULL);
//                   CloseHandle(hwrite);
//                   return 0;
//                 }
//   // and now that the zip is created, let's do something with it:
//   void *zbuf; 
//   unsigned long zlen; 
//   ZipGetMemory(hz,&zbuf, &zlen);

//----------------------------------------------------------------------------------------------
//   HZIP hz = OpenZip(zipbuf, ziplen, 0);
//     - unzip to a membuffer -
//   ZIPENTRY ze; int i; FindZipItem(hz,"file.dat",true,&i,&ze);
//   char *ibuf = new char[ze.unc_size];
//   UnzipItem(hz,i, ibuf, ze.unc_size);
//   delete[] ibuf;



namespace
{
    const char* ZipFileName = "file.dat";


    //-------------------------------------------------------------------------------------------------
    std::string GetErrorMessage(ZRESULT result)
    {
        auto errorMessageSize = FormatZipMessage(result, nullptr, 0) + 1;
        auto messageBuffer = std::make_unique<char[]>(errorMessageSize);
        FormatZipMessage(result, messageBuffer.get(), errorMessageSize);
        return messageBuffer.get();
    }

}


//-------------------------------------------------------------------------------------------------
yaget::io::Buffer yaget::compression::ZipBuffer(const io::BufferView sourceBuffer)
{
    size_t multiplier = 10;
    const uint32_t initialSize = static_cast<uint32_t>(io::size_data(sourceBuffer) * multiplier);

    auto hz = CreateZip(nullptr, initialSize, nullptr);
    if (!hz)
    {
        YLOG_ERROR("ZIP", "Could not create zip buffer with size: '%d'", initialSize);
        return {};
    }

    void* sourceData = static_cast<void*>(io::cast_data<uint8_t>(sourceBuffer));
    uint32_t sourceSize = static_cast<uint32_t>(io::size_data(sourceBuffer));
    auto result = ZipAdd(hz, ZipFileName, sourceData, sourceSize);
    if (result != ZR_OK)
    {
        YLOG_ERROR("ZIP", "Could not add buffer to zip. Error: '%s'", GetErrorMessage(result).c_str());
        return {};
    }

    void* bufferData;// = nullptr;
    unsigned long bufferSize;
    result = ZipGetMemory(hz, &bufferData, &bufferSize);
    if (result != ZR_OK)
    {
        YLOG_ERROR("ZIP", "Could not get zipped buffer from zip. Error: '%s'", GetErrorMessage(result).c_str());
        return {};
    }

    auto compressedBuffer = io::CreateBuffer(static_cast<uint8_t*>(bufferData), bufferSize);
    std::ignore = CloseZip(hz);

    return compressedBuffer;
}


//-------------------------------------------------------------------------------------------------
yaget::io::Buffer yaget::compression::UnzipBuffer(const io::BufferView sourceBuffer)
{
    void* zippedData = static_cast<void*>(io::cast_data<uint8_t>(sourceBuffer));
    uint32_t zippedSize = static_cast<uint32_t>(io::size_data(sourceBuffer));

    HZIP hz = OpenZip(zippedData, zippedSize, nullptr);
    if (!hz)
    {
        YLOG_ERROR("ZIP", "Could not open zip buffer of size: '%d'.", zippedSize);
        return {};
    }

    ZIPENTRY zipEntry; 
    int zipIndex = -1; 
    auto result = FindZipItem(hz, ZipFileName, true, &zipIndex, &zipEntry);
    if (result != ZR_OK)
    {
        YLOG_ERROR("ZIP", "Could not find zipped entry: '%s'. Error: '%s'", ZipFileName, GetErrorMessage(result).c_str());
        return {};
    }

    auto resultBuffer = io::CreateBuffer(static_cast<size_t>(zipEntry.unc_size));
    void* destData = static_cast<void*>(io::cast_data<uint8_t>(resultBuffer));
    result = UnzipItem(hz, zipIndex, destData, zipEntry.unc_size);
    if (result != ZR_OK)
    {
        YLOG_ERROR("ZIP", "Could not unzip buffer. Error: '%s'", GetErrorMessage(result).c_str());
        return {};
    }

    std::ignore = CloseZip(hz);

    return resultBuffer;
}
