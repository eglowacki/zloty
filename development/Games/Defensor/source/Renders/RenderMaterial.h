///////////////////////////////////////////////////////////////////////
// RenderMaterial.h
//
//  Copyright 01/30/2026 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//
//  #include "Renders/RenderMaterial.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "VTS/VirtualTransportSystem.h"


namespace defensor::render
{
    using namespace yaget;

    //-------------------------------------------------------------------------------------------------
    class RenderMaterial
    {
    public:
        using Section = io::VirtualTransportSystem::Section;

        RenderMaterial(Section materialSection, io::VirtualTransportSystem& vts);
        ~RenderMaterial();

    private:
        Section mMaterialSection;
        io::VirtualTransportSystem& mVTS;
    };

}
