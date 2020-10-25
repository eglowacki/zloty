//////////////////////////////////////////////////////////////////////
// GameSystemsCoordinator.h
//
//  Copyright 10/19/2020 Edgar Glowacki
//
//  Maintained by: Edgar
//
//  NOTES:
//      Replaces functionality of GameCoordinator, but serves similar purpose
//
//
//  #include "Components/GameSystemsCoordinator.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "CoordinatorSet.h"

namespace yaget {
    namespace metrics {
        class Channel;
    }
}

namespace yaget::comp::gs
{
    // Create coordinator for system and call each for update
    template <typename T, typename... S>
    class GameSystemsCoordinator
    {
    public:
        using CoordinatorSet = T;
        using Systems = std::tuple<S...>;

        void Tick(const time::GameClock& gameClock, metrics::Channel& channel);

        template <typename C>
        comp::Coordinator<C>& GetCoordinator();

        template <typename C>
        const comp::Coordinator<C>& GetCoordinator() const;

    private:
        CoordinatorSet mCoordinatorSet;
        Systems mSystems;
    };

}


//#include "CoordinatorSetImplementation.h"
namespace yaget::comp::gs
{
    template <typename T, typename... S>
    void GameSystemsCoordinator<T, S...>::Tick(const time::GameClock& gameClock, metrics::Channel& channel)
    {
        // possibly run each system on own thread, taking Policy (usage) into account
        meta::for_each(mSystems, [this, &gameClock, &channel](auto& gameSystem)
        {
            gameSystem.Tick(mCoordinatorSet);
        });
    }

    template <typename T, typename ... S>
    template <typename C>
    comp::Coordinator<C>& GameSystemsCoordinator<T, S...>::GetCoordinator()
    {
        return mCoordinatorSet.template GetCoordinator<C>();
    }

    template <typename T, typename ... S>
    template <typename C>
    const comp::Coordinator<C>& GameSystemsCoordinator<T, S...>::GetCoordinator() const
    {
        return mCoordinatorSet.template GetCoordinator<C>();
    }


    //int RunApplication()
}
