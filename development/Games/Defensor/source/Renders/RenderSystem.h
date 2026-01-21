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
#include "Parsers/DependencyGraph.h"
#include "Render/DesktopApplication.h"
#include "RenderPipeline.h"
#include "Renders/RenderSignatures.h"
#include "RenderShader.h"


namespace defensor::render
{
    class RenderSystem : public yaget::render::RenderSystemApp<RenderCoordinatorSet, comp::gs::GenerateEndMarker, Messaging, RenderComponent*, SceneComponent*>
    {
    public:
        RenderSystem(Messaging& messaging, Application& app, RenderCoordinatorSet& coordinatorSet);

    private:
        void OnUpdate(comp::Id_t id, const time::GameClock& gameClock, metrics::Channel& channel, RenderComponent* renderComponent, SceneComponent* sceneComponent);
        void PreloadAssets();

        mt::JobPool mAssetPoolThread;
        math3d::Interpolator mColorInterpolator;
        RenderSignatures mRenderSignatures;
        RenderPipeline mRenderPipeline;
        RenderShader mRenderShader;

        DependencyGraph mDependencyGraph;

        std::atomic_bool mAssetsPreloaded{false};
    };

}
