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
        static inline math3d::Color PreviousColor = { -1, -1, -1, -1 };

        std::string mText;
        int mX{ PreviousValue };
        int mY{ PreviousValue };
        float mSize{ - 1 };
        math3d::Color mColor{ PreviousColor };
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
