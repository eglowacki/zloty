/////////////////////////////////////////////////////////////////////////
// ComponentPools.h
//
//  Copyright 8/13/2016 Edgar Glowacki.
//
// NOTES:
//      Exposes one structure to deal with component pools, mostly r&d effort for now
//
//
// #include "Components/ComponentPools.h"
//
/////////////////////////////////////////////////////////////////////////
//! \file
#pragma once
#include "Components/RenderComponent.h"
#include "Components/LocationComponent.h"
#include "Components/PhysicsComponent.h"
#include "Components/ModelComponent.h"
#include "Components/GridComponent.h"
#include "Components/LineComponent.h"
#include "Components/CameraComponent.h"
#include "Components/ModelComponent.h"
#include "Components/TerrainComponent.h"

namespace yaget
{
    namespace io { class VirtualTransportSystem; }

    namespace comp
    {
        struct ComponentPools
        {
            comp::LocationComponentPool mLocation;
            comp::PhysicsComponentPool mPhysics;
            render::ModelComponentPool mModel;
            render::QuadComponentPool mQuad;
            render::LineComponentPool mLine;
            render::GridComponentPool mGrid;
            render::CameraComponentPool mCamera;
            render::TerrainComponentPool mTerrain;
        };


    } // namespace comp

} // namespace yaget


