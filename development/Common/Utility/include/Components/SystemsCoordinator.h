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

#include "Components/CoordinatorSet.h"
#include "App/Application.h"

namespace yaget::metrics {
    class Channel;
}


namespace yaget::comp::gs
{
    // Create coordinator for system and call each for update
    // T is GameCoordinatorSet and ...S are Systems (classes that follow yaget::comp::gs::GameSystem)
    template <typename T, typename M, typename... S>
    class SystemsCoordinator
    {
    public:
        using CoordinatorSet = T;
        using Messaging = M;

        SystemsCoordinator(M& messaging);

        void Tick(const time::GameClock& gameClock, metrics::Channel& channel);

        template <typename C>
        comp::Coordinator<C>& GetCoordinator();

        template <typename C>
        const comp::Coordinator<C>& GetCoordinator() const;

    private:
        using Systems = std::tuple<std::shared_ptr<S>...>;

        Messaging& mMessaging;
        CoordinatorSet mCoordinatorSet;
        Systems mSystems;
    };

    namespace internal
    {
        template <typename T, typename M>
        struct Updater
        {
            Updater(M& messaging)
                : mMessaging(messaging)
            {}

            void operator()(const time::GameClock& gameClock, metrics::Channel& channel);
            std::shared_ptr<T> mSystemsCoordinator;

            M& mMessaging;
        };

    }

    // Helper to create game and render coordinator systems, connect to app and run it.
    // It creates each Coordinator on the thread that it's run and Tick is called from.
    // In this case we have 2 threads, logic and render.
    template <typename TG, typename TR, typename M, typename A>
    int RunGame(M& messaging, A& app)
    {
        return app.Run(internal::Updater<TG, M>(messaging), internal::Updater<TR, M>(messaging));
    }

}


//#include "Components/SystemsCoordinatorImplementation.h"
template <typename T, typename M, typename... S>
yaget::comp::gs::SystemsCoordinator<T, M, S...>::SystemsCoordinator(M& messaging)
    : mMessaging(messaging)
{
    meta::for_each(mSystems, [this]<typename T0>(T0& system)
    {
        using BaseType = T0;
        using SystemType = typename BaseType::element_type;
        system = std::make_shared<SystemType>(mMessaging);
    });
}

template <typename T, typename M, typename... S>
void yaget::comp::gs::SystemsCoordinator<T, M, S...>::Tick(const time::GameClock& gameClock, metrics::Channel& channel)
{
    // possibly run each system on own thread, taking Policy (usage) into account
    meta::for_each(mSystems, [this, &gameClock, &channel](auto& gameSystem)
    {
        gameSystem->Tick(mCoordinatorSet, gameClock, channel);
    });
}

template <typename T, typename M, typename... S>
template <typename C>
yaget::comp::Coordinator<C>& yaget::comp::gs::SystemsCoordinator<T, M, S...>::GetCoordinator()
{
    return mCoordinatorSet.template GetCoordinator<C>();
}

template <typename T, typename M, typename... S>
template <typename C>
const yaget::comp::Coordinator<C>& yaget::comp::gs::SystemsCoordinator<T, M, S...>::GetCoordinator() const
{
    return mCoordinatorSet.template GetCoordinator<C>();
}

// on the first call, we'll create system coordinator. This ensures it get's created on the same
// thread as it get's to call Tick on.
template <typename T, typename M>
void yaget::comp::gs::internal::Updater<T, M>::operator()(const time::GameClock& gameClock, metrics::Channel& channel)
{
    if (!mSystemsCoordinator)
    {
        mSystemsCoordinator = std::make_shared<T>(mMessaging);
    }

    mSystemsCoordinator->Tick(gameClock, channel);
}