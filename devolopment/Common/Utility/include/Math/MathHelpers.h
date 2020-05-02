///////////////////////////////////////////////////////////////////////
// MathHelpers.h
//
//  Copyright 10/30/2005 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//
//
//  #include "Math/MathHelpers.h"
//
//////////////////////////////////////////////////////////////////////
//! \file

#ifndef MATH_MATH_HELPERS_H
#define MATH_MATH_HELPERS_H
#pragma once

#include "Base.h"


namespace eg
{
    //! This will convert string delimited with , into floating point values
    void GetFloatValues(const char *pValue, float *a_FloatValues, size_t numValues);
} // namespace eg

#endif // MATH_MATH_HELPERS_H

