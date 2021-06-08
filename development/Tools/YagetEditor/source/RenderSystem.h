///////////////////////////////////////////////////////////////////////
// RenderSystem.h
//
//  Copyright 06/06/2021 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      
//
//
//  #include "RenderSystem.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once


#include "Components/GameSystem.h"
#include "EditorGameTypes.h"

namespace yaget::render
{
    class DesktopApplication;
    class Device;
}

namespace yaget::editor
{
    //comp::gs::GenerateEndMarker
    class RenderSystem : public yaget::comp::gs::GameSystem<comp::gs::GenerateEndMarker, Messaging, RenderComponent*>
    {
    public:
        RenderSystem(Messaging& messaging, render::DesktopApplication& app);

    private:
        void OnUpdate(yaget::comp::Id_t id, const yaget::time::GameClock& gameClock, yaget::metrics::Channel& channel, RenderComponent* renderComponent);

        render::Device& mDevice;
    };
}
