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

YAGET_COMPILE_SUPPRESS_START(4201, "nonstandard extension used: nameless struct/union")
#include "Mathter/Vector.hpp"
#include "Mathter/Matrix.hpp"
#include "Mathter/Quaternion.hpp"
#include "Mathter/Utility.hpp"
#include "Mathter/Geometry.hpp"
#include "Mathter/IoStream.hpp"
YAGET_COMPILE_SUPPRESS_END


namespace yaget::math
{
    using Vector3 = mathter::Vector<float, 3, false>;
    using Vector2 = mathter::Vector<float, 2, false>;

    using Vector3i = mathter::Vector<int, 3, false>;
    using Vector2i = mathter::Vector<int, 2, false>;

    using Color = mathter::Vector<float, 4, false>;

} // namespace yaget::math

