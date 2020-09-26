/////////////////////////////////////////////////////////////////////////
// Primitives.h
//
//  Copyright 8/12/2016 Edgar Glowacki.
//
// NOTES:
//      Expose helper methods to render various 2D constructs
//
//
// #include "Debugging/Primitives.h"
//
/////////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "YagetCore.h"
#include "MathFacade.h"

namespace yaget
{
    namespace render
    {
        class Device;

        namespace primitives
        {
            struct Line
            {
                math3d::Vector3 p0;
                math3d::Vector3 p1;
                colors::Color c0;
                colors::Color c1;
            };

            using Lines = std::vector<Line>;
            using LinesPtr = std::shared_ptr<Lines>;

            void Mark(const math3d::Vector3& location, const colors::Color& color, float magnitude, Lines& lines);
            inline Lines Mark(const math3d::Vector3& location, const colors::Color& color, float magnitude)
            {
                Lines lines;
                Mark(location, color, magnitude, lines);
                return lines;
            }

            void Outline(float width, float height, const colors::Color& color, Lines& lines);
            inline Lines Outline(float width, float height, const colors::Color& color)
            {
                Lines lines;
                Outline(width, height, color, lines);
                return lines;
            }

            void Rectangle(const math3d::Vector3& upperLeft, const math3d::Vector3& lowerRight, const colors::Color& color, Lines& lines);
            inline Lines Rectangle(const math3d::Vector3& upperLeft, const math3d::Vector3& lowerRight, const colors::Color& color)
            {
                Lines lines;
                Rectangle(upperLeft, lowerRight, color, lines);
                return lines;
            }

        } // namespace primitives


    } // namespace render
} // namespace yaget
