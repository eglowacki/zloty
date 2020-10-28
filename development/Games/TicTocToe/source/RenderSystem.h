/////////////////////////////////////////////////////////////////////////
// RenderSystem.h
//
//  Copyright 10/25/2020 Edgar Glowacki.
//
// NOTES:
//      Render system that will iterate over all RenderComponents
//
// #include "RenderSystem.h"
//
/////////////////////////////////////////////////////////////////////////
//! \file

#pragma once

#include "Components/GameSystem.h"
#include "GameCoordinator.h"

namespace ttt
{
    class RenderComponent;

    class RenderSystem : public yaget::comp::gs::GameSystem<yaget::EndMarkerEntity, RenderComponent*>
    {
    public:
        RenderSystem();

    private:
        void OnUpdate(yaget::comp::Id_t id, const yaget::time::GameClock& gameClock, yaget::metrics::Channel& channel, RenderComponent* renderComponent);
    };
}
