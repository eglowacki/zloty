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
//  #include "Render/Components/RenderComponent.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "Components/Component.h"


namespace yaget::render
{
    //-------------------------------------------------------------------------------------------------
    class RenderComponent : public comp::BaseComponent<comp::DefaultPoolSize>
    {
    public:
        RenderComponent(comp::Id_t id, const math3d::Matrix& matrix, const io::Tag& sceneItemTag);
        ~RenderComponent();

        void UpdateMatrix(const math3d::Matrix& matrix);

        math3d::Matrix mMatrix = math3d::Matrix::Identity;
        math3d::Matrix mMatrixT = math3d::Matrix::Identity;
        io::Tag mSceneItemTag;
    };

}
