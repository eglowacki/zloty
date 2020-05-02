/////////////////////////////////////////////////////////////////////////
// Profiler.h
//
//  Copyright 4/19/2009 Edgar Glowacki.
//
// NOTES:
//      Provides c++ wrapper for profiler code
//      to manage it (things like input and rendering)
//      where sampling it's still uses macros
//
//
// #include "Profiler/Profiler.h"
//
/////////////////////////////////////////////////////////////////////////
//! \file

#ifndef PROFILER_PROFILER_H
#define PROFILER_PROFILER_H
#pragma once

#include "Input/InputManager.h"
#include "Profiler/prof.h"


namespace eg
{
    class Profiler
    {
        // no copy semantics
        Profiler(const Profiler&);
        Profiler& operator=(const Profiler&);

    public:
        Profiler(InputManager& input);
        ~Profiler();

        void Tick(float deltaTime);

    private:
        void onAction(const std::string& actionName, uint32_t timeStamp, int32_t mouseX, int32_t mouseY);

        InputManager& mInput;
        bool mbProfilerOn;
        int mProfilerMode;
        bool mbProfilerGraph;
        float mCurrentTotal;
        InputManager::ActionListener_t mProfilerToggle;
        InputManager::ActionListener_t mProfilerReportMode;
        InputManager::ActionListener_t mProfilerMoveUp;
        InputManager::ActionListener_t mProfilerMoveDown;
        InputManager::ActionListener_t mProfilerSelectZone;
        InputManager::ActionListener_t mProfilerSelectParent;
        InputManager::ActionListener_t mProfilerToggleDisplay;
    };


} // namespace eg

#endif // PROFILER_PROFILER_H

