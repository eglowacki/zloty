//////////////////////////////////////////////////////////////////////
// GameSystemInline.h
//
//  Copyright 2/21/2024 Edgar Glowacki
//
//  Maintained by: Edgar
//
//  NOTES:
//		Inline implementation of GameSystem class
//		Only included by GameSystem.h file.
//
//
//  #include "Components/GameSystemInline.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#ifndef YAGET_GAME_SYSTEM_INCLUDE_IMPLEMENTATION
    #error "Do not include this file explicitly."
#endif // YAGET_GAME_SYSTEM_INCLUDE_IMPLEMENTATION


namespace yaget::comp::gs
{
    //---------------------------------------------------------------------------------------------------------
    template <typename CS, typename E, typename M, typename... Comps>
    void GameSystem<CS, E, M, Comps...>::Tick(const time::GameClock& gameClock, metrics::Channel& channel)
    {
        mCoordinatorSet.template ForEach<Row>([this, &gameClock, &channel](Id_t id, const auto& row)
        {
            Update(id, gameClock, channel, row);
            return true;
        });

        if constexpr (std::is_same_v<EndMarker, GenerateEndMarker>)
        {
            Update(END_ID_MARKER, gameClock, channel, {});
        }
    }

    //---------------------------------------------------------------------------------------------------------
    template <typename CS, typename E, typename M, typename... Comps>
    GameSystem<CS, E, M, Comps...>::GameSystem(const char* niceName, Messaging& messaging, Application& /*app*/, UpdateFunctor updateFunctor, CS& coordinatorSet)
        : mMessaging(messaging)
        , mNiceName(niceName)
        , mUpdateFunctor(updateFunctor)
        , mCoordinatorSet(coordinatorSet)
    {}

    //---------------------------------------------------------------------------------------------------------
    template <typename CS, typename E, typename M, typename... Comps>
    void GameSystem<CS, E, M, Comps...>::Update(Id_t id, const time::GameClock& gameClock, metrics::Channel& channel, const Row& row)
    {
        // NOTE how can we convert row to new row with some of them being const modified?
        auto newRow = std::tuple_cat(std::tie(id, gameClock, channel), row);
        std::apply(mUpdateFunctor, newRow);
    }

} // namespace yaget::comp::gs
