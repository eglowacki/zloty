///////////////////////////////////////////////////////////////////////
// PipelinesContext.h
//
//  Copyright 05/22/2026 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//
//  #include "Render/Pipeline/PipelineContext.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "Parsers/DependencyGraph.h"
#include "Render/RenderCore.h"
#include "Render/Commands/RenderPasses.h"
#include "Render/Commands/RenderTarget.h"
#include "Render/Pipeline/PipelineTags.h"
#include "Render/Pipeline/RenderGeometries.h"
#include "Render/Pipeline/RenderMaterialProperties.h"
#include "Render/Pipeline/RenderPipelines.h"
#include "Render/Pipeline/RenderShaders.h"
#include "Render/Pipeline/RenderSignatures.h"
#include "Render/Pipeline/RenderTextures.h"
#include "Render/Pipeline/ShaderBuffers.h"
#include "Render/UI/FontRender.h"
#include "Render/Scene/RenderSceneItems.h"
#include "Render/Device.h"



namespace yaget::render
{
    class PipelineContext
    {
    public:
        using ProgressCallback = std::function<void(const comp::gs::InitEvent& event)>;
        PipelineContext(DeviceB& device, io::VirtualTransportSystem& vts, ProgressCallback progressCallback);

        void PreloadAssets(const std::string& sectionSuffix);
        void OnResetDevice(const app::WindowFrame& windowFrame, DeviceB::ResizeState resizeState);

        DependencyGraph mDependencyGraph;
        RenderSignatures mRenderSignatures;
        RenderPipelines mRenderPipelines;
        RenderShaders mRenderShaders;
        PipelineTags mPipelineTags;
        RenderMaterialProperties mRenderMaterials;
        RenderTextures mRenderTextures;
        TextureResources mTextureResources;
        ShaderBuffers mShaderBuffers;
        RenderGeometries mRenderGeometries;
        GeometriesResources mGeometryResources;
        commands::RenderTargetStorage mRenderTargetStorage;
        scene::SceneItemsStorage mSceneItemsStorage;
        ui::FontStorage mFontStorage;
        commands::RenderPasses mRenderPasses;

    private:
        using Section = io::VirtualTransportSystem::Section;

        void RebindMaterial(const io::Tag& tag, const yaget::render::MaterialPropertyTags& material);
        void HotRebindItemProperties(const Guid& guid);

        io::VirtualTransportSystem& mVTS;
        ProgressCallback mProgressCallback;
    };
}
