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

namespace yaget::render::commands
{
    struct ScenePassData
    {
        std::string mName;
        io::VirtualTransportSystem::Section mRenderTargetSection;
        // this is filled in on file load based on mRenderTargetSection value
        io::Tag mRenderTargetTag;

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

        float mProjection[6];

        math3d::Matrix GetViewMatrix() const;
        math3d::Matrix GetProjectionMatrix() const;
    };
    using ScenePasses = std::vector<ScenePassData>;

    //-------------------------------------------------------------------------------------------------
    class RenderPasses
    {
    public:
        RenderPasses(io::VirtualTransportSystem& vts);
        ~RenderPasses();

        void BindAsset(const io::Tag& tag);
        const ScenePasses& GetPasses() const { return mPasses; }

    private:
        io::VirtualTransportSystem& mVTS;
        io::Tag mSceneTag;

        ScenePasses mPasses;
    };
}
