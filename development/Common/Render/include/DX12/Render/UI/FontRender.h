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
#include "Render/Commands/RenderCommandTypes.h"
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
    struct TextPrinter
    {
        static inline int PreviousValue = std::numeric_limits<int>::max();

        std::string mText;
        int mX;
        int mY;
        float mSize;
        math3d::Color mColor;
    };
    using TextPrinters = std::vector<TextPrinter>;

    //-------------------------------------------------------------------------------------------------
    io::Buffer GetText(const TextPrinters& textPrinters);


    //-------------------------------------------------------------------------------------------------
    class FontStorage
    {
    public:
        FontStorage(RenderGeometries& renderGeometries, GeometriesResources& geometryResources, scene::SceneItemsStorage& sceneItemsStorage, io::VirtualTransportSystem& vts);
        ~FontStorage();

        void UpdateText(const io::Tag& tag, const TextPrinters& textPrinters, commands::Type commandType);

    private:
        RenderGeometries& mRenderGeometries;
        GeometriesResources& mGeometryResources;
        scene::SceneItemsStorage& mSceneItemsStorage;
        io::VirtualTransportSystem& mVTS;
    };

}
