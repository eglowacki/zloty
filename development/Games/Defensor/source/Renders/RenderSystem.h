///////////////////////////////////////////////////////////////////////
// RenderSystem.h
//
//  Copyright 01/11/2026 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//
//  #include "Renders/RenderSystem.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "DefensorGameTypes.h"
#include "Math/Interpolators.h"
#include "Render/DesktopApplication.h"

namespace yaget::render 
{
    class DeviceB; 
    //class DesktopApplication; 
}

namespace defensor::render
{
    class RenderSystem : public yaget::render::RenderSystemApp<RenderCoordinatorSet, comp::gs::GenerateEndMarker, Messaging, RenderComponent*, SceneComponent*>
    {
    public:
        RenderSystem(Messaging& messaging, Application& app, RenderCoordinatorSet& coordinatorSet);

    private:
        void OnUpdate(comp::Id_t id, const time::GameClock& gameClock, metrics::Channel& channel, RenderComponent* renderComponent, SceneComponent* sceneComponent);

        math3d::Interpolator mColorInterpolator;
    };

}
