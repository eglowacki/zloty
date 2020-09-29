//////////////////////////////////////////////////////////////////////
// PythonComponent.h
//
//  Copyright 4/22/2020 Edgar Glowacki
//
//  Maintained by: Edgar
//
//  NOTES:
//      Lua specific implementation of script componenet
//
//
//  #include "Scripting/LuaComponent.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "YagetCore.h"
#include "Components/Component.h"
#include "Streams/Buffers.h"


namespace yaget
{
    namespace time { class GameClock; }
    namespace metrics { class Channel; }

    namespace scripting
    {
        class LuaComponent : public comp::Component
        {
        public:
            LuaComponent(comp::Id_t id, const io::Tags& tags);
            virtual ~LuaComponent();

            void Update(const time::GameClock& gameClock, metrics::Channel& channel);

            void InitializeScript(const time::GameClock& gameClock, metrics::Channel& channel);

        private:
            size_t CalculateStateHash() const override;
            io::Tags ParseTags(const io::Tags& tags) const;
			
            io::Tags mTags;
        };

    } // namespace scripting
} // namespace yaget
