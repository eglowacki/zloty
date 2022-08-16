///////////////////////////////////////////////////////////////////////
// EditorGameCoordinator.h
//
//  Copyright 06/06/2021 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      
//
//
//  #include "EditorGameCoordinator.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "YagetCore.h"
#include "EditorSystem.h"


namespace yaget::editor
{
    namespace internal
    {
        using SystemsCoordinatorE = comp::gs::SystemsCoordinator<EditorGameCoordinatorSet, Messaging, Application, EditorSystem>;
    }

    class EditorSystemsCoordinator : public internal::SystemsCoordinatorE
    {
    public:
        EditorSystemsCoordinator(Messaging& m, Application& app);
        ~EditorSystemsCoordinator();

    private:
        comp::ItemIds mItems;
        comp::ItemIds mGlobalItems;
    };
}
