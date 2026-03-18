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
    namespace platform
    {
        class Adapter;
    }

    // Just a placeholder for return value from GetBuffer
    struct ConstantData {};

    //--------------------------------------------------------------------------------------------------
    class ConstantBuffers
    {
    public:
        ConstantBuffers(const platform::Adapter& adapter, io::VirtualTransportSystem& vts, io::VirtualTransportSystem::Section fileName);
        ~ConstantBuffers();

        void MakeBuffers(const io::Tag& tag, const RenderShaders::IndexMap& indexMap);
        ConstantData GetBuffer(const io::Tag& tag);


    private:
        const platform::Adapter& mAdapter;
    };
}
