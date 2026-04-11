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
        io::Tag mRenderTargetTag;

        // optional, this can be used to specify which SceneItems to use for rendering.
        // If none specified, then render this as is 
        io::VirtualTransportSystem::Sections mSceneItemSections;
        io::Tags mSceneItemTags;
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
