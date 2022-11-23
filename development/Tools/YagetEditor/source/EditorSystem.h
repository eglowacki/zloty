///////////////////////////////////////////////////////////////////////
// EditorSystem.h
//
//  Copyright 10/10/2020 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      System which iterates and tick over entities
//
//
//  #include "EditorSystem.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once


#include "Components/GameSystem.h"
#include "EditorGameTypes.h"

namespace yaget::editor
{
    class EditorSystem : public yaget::comp::gs::GameSystem<comp::gs::NoEndMarker, Messaging, /*EditorComponent*,*/ EmptyComponent*, BlankComponent*>
    {
    public:
        EditorSystem(Messaging& messaging, Application& app);

    private:
        void OnUpdate(yaget::comp::Id_t id, const yaget::time::GameClock& gameClock, yaget::metrics::Channel& channel, /*EditorComponent* editorComponent,*/ EmptyComponent* emptyComponent, const BlankComponent* blankComponent);
    };
}
