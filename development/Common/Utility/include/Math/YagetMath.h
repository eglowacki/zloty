/////////////////////////////////////////////////////////////////////////
// YagetMath.h
//
//  Copyright July 17, 2021 Edgar Glowacki.
//
// NOTES:
//      Including Mather header only library
//
// #include "Math/YagetMath.h"
//
/////////////////////////////////////////////////////////////////////////
//! \file

#pragma once

#include "YagetCore.h"

#if 0

#include "Mathter/Vector.hpp"
#include "Mathter/Matrix.hpp"
#include "Mathter/Quaternion.hpp"
#include "Mathter/Utility.hpp"
#include "Mathter/Geometry.hpp"
#include "Mathter/IoStream.hpp"

// sample of code for converting hemi to uv and back
//Vec3 TexCoordToHemisphere(const float u, const float v)
//{
//    // [0,1] -> [-1,1] paraboloid domain
//    float px = 2.0f * u - 1.0f;
//    float py = 2.0f * (1.0f - v) - 1.0f; // v up -> py positive
//
//    float p2 = px * px + py * py;
//
//    // Optional robustness clamp
//    if (p2 > 1.0f) p2 = 1.0f;
//
//    float inv = 1.0f / (1.0f + p2);
//    float z = (1.0f - p2) * inv;      // z = (1 - p^2) / (1 + p^2)
//    float scale = 2.0f * inv;             // 1 + z = 2 / (1 + p^2)
//
//    float x = px * scale;                 // x = px * (1 + z)
//    float y = py * scale;                 // y = py * (1 + z)
//
//    return Vec3(x, y, z);                 // already unit length
//}
//
//Vec2 HemisphereToTexCoord(const float dx, const float dy, const float dz)
//{
//    // Assume (dx,dy,dz) is normalized and dz >= 0 (front hemisphere)
//    float denom = 1.0f + dz;
//
//    float px = dx / denom;
//    float py = dy / denom;
//
//    float u = px * 0.5f + 0.5f;
//    float v = 1.0f - (py * 0.5f + 0.5f);  // Y-up -> v decreases with +y
//
//    return Vec2(u, v);
//}

namespace yaget::math
{
    using Vector3 = mathter::Vector<float, 3, false>;
    using Vector2 = mathter::Vector<float, 2, false>;

    using Vector3i = mathter::Vector<int, 3, false>;
    using Vector2i = mathter::Vector<int, 2, false>;

    using Color = mathter::Vector<float, 4, false>;

} // namespace yaget::math

#endif // #if 0