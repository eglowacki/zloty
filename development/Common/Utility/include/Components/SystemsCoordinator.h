//////////////////////////////////////////////////////////////////////
// SystemsCoordinator.h
//
//  Copyright 10/19/2020 Edgar Glowacki
//
//  Maintained by: Edgar
//
//  NOTES:
//      Replaces functionality of GameCoordinator, but serves similar purpose
//
//
//  #include "Components/SystemsCoordinator.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "CoordinatorSet.h"
#include "App/Application.h"

namespace yaget {
    namespace metrics {
        class Channel;
    }
}


namespace yaget::comp::gs
{
    // Create coordinator for system and call each for update
    // T is GameCoordinatorSet and ...S are systems (classes that follow GameSystem)
    template <typename T, typename... S>
    class SystemsCoordinator
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

    namespace internal
    {
        template <typename T>
        struct Updater
        {
            void operator()(const time::GameClock& gameClock, metrics::Channel& channel);
            std::shared_ptr<T> mSystemsCoordinator;
        };

    }

    // Helper to create game and render coordinators, systems and connect to app and run it
    // It creates each Coordinator on the thread that will be run. In this case we have
    // 2 threads, logic and render
    template <typename TG, typename TR, typename A>
    int RunGame(A& app)
    {
        return app.Run(internal::Updater<TG>(), internal::Updater<TR>());
    }

}


//#include "CoordinatorSetImplementation.h"
namespace yaget::comp::gs
{
    template <typename T, typename... S>
    void SystemsCoordinator<T, S...>::Tick(const time::GameClock& gameClock, metrics::Channel& channel)
    {
        // possibly run each system on own thread, taking Policy (usage) into account
        meta::for_each(mSystems, [this, &gameClock, &channel](auto& gameSystem)
        {
            gameSystem.Tick(mCoordinatorSet, gameClock, channel);
        });
    }

    template <typename T, typename ... S>
    template <typename C>
    comp::Coordinator<C>& SystemsCoordinator<T, S...>::GetCoordinator()
    {
        return mCoordinatorSet.template GetCoordinator<C>();
    }

    template <typename T, typename ... S>
    template <typename C>
    const comp::Coordinator<C>& SystemsCoordinator<T, S...>::GetCoordinator() const
    {
        return mCoordinatorSet.template GetCoordinator<C>();
    }

    template <typename T>
    void internal::Updater<T>::operator()(const time::GameClock& gameClock, metrics::Channel& channel)
    {
        if (!mSystemsCoordinator)
        {
            mSystemsCoordinator = std::make_shared<T>();
        }

        mSystemsCoordinator->Tick(gameClock, channel);
    }


    //int RunApplication()
}
