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
#include "Render/Pipeline/RenderMaterialProperties.h"
#include "Render/Pipeline/RenderPipelines.h"
#include "Render/Pipeline/RenderShaders.h"
#include "Render/Pipeline/RenderSignatures.h"
#include "Render/Pipeline/RenderTextures.h"
#include "Render/Pipeline/ShaderBuffers.h"


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
        void RebindMaterial(const io::Tag& tag, yaget::render::MaterialProperties material);
        void HotRebindMaterial(const Guid& guid);

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
        yaget::render::RenderMaterialProperties mRenderMaterials;
        yaget::render::RenderTextures mRenderTextures;
        yaget::render::TextureResources mTextureResources;
        yaget::render::ShaderBuffers mShaderBuffers;

        RenderState mCurrentRenderState;
    };

}
