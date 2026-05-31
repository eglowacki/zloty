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
#include "Render/Commands/RenderTarget.h"
#include "Render/DesktopApplication.h"
#include "Render/Commands/RenderPasses.h"
#include "Render/Pipeline/PipelineContext.h"
#include "Render/Pipeline/PipelineTags.h"
#include "Render/Pipeline/RenderGeometries.h"
#include "Render/Pipeline/RenderMaterialProperties.h"
#include "Render/Pipeline/RenderPipelines.h"
#include "Render/Pipeline/RenderShaders.h"
#include "Render/Pipeline/RenderSignatures.h"
#include "Render/Pipeline/RenderTextures.h"
#include "Render/Pipeline/ShaderBuffers.h"
#include "Render/Scene/RenderSceneItems.h"
#include "Render/UI/FontRender.h"


namespace defensor::render
{
    class RenderSystem : public yaget::render::RenderSystemApp<RenderCoordinatorSet, comp::gs::GenerateEndMarker, Messaging, SceneComponent*>
    {
    public:
        RenderSystem(Messaging& messaging, Application& app, RenderCoordinatorSet& coordinatorSet);

    private:
        using AssetCacheType = yaget::render::AssetCacheType;
        using Section = io::VirtualTransportSystem::Section;

        void OnUpdate(comp::Id_t id, const time::GameClock& gameClock, metrics::Channel& channel, const SceneComponent* sceneComponent);
        void PreloadAssets();

        void OnResetDevice(const app::WindowFrame& windowFrame, yaget::render::DeviceB::ResizeState resizeState);

        math3d::Interpolator<colors::Color> mColorInterpolator;
        math3d::Interpolator<float> mMatrixInterpolator;

        yaget::render::PipelineContext mPipelineContext;

        io::Tag mSwapChainRenderTargetTag{ .mName = "SwapChainRenderTarget", .mGuid = NewGuid() };
        io::Tag mSceneRenderTargetTag{ .mName = "SceneRenderTarget", .mGuid = NewGuid() };

        size_t mResizeCallbackId{};
        mt::JobPool mAssetPreloader{ "AssetPreloader", 1, mt::JobPool::Behaviour::StartAsRun, true };
        std::atomic_bool mApplicationQuiting{ false };

        yaget::render::commands::RenderPassState mCurrentRenderPassState;

        // frame rate calculations
        float mFramesThisSecond = 1;
        float mAverageFps = 1.0f;
        float mCurrentCalcTime = 0.0f;
    };

}
