/////////////////////////////////////////////////////////////////////////
// MathFacade.h
//
//  Copyright 7/27/2016 Edgar Glowacki.
//
// NOTES:
//      Helper include to bring SImpleMath (and it's windows 
//      dependencies header)
//
//
// #include "MathFacade.h"
//
/////////////////////////////////////////////////////////////////////////
//! \file

#ifndef YAGET_MATH_FACADA_H
#define YAGET_MATH_FACADA_H
#pragma once

#include "Base.h"
//#include "Math/MathBase.h"
#include "Platform/WindowsLean.h"
#include <d3d11_2.h>
#include <SimpleMath.h>
#include <DirectXColors.h>

namespace math3d
{
    using namespace DirectX::SimpleMath;

    inline math3d::Matrix CreateMatrix(const math3d::Vector3& location, const math3d::Quaternion& rotation, const math3d::Vector3& scale = math3d::Vector3::One)
    {
        // this will scale object first, then rotate and finally move
        return math3d::Matrix::CreateScale(scale) * math3d::Matrix::CreateFromQuaternion(rotation) * math3d::Matrix::CreateTranslation(location);
        // this will move object first, then rotate around initial location point and then scale
        //return math3d::Matrix::CreateTranslation(location) * math3d::Matrix::CreateFromQuaternion(rotation) * math3d::Matrix::CreateScale(scale);
    }

    //! Convert from Degrees to Radians.
    inline float DegToRad(float a)
    {
        return a * 0.01745329252f;
    }

    //! Convert from Radians to Degrees.
    inline float RadToDeg(float a)
    {
        return a * 57.29577951f;
    }

} // namespace math3d


namespace colors
{
    using namespace DirectX::Colors;

    struct Color : public math3d::Color
    {
        Color(const DirectX::XMVECTORF32& colorRef) : math3d::Color(colorRef) {}
        Color(const math3d::Color& colorRef) : math3d::Color(colorRef) {}
        Color(float _r, float _g, float _b, float _a) : math3d::Color(_r, _g, _b, _a) {}
        Color() : math3d::Color() {}
    };

} // namespace colors

//inline math3d::Matrix CreateOrthographicOffCenterLH(float ViewLeft, float ViewRight, float ViewBottom, float ViewTop, float NearZ, float FarZ)
//{
//    using namespace DirectX;
//    math3d::Matrix R;
//    XMStoreFloat4x4(&R, XMMatrixOrthographicOffCenterLH(ViewLeft, ViewRight, ViewBottom, ViewTop, NearZ, FarZ));
//    return R;
//}

//inline math3d::Matrix CreateOrthographicLH(float width, float height, float zNearPlane, float zFarPlane)
//{
//    using namespace DirectX;
//    math3d::Matrix R;
//    XMStoreFloat4x4(&R, XMMatrixOrthographicLH(width, height, zNearPlane, zFarPlane));
//    return R;
//}

inline math3d::Matrix CreatePerspectiveFieldOfViewLH(float fov, float aspectRatio, float nearPlane, float farPlane)
{
    using namespace DirectX;
    math3d::Matrix R;
    XMStoreFloat4x4(&R, XMMatrixPerspectiveFovLH(fov, aspectRatio, nearPlane, farPlane));
    return R;
}

namespace yaget
{
    namespace math
    {
        // left, up, width, height
        typedef math3d::Vector4 Size_t;
        // left, up, right, bottom
        typedef  math3d::Vector4 Rect_t;

        struct Box
        {
            Box() = default;// : mMin(std::numeric_limits<float>::max()), mMax(std::numeric_limits<float>::lowest()) {}
            Box(float v) : mMin(v), mMax(v) {}

            math3d::Vector3 mMin = math3d::Vector3(std::numeric_limits<float>::max());
            math3d::Vector3 mMax = math3d::Vector3(std::numeric_limits<float>::lowest());

            void GrowExtends(const math3d::Vector3& meshPoint)
            {
                mMax.x = meshPoint.x > mMax.x ? meshPoint.x : mMax.x;
                mMax.y = meshPoint.y > mMax.y ? meshPoint.y : mMax.y;
                mMax.z = meshPoint.z > mMax.z ? meshPoint.z : mMax.z;

                mMin.x = meshPoint.x < mMin.x ? meshPoint.x : mMin.x;
                mMin.y = meshPoint.y < mMin.y ? meshPoint.y : mMin.y;
                mMin.z = meshPoint.z < mMin.z ? meshPoint.z : mMin.z;
            }

            void GrowExtends(const Box& box)
            {
                GrowExtends(box.mMin);
                GrowExtends(box.mMax);
            }

            math3d::Vector3 GetSize() const
            {
                return math3d::Vector3(mMax - mMin);
            }

            math3d::Vector3 GetMid() const
            {
                return mMin + math3d::Vector3(GetSize() / 2.0f);
            }

            static Box Zero()
            {
                static Box zeroBox(0);
                return zeroBox;
            }
        };

    } // namespace math
} // namespace yaget

#endif // YAGET_MATH_FACADA_H

