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
#include "Render/Pipeline/PipelineTags.h"
#include "Render/Pipeline/RenderGeometries.h"
#include "Render/Pipeline/RenderMaterialProperties.h"
#include "Render/Pipeline/RenderPipelines.h"
#include "Render/Pipeline/RenderShaders.h"
#include "Render/Pipeline/RenderSignatures.h"
#include "Render/Pipeline/RenderTextures.h"
#include "Render/Pipeline/ShaderBuffers.h"
#include "Render/Scene/RenderSceneItems.h"


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
        void RebindMaterial(const io::Tag& tag, const yaget::render::MaterialPropertyTags& material);
        void HotRebindMaterial(const Guid& guid);

        void OnResetDevice(const app::WindowFrame& windowFrame, yaget::render::DeviceB::ResizeState resizeState);

        // This structure is used to keep track of what assets are used for rendering particular entity. 
        // It is used to track changes in assets and update them accordingly.
        struct RenderState
        {
            Guid mSignatureGuid;
            Guid mPipelineGuid;
            Guid mVertexShaderGuid;
            Guid mPixelShaderGuid;
        };

        math3d::Interpolator<colors::Color> mColorInterpolator;
        math3d::Interpolator<float> mMatrixInterpolator;

        DependencyGraph mDependencyGraph;
        yaget::render::RenderSignatures mRenderSignatures;
        yaget::render::RenderPipelines mRenderPipelines;
        yaget::render::RenderShaders mRenderShaders;
        yaget::render::PipelineTags mPipelineTags;
        yaget::render::RenderMaterialProperties mRenderMaterials;
        yaget::render::RenderTextures mRenderTextures;
        yaget::render::TextureResources mTextureResources;
        yaget::render::ShaderBuffers mShaderBuffers;
        yaget::render::RenderGeometries mRenderGeometries;
        yaget::render::GeometriesResources mGeometryResources;
        yaget::render::commands::RenderTargetStorage mRenderTargetStorage;
        yaget::render::scene::SceneItemsStorage mSceneItemsStorage;
        yaget::render::commands::RenderPasses mRenderPasses;
        io::Tag mSwapChainRenderTargetTag{ .mName = "SwapChainRenderTarget", .mGuid = NewGuid() };
        io::Tag mSceneRenderTargetTag{ .mName = "SceneRenderTarget", .mGuid = NewGuid() };

        size_t mResizeCallbackId{};

        RenderState mCurrentRenderState;
    };

}
