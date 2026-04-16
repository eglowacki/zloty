///////////////////////////////////////////////////////////////////////
// FontRender.h
//
//  Copyright 01/11/2026 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//
//  #include "Render/UI/FontRender.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "MathFacade.h"

#include "Render/RenderCore.h"
#include "Streams/Buffers.h"


namespace yaget::render
{
    namespace scene
    {
        class SceneItemsStorage;
    }

    class GeometriesResources;
    class RenderGeometries;
}

namespace yaget::io
{
    class VirtualTransportSystem;
}

namespace yaget::render::ui
{
    //-------------------------------------------------------------------------------------------------
    yaget::io::Buffer GetText(const std::string& text, int x, int y, float size, const math3d::Color* color = nullptr);


    //-------------------------------------------------------------------------------------------------
    class FontStorage
    {
    public:
        FontStorage(RenderGeometries& renderGeometries, GeometriesResources& geometryResources, scene::SceneItemsStorage& sceneItemsStorage, io::VirtualTransportSystem& vts);
        ~FontStorage();

        // if color is nullptr, then it will default to white
        void UpdateText(const io::Tag& tag, const std::string& text, int x, int y, float size, const math3d::Color* color = nullptr);

    private:
        RenderGeometries& mRenderGeometries;
        GeometriesResources& mGeometryResources;
        scene::SceneItemsStorage& mSceneItemsStorage;
        io::VirtualTransportSystem& mVTS;
    };

}
