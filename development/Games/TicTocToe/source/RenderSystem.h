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
//#include "cpp-terminal/terminal.h"

namespace Term { class Terminal; }

namespace ttt
{
    class RenderComponent;

    class RenderSystem : public yaget::comp::gs::GameSystem<yaget::EndMarkerEntity, RenderComponent*>
    {
    public:
        RenderSystem();
        ~RenderSystem();

    private:
        void OnUpdate(yaget::comp::Id_t id, const yaget::time::GameClock& gameClock, yaget::metrics::Channel& channel, RenderComponent* renderComponent);

        std::unique_ptr<Term::Terminal> mTerminal;
    };
}
