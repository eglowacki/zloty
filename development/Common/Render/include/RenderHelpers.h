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

        void ThrowOnError(long hr, const std::string& message, const char* file = nullptr, unsigned line = 0);

        namespace draw
        {
            void BoundingBox(const math::Box& box, render::LineComponent* lineComponent);

        } // namespace draw

    } // namespace render
} // namespace yaget


#define YAGET_THROW_ON_RROR(hr, message) yaget::render::ThrowOnError(hr, message, __FILE__, __LINE__)
#define YAGET_SET_DEBUG_NAME(d3dData, message) yaget::render::SetDebugName(d3dData, message)
