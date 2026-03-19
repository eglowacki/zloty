///////////////////////////////////////////////////////////////////////
// RenderTextures.h
//
//  Copyright 03/14/2026 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//
//  #include "Render/Pipeline/RenderTextures.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once
#include "Render/RenderCore.h"
#include "Streams/Buffers.h"
#include "VTS/VirtualTransportSystem.h"
#include "Render/Cache/CacheWatcher.h"


namespace yaget::render
{

    //-------------------------------------------------------------------------------------------------
    class RenderTextures : public CacheWatcher<io::Buffer>
    {
    public:
        RenderTextures(io::VirtualTransportSystem& vts, io::VirtualTransportSystem::Section fileName);
        ~RenderTextures();


        io::Buffer GetTexture(const io::Tag& tag);
        std::vector<io::Buffer> GetTextures(const io::Tags& tags);

        static void PopulateMappings(io::VirtualTransportSystem::Section fileName, io::VirtualTransportSystem& vts);
        static void SaveMappings(io::VirtualTransportSystem::Section fileName, io::VirtualTransportSystem& vts);
    };

}
