///////////////////////////////////////////////////////////////////////
// RenderPasses.h
//
//  Copyright 04/10/2026 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      
//
//  #include "Render/Commands/RenderPasses.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "Streams/Buffers.h"
#include "VTS/VirtualTransportSystem.h"
#include "Render/Commands/RenderTarget.h"

namespace yaget::render::commands
{
    //struct DepthStencilClear;

    struct ScenePassData
    {
        std::string mName;
        io::VirtualTransportSystem::Section mRenderTargetSection;
        // this is filled in on file load based on mRenderTargetSection value
        io::Tag mRenderTargetTag;

        // we can specify clear color clear and depth-stencil clear values.
        struct ClearValues
        {
            bool mUseClearColor = false;
            bool mUseClearDepth = false;
        };
        
        ClearValues mClearValues;

        const math3d::Color* GetColorClear(const RenderTarget* renderTarget) const;
        const DepthStencilClear* GetDepthStencilClear(const RenderTarget* renderTarget) const;

        // optional, this can be used to specify which SceneItems to use for rendering.
        // If none specified, then render this as is 
        io::VirtualTransportSystem::Sections mSceneItemSections;
        // this is filled in on file load based on mSceneItemSections value
        io::Tags mSceneItemTags;

        // optional, specify camera and/or projection matrix for this pass.
        // This is useful in simple pass rendering where we do not need control of camera
        bool mCameraValid = false;
        enum class CameraValues { Position, Target, Up };
        math3d::Vector3 mViewCamera[3];

        enum class ProjectionType { None, Orthographic, OrthographicOffCenter, PerspectiveFOV, Perspective, PerspectiveOffCenter };
        ProjectionType mProjectionType = ProjectionType::None;

        enum class ProjectionValues { Left, Right, Bottom, Top, Near, Far };
        enum class ProjectionValues1 { Width, Height, Near, Far };
        enum class ProjectionValues2 { FOV, AspectRatio, Near, Far };

        constexpr static inline size_t NumElementsInProjection = 6;

        float mProjection[NumElementsInProjection];

        enum class ProjectionCalculationType { None, WindowSizeX, WindowSizeY };
        ProjectionCalculationType mProjectionCalculationType[NumElementsInProjection] = {};

        math3d::Matrix GetViewMatrix() const;
        math3d::Matrix GetProjectionMatrix() const;

        const app::WindowFrame* mWindowFrame{};
    };
    using ScenePasses = std::vector<ScenePassData>;

    //-------------------------------------------------------------------------------------------------
    class RenderPasses
    {
    public:
        RenderPasses(io::VirtualTransportSystem& vts, const app::WindowFrame& windowFrame);
        ~RenderPasses();

        void BindAsset(const io::Tag& tag);
        const ScenePasses& GetPasses() const { return mPasses; }

        // Return true if the given tag should be rendered in this pass. This is calculated based
        // on previous passes if any have tag in mSceneItemTags, which means do not render this pass,
        // since that tag was already rendered.
        bool RenderThisPass(const io::Tag& tag, const ScenePassData& pass) const;

    private:
        io::VirtualTransportSystem& mVTS;
        const app::WindowFrame& mWindowFrame;
        io::Tag mSceneTag;

        ScenePasses mPasses;
    };
}
