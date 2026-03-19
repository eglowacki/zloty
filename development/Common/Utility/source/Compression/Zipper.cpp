#include "Compression/Zipper.h"
#include "zip-utils/XZip.h"
#include "zip-utils/XUnzip.h"
#include "zip-utils/XZresult.h"


namespace
{
    const char* ZipFileName = "file.dat";

    //-------------------------------------------------------------------------------------------------
    constexpr size_t CurrentZipVersion = 1;

    struct YagetZipSignature
    {
        const char Signature[4] = { 'G', 'Z', 'I', 'P' };
        size_t Version = 0;

        bool IsValidZip() const
        {
            return std::memcmp(Signature, "GZIP", 4) == 0;
        }
        bool IsValid() const
        {
            return IsValidZip() && Version <= CurrentZipVersion;
        }
    };


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

    YagetZipSignature zipSignature;
    zipSignature.Version = CurrentZipVersion;

    auto compressedBuffer = io::CreateBuffer(sizeof(YagetZipSignature) + bufferSize);
    auto dataPointer = io::cast_data<char>(compressedBuffer);
    size_t offset = 0;

    std::memcpy(dataPointer + offset, &zipSignature, sizeof(YagetZipSignature));
    offset += sizeof(YagetZipSignature);
    std::memcpy(dataPointer + offset, static_cast<uint8_t*>(bufferData), bufferSize);

    std::ignore = CloseZip(hz);

    return compressedBuffer;
}


//-------------------------------------------------------------------------------------------------
yaget::io::Buffer yaget::compression::UnzipBuffer(const io::BufferView sourceBuffer)
{
    size_t offset = 0;

    auto zipSignature = reinterpret_cast<YagetZipSignature*>(io::cast_data<char>(sourceBuffer) + offset);
    if (!zipSignature->IsValidZip())
    {
        // NOTE(eg) If this buffer is not a zip, then should we return sourceBuffer?
        YLOG_ERROR("ZIP", "Buffer is not a zip file.");
        return {};
    }

    if (!zipSignature->IsValid())
    {
        YLOG_ERROR("DEVI", "Unsupported zip buffer, version: '%d'. Expected version is <= '%d'. Cache will be ignored.",
            zipSignature->Version,
            CurrentZipVersion);

        return {};
    }

    offset += sizeof(YagetZipSignature);
    void* zippedData = static_cast<void*>(io::cast_data<uint8_t>(sourceBuffer) + offset);
    uint32_t zippedSize = static_cast<uint32_t>(io::size_data(sourceBuffer) - offset);

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
