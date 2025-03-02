//////////////////////////////////////////////////////////////////////
// ScriptComponent.h
//
//  Copyright 8/19/2019 Edgar Glowacki
//
//  Maintained by: Edgar
//
//  NOTES:
//      
//
//
//  #include "Components/ScriptComponent.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "Component.h"


namespace yaget
{
    namespace time { class GameClock; }
    namespace metrics { class Channel; }

    namespace comp
    {
        class ScriptComponent;

        class ScriptComponent : public Component
        {
        public:
            using ScriptFunction = std::function<void(comp::Id_t id, const time::GameClock& gameClock, metrics::Channel& channel)>;

            ScriptComponent(Id_t id, ScriptFunction scriptFunction);
            ~ScriptComponent() override;

            void Update(const time::GameClock& gameClock, metrics::Channel& channel);

        private:
            size_t CalculateStateHash() const override;

            ScriptFunction mScriptFunction;
        };

    } // namespace comp
} // namespace yaget
