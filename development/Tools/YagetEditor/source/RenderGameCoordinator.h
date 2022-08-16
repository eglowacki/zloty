///////////////////////////////////////////////////////////////////////
// RenderGameCoordinator.h
//
//  Copyright 06/06/2021 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      
//
//
//  #include "RenderGameCoordinator.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "RenderSystem.h"

namespace yaget::render { class DesktopApplication; }

namespace yaget::editor
{
    namespace internal
    {
        using SystemsCoordinatorR = comp::gs::SystemsCoordinator<RenderCoordinatorSet, Messaging, render::DesktopApplication, RenderSystem>;
    }

    class RenderSystemsCoordinator : public internal::SystemsCoordinatorR
    {
    public:
        RenderSystemsCoordinator(Messaging& m, render::DesktopApplication& app);
        ~RenderSystemsCoordinator();

    private:
        comp::ItemIds mRenderItems;
    };
}
