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

struct ID3D12GraphicsCommandList;
namespace yaget::render
{
    class Polygon;
    namespace platform { class Adapter; }
}

namespace defensor::render
{
    using namespace yaget;

    class RenderComponent : public comp::BaseComponent<comp::DefaultPoolSize>
    {
    public:
        RenderComponent(comp::Id_t id, const math3d::Matrix& matrix, const yaget::render::platform::Adapter& adapter);
        ~RenderComponent();

        void Render(ID3D12GraphicsCommandList* commandList);

        math3d::Matrix mMatrix = math3d::Matrix::Identity;
        std::unique_ptr<yaget::render::Polygon> mPolygon;
    };

}
