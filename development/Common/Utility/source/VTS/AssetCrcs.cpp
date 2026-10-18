#include "VTS/AssetCrcs.h"
#include "Streams/Populators.h"


//-------------------------------------------------------------------------------------------------------------------------------
yaget::io::crc::AssetsData::AssetsData(io::VirtualTransportSystem& vts, io::VirtualTransportSystem::Section manifestSection, io::VirtualTransportSystem::Section vtsSection)
    : mVTS(vts)
    , mManifestSection(manifestSection)
    , mVTSSection(vtsSection)
{
    CrcCaches crcDiskCaches;
    PopulateMap(mManifestSection, mVTS, crcDiskCaches);
    auto crcVtsCaches = CalculateVTSCrcs(io::VirtualTransportSystem::CrcMode::ClearLoaded);

    // Determine which cached "blob" assets currently known to the VirtualTransportSystem (VTS)
    // are invalid compared to the on-disk cache manifest, remove those invalid blobs from VTS,
    Tags invalidCaches = crcVtsCaches | std::views::filter([&crcDiskCaches](const auto& crcValue)
    {
        return !crcDiskCaches.contains(crcValue.first) || crcDiskCaches.at(crcValue.first) != crcValue.second;
    }) | std::views::transform([this](const auto& crcValue)
    {
        const auto cacheFileTag = mVTS.FindTag(crcValue.first);

        return cacheFileTag;
    }) | std::ranges::to<std::vector>();

    mVTS.DeleteBlobs(invalidCaches);
}


//-------------------------------------------------------------------------------------------------------------------------------
yaget::io::crc::AssetsData::~AssetsData()
{
    auto crcCaches = CalculateVTSCrcs(io::VirtualTransportSystem::CrcMode::KeepLoaded);

    SaveMap(mManifestSection, mVTS, crcCaches);
}


//-------------------------------------------------------------------------------------------------------------------------------
yaget::io::crc::AssetsData::CrcCaches yaget::io::crc::AssetsData::CalculateVTSCrcs(io::VirtualTransportSystem::CrcMode crcMode) const
{
    using Section = io::VirtualTransportSystem::Section;

    auto tags = mVTS.GetTags(mVTSSection);
    auto crcs = mVTS.GetCrcs(tags, crcMode);

    CrcCaches crcCaches;
    for (const auto& [tag, crcValue] : std::views::zip(tags, crcs))
    {
        crcCaches.insert({ tag.mGuid, crcValue });
    }

    return crcCaches;
}
