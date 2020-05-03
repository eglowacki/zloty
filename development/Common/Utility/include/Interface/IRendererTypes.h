/////////////////////////////////////////////////////////////////////
// IRendererTypes.h
//
//  Copyright 1/27/2008 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      Standard header for message ids and any emums
//      pertaining to Render module
//
//
//  #include "Interface/IRendererTypes.h"
//
//////////////////////////////////////////////////////////////////////
//! \file

#ifndef INTERFACE_IRENDERER_TYPES_H
#define INTERFACE_IRENDERER_TYPES_H
#pragma once

#include "Base.h"

namespace eg
{
    namespace mtype
    {
        //! This can be send to MessageManager to get current pointer
        //! to valid 3d device which will be in void *. This is very os and hardware
        //! specific
        //! \note NOT USED
        static const guid_t kGet3dDevice = 0x316e398c;

        //! This is send by Renderer from OnInit method to get
        //! initialization data needed to create and activate renderer.
        //! Message.mpData points to RenderInitData structure
        static const guid_t kGetRendererInitData = 0xb17ccbe0;

        //! This send system wide anytime renderer switches video modes
        //! Message.mpData points to RenderInitData structure filled with correct values
        //! You can disregard mWidgetHandle
        //! \note NOT USED
        static const guid_t kRendererVideoModeChanged = 0x717cd0a5;

        //! This is send before we switch video mode or resolution, which
        //! which allows to handle re-creation of resources.
        //! Message.mpData points to NULL.
        //! \note NOT USED
        static const guid_t kRendererVideoModeReset = 0x91fc5673;

        //! This is send by renderer just before any rendering starts
        //! boost::any - Vector2 which holds width and height of a viewport
        static const guid_t kBeginRenderFrame = 0xb4a7455f;

        //! This is send by renderer right after rendering of entities is finished
        //! This will happen before buffer flip
        //! boost::any - not used
        static const guid_t kEndRenderFrame = 0x94a7463e;

        //! Send to component (in most cases that would be View one)
        //! to receive pick Ray object in world coordinates.
        //! boost::any:
        //!     [in]  Vector2 mouse position in local window coordinates
        //!     [out] Ray object in world coordinates
        static const guid_t kGetPickRay = 0xb4cb35c1;

    } // namespace mtype

} // namespace eg

#endif // INTERFACE_IRENDERER_TYPES_H

