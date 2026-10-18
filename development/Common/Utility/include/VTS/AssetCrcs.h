//////////////////////////////////////////////////////////////////////
// AssetCrcs.h
//
//  Copyright 10/18/2026 Edgar Glowacki
//
//  Maintained by: Edgar
//
//  NOTES:
//      Provides support for vts files to calculate CRC and delete file if manifest
//      does not match. Manifest file is updated on exit. 
//
//
//  #include "VTS/AssetCrcs.h"
//
//////////////////////////////////////////////////////////////////////
//! \file

#pragma once

#include "VTS/VirtualTransportSystem.h"


namespace yaget::io::crc
{
    //-------------------------------------------------------------------------------------------------------------------------------
    class AssetsData
    {
    public:
        AssetsData(io::VirtualTransportSystem& vts, io::VirtualTransportSystem::Section manifestSection, io::VirtualTransportSystem::Section vtsSection);
        ~AssetsData();

    private:
        using CrcCaches = std::map<yaget::Guid, uint32_t>;

        CrcCaches CalculateVTSCrcs(io::VirtualTransportSystem::CrcMode crcMode) const;
        
        io::VirtualTransportSystem& mVTS;
        VirtualTransportSystem::Section mManifestSection;
        VirtualTransportSystem::Section mVTSSection;
    };
    
}
