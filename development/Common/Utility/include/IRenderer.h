///////////////////////////////////////////////////////////////////////
// IRenderer.h
//
//  Copyright 10/8/2005 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      Interface to render Object
//
//
//  #include "IRenderer.h"
//
//////////////////////////////////////////////////////////////////////
//! \file

#ifndef I_RENDERER_H
#define I_RENDERER_H
#pragma once



#include "Interface/IRendererTypes.h"
#include "Interface/ICollision.h"
#include "Hash/Hash.h"
#include "ObjectID.h"
#include "Plugin/IPluginObject.h"
#include "MessageInterface.h"
#include "Registrate.h"
#include "Math/Matrix.h"

class wxWindow;

namespace eg
{
    typedef void *WXWidget;
    namespace render
    {
        const uint32_t DefaultResX = 1024;
        const uint32_t DefaultResY = 768;
    } // namespace render

    /*!
    This is used with kGetRendererInitData message.
    Renderer will create this structure and Message.mpData
    will point to instance of it before this message send.
    */
    struct RenderInitData
    {
        RenderInitData() :
        mWidgetHandle(0), mResX(0), mResY(0), mFlags(0)
        {
        }

        //! OS dependent window handle
        WXWidget mWidgetHandle;
        //! Resolution to set renderer
        uint32_t mResX, mResY;
        //! Flags, zero more combination of IRenderer::rf<...>
        uint32_t mFlags;
    };

    //! Renderer object
    class DLLIMPEXP_UTIL_CLASS IRenderer : public IPluginObject
    {
    public:
        //! This is used to create new view into existing renderer
        //! which provides separate window
        struct ViewToken
        {
            ViewToken() : index(-1)
            {
            }
            int32_t index;
            //! Quick check to see if this view token is valid
            //bool IsValid() const {return index > -1);
        };

        //! Render Flags (rf...) which are passed to Init method
        static const uint32_t rfFullScreenMode = 0x0001;  ///< initialize as a full screen mode (default is Window Mode)
        static const uint32_t rfFreeRefreshMode = 0x0002; ///< no syncing with refresh rate (default is Sync with Refresh))

        //! This is called anytime we change window size, switch vide modes (full screen, window) or change resolution
        virtual void Reset() = 0;

        //! This called before we start rendering
        virtual bool BeginScene() const = 0;

        //! This called after we finish rendering
        virtual void EndScene() const = 0;

        //! This will make back buffer visible (copy/flip to front buffer)
        virtual void Flip() const = 0;

        //! Return view token used in mult window rendering
        virtual ViewToken CreateViewToken(void *pWindow) = 0;

        //! This will set renderer to use this view token for rendering
        //! \return true if token was valid ands set
        virtual bool UseViewToken(ViewToken viewToken) = 0;

        //! This will draw line in specific color
        //!
        //! \param begPos beginning position of line
        //! \param endPos ending position of line
        //! \param begColor beginning color of line
        //! \param endColor ending color of line
        //! \param pMatrix if this is NULL, then begPos/endPos are in screen space, otherwise transform points using this matrix
        virtual void DrawLine(const Vector3& begPos, const Vector3& endPos, const Color4_t& begColor, const Color4_t& endColor, const Matrix44 *pMatrix) = 0;

        //! Print text at specified screen coordinates (upper-left is 0,0) with color
        //!
        //! \param x screen coordinates in pixels
        //! \param y screen coordinates in pixels
        //! \param text string to print
        //! \param color use this color for text
        virtual void Print(int32_t x, int32_t y, const std::string& text, const Color4_t& color) = 0;

        //! Set global font which is used by Print(...) methods
        //!
        //! \param fontName name of font to use
        //! \param size size of the font
        //! \param bBold true, then render as a bol, otherise use default
        virtual void SetFont(const std::string& fontName, size_t size, bool bBold) = 0;

        //! Return list of all objects hit by ray.
        //!
        //! \param viewObjectId which view (camera) object to use
        //! \param mousePos mouse position to use to generate hit ray
        //! \param bGetAllObjects if this si TRUE< then return all objects along the hit ray path, otherwwise return
        //!        closest one
        //! \return vector of object id's which are in the hit ray path. All those objects have some kind od bounding valume
        //!         attached to it, otherwise the are ignored
        virtual collision::Response GetHitObjects(uint64_t viewObjectId, const Vector2& mousePos, bool bGetAllObjects = false) const = 0;
        virtual collision::Response GetHitObjects(const std::string& viewObjectName, const Vector2& mousePos, bool bGetAllObjects = false) const = 0;
    };

    //! Helper function to check if token is valid
    inline bool IsViewTokenValid(const IRenderer::ViewToken& viewToken)
    {
        return viewToken.index > -1;
    }

    /*
    //! Convenience function
    inline void DrawLine(const Vector3& begPos, const Vector3& endPos, const Color4_t& begColor = v4::ONE(), const Color4_t& endColor = v4::ONE(), const Matrix44 *pMatrix = 0)
    {
        if (IRenderer *pIRenderer = registrate::p_cast<IRenderer>("Renderer"))
        {
            pIRenderer->DrawLine(begPos, endPos, begColor, endColor, pMatrix);
        }
    }

    //! Convenience function
    inline void Print(int32_t x, int32_t y, const std::string& text, const Color4_t& color = v4::WONE())
    {
        if (IRenderer *pIRenderer = registrate::p_cast<IRenderer>("Renderer"))
        {
            pIRenderer->Print(x, y, text, color);
        }
    }
    */

    namespace render
    {
        /*
        inline collision::Response GetHitObjects(const std::string& viewObjectName, const Vector2& mousePos, bool bGetAllObjects = false)
        {
            if (IRenderer *pIRenderer = registrate::p_cast<IRenderer>("Renderer"))
            {
                return pIRenderer->GetHitObjects(viewObjectName, mousePos, bGetAllObjects);
            }

            return collision::Response();
        }

        inline collision::Response GetHitObjects(uint64_t viewObjectId, const Vector2& mousePos, bool bGetAllObjects = false)
        {
            if (IRenderer *pIRenderer = registrate::p_cast<IRenderer>("Renderer"))
            {
                return pIRenderer->GetHitObjects(viewObjectId, mousePos, bGetAllObjects);
            }

            return collision::Response();
        }

        inline void SetFont(const std::string& fontName, size_t size, bool bBold)
        {
            if (IRenderer *pIRenderer = registrate::p_cast<IRenderer>("Renderer"))
            {
                pIRenderer->SetFont(fontName, size, bBold);
            }
        }
        */

    } // namespace render


} // namespace eg

#endif // I_RENDERER_H

