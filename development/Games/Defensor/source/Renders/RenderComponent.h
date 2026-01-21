///////////////////////////////////////////////////////////////////////
// RenderComponent.h
//
//  Copyright 01/12/2026 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//
//
//  #include "Renders/RenderComponent.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "Components/Component.h"
#include "DefensorRenderTypes.h"
#include "Render/Polygons/RenderShape.h"


struct ID3D12GraphicsCommandList;
namespace yaget::io
{
    class VirtualTransportSystem;
}

namespace yaget::render
{
    namespace platform { class Adapter; }
}


namespace defensor::render
{
    using namespace yaget;

    class RenderComponent : public comp::BaseComponent<comp::DefaultPoolSize>
    {
    public:
        RenderComponent(comp::Id_t id, const math3d::Matrix& matrix, const io::Tag& assetTag, io::VirtualTransportSystem& vts, const yaget::render::platform::Adapter& adapter);
        ~RenderComponent();

        void Render(ID3D12GraphicsCommandList* commandList) const;

        math3d::Matrix mMatrix = math3d::Matrix::Identity;

        // represents actual geometry (triangle topology)
        yaget::render::RenderShape mRenderShape;
    };

}
