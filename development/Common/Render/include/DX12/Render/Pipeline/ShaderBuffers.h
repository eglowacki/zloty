///////////////////////////////////////////////////////////////////////
// ShaderBuffers.h
//
//  Copyright 03/17/2026 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//
//  #include "Render/Pipeline/ShaderBuffers.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once


#include "RenderShaders.h"
#include "VTS/VirtualTransportSystem.h"


namespace yaget::render
{
    class ConstantBuffer;

    namespace platform
    {
        class Adapter;
    }

    //--------------------------------------------------------------------------------------------------
    class ShaderBuffers
    {
    public:
        ShaderBuffers(const platform::Adapter& adapter, io::VirtualTransportSystem& vts, io::VirtualTransportSystem::Section fileName);
        ~ShaderBuffers();

        void MakeBuffers(const io::Tag& tag, const RenderShaders::IndexMap& indexMap);
        ConstantBuffer* GetBuffer(const io::Tag& tag);


    private:
        const platform::Adapter& mAdapter;

        using BuffersMap = std::map<io::Tag, std::shared_ptr<ConstantBuffer>>;
        BuffersMap mBuffersMap;
    };
}
