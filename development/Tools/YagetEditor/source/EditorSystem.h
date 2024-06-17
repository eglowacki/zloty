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
    class EditorSystem : public comp::gs::GameSystem<EditorGameCoordinatorSet, comp::gs::NoEndMarker, Messaging, EmptyComponent*, BlankComponent*/*, EditorComponent* */>
    {
    public:
        EditorSystem(Messaging& messaging, Application& app, EditorGameCoordinatorSet& coordinatorSet);

    private:
        void OnUpdate(comp::Id_t id, const time::GameClock& gameClock, metrics::Channel& channel, EmptyComponent* emptyComponent, const BlankComponent* blankComponent/*, EditorComponent* editorComponent*/);
    };
}
