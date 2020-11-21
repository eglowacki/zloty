/////////////////////////////////////////////////////////////////////////
// RenderHelper.h
//
//  Copyright 7/26/2016 Edgar Glowacki.
//
// NOTES:
//      Common functions and utilities for render
//
//
// #include "RenderHelper.h"
//
/////////////////////////////////////////////////////////////////////////
//! \file

#pragma once

#include "YagetCore.h"

struct ID3D11DeviceChild;
struct ID3D11Device;

namespace yaget
{
    namespace math { struct Box; }

    namespace render
    {
        void SetDebugName(ID3D11DeviceChild* d3dData, const std::string& message);
        void SetDebugName(ID3D11Device* d3dData, const std::string& message);

        class LineComponent;

        namespace draw
        {
            void BoundingBox(const math::Box& box, render::LineComponent* lineComponent);

        } // namespace draw

    } // namespace render
} // namespace yaget


#define YAGET_SET_DEBUG_NAME(d3dData, message) yaget::render::SetDebugName(d3dData, message)
