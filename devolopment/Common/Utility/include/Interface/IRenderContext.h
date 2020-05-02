///////////////////////////////////////////////////////////////////////
// IRenderContext.h
//
//  Copyright 11/1/2005 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      Abstract interfacd to render context which provides data access for geometry
//      objects during rendering phase.
//
//
//  #include "Interface/IRenderContext.h"
//
//////////////////////////////////////////////////////////////////////
//! \file

#ifndef I_RENDER_CONTEXT_H
#define I_RENDER_CONTEXT_H

#include "Base.h"

namespace eg
{
    // forward declarations
    class Matrix44;
    class Vector3;
    class Vector2;

    /*!
    This provides data to geometry object for the current frame duration
    */
    class IRenderContext
    {
    public:
        //! Return world matrix
        virtual const Matrix44& GetWorldMatrix() const = 0;
        //! Return view matrix (camera)
        virtual const Matrix44& GetViewMatrix() const = 0;
        //! Return projection matrix
        virtual const Matrix44& GetProjectionMatrix() const = 0;
        //! Return full matrix (World * View * Projection)
        //! To transform vertex
        //! Vector4 newPos = matrix * pos;
        virtual const Matrix44& GetWVPMatrix() const = 0;

        //! This will return scale of world matrix
        virtual const Vector3& GetWorldScale() const = 0;

        //! This will return current render id
        virtual uint32_t GetRenderId() const = 0;

        //! Return object id for camera used for this context
        virtual uint64_t GetCameraId() const = 0;

        //! Return size of render target in pixels
        //! To calculate aspect ratio:
        //! float aspectRatio = GetRenderTargetSize().x / GetRenderTargetSize().y;
        virtual const Vector2& GetRenderTargetSize() const = 0;
    };

} // namespace eg

#endif // I_RENDER_CONTEXT_H

