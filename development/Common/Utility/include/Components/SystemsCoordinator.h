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


namespace yaget::metrics { class Channel; }

namespace yaget::comp::gs
{
    //-------------------------------------------------------------------------------------------------
    // Create coordinator for system and call each for update
    // T is GameCoordinatorSet and ...S are Systems (classes that follow yaget::comp::gs::GameSystem)
    // M is Messaging and A represents Application
    template <typename T, typename M, typename A, typename... S>
    class SystemsCoordinator
    {
    public:
        using CoordinatorSet = T;
        using Messaging = M;
        using Systems = std::tuple<S*...>;

        SystemsCoordinator(M& messaging, A& app);

        void Tick(const time::GameClock& gameClock, metrics::Channel& channel);

        template <typename C>
        comp::Coordinator<C>& GetCoordinator();

        template <typename C>
        const comp::Coordinator<C>& GetCoordinator() const;

        template <typename SY>
        SY& GetSystem();

        template <typename SY>
        const SY& GetSystem() const;

        template <typename CT, typename... Args>
        CT* AddComponent(comp::Id_t id, Args&&... args);

    private:
        using ManagedSystems = std::tuple<std::shared_ptr<S>...>;

        Messaging& mMessaging;
        CoordinatorSet mCoordinatorSet;
        ManagedSystems mSystems;
    };

    namespace internal
    {
        //-------------------------------------------------------------------------------------------------
        template <typename T, typename M, typename A>
        struct Updater
        {
            Updater(M& messaging, A& application)
                : mMessaging(messaging)
                , mApplication(application)
            {}

            void operator()(const time::GameClock& gameClock, metrics::Channel& channel);

            std::shared_ptr<T> mSystemsCoordinator;
            M& mMessaging;
            A& mApplication;
        };

    } // namespace internal

    //-------------------------------------------------------------------------------------------------
    // Helper to create game and render coordinator systems, connect to app and run it.
    // It creates each Coordinator on the thread that it's run and Tick is called from.
    // In this case we have 2 threads, logic and render.
    template <typename TG, typename TR, typename M, typename A>
    int RunGame(M& messaging, A& app)
    {
        return app.Run(internal::Updater<TG, M, A>(messaging, app), internal::Updater<TR, M, A>(messaging, app));
    }

    template <typename TG, typename M, typename A>
    int RunGame(M& messaging, A& app)
    {
        return app.Run(internal::Updater<TG, M, A>(messaging, app));
    }

} // namespace yaget::comp::gs


//-------------------------------------------------------------------------------------------------
template <typename T, typename M, typename A, typename... S>
yaget::comp::gs::SystemsCoordinator<T, M, A, S...>::SystemsCoordinator(M& messaging, A& app)
    : mMessaging(messaging)
{
    meta::for_each(mSystems, [this, &app]<typename T0>(T0& system)
    {
        using BaseType = T0;
        using SystemType = typename BaseType::element_type;
        system = std::make_shared<SystemType>(mMessaging, app, mCoordinatorSet);
    });
}


//-------------------------------------------------------------------------------------------------
template <typename T, typename M, typename A, typename... S>
void yaget::comp::gs::SystemsCoordinator<T, M, A, S...>::Tick(const time::GameClock& gameClock, metrics::Channel& channel)
{
    // possibly run each system on own thread, taking Policy (usage) into account
    meta::for_each(mSystems, [this, &gameClock, &channel](auto& gameSystem)
    {
        const auto& message = fmt::format("System Tick {}", gameSystem->NiceName());
        metrics::Channel systemChannel(message, YAGET_METRICS_CHANNEL_FILE_LINE);

        gameSystem->Tick(gameClock, channel);
    });
}


//-------------------------------------------------------------------------------------------------
template <typename T, typename M, typename A, typename... S>
template <typename C>
yaget::comp::Coordinator<C>& yaget::comp::gs::SystemsCoordinator<T, M, A, S...>::GetCoordinator()
{
    return mCoordinatorSet.template GetCoordinator<C>();
}


//-------------------------------------------------------------------------------------------------
template <typename T, typename M, typename A, typename... S>
template <typename C>
const yaget::comp::Coordinator<C>& yaget::comp::gs::SystemsCoordinator<T, M, A, S...>::GetCoordinator() const
{
    return mCoordinatorSet.template GetCoordinator<C>();
}


//-------------------------------------------------------------------------------------------------
template <typename T, typename M, typename A, typename... S>
template <typename SY>
SY& yaget::comp::gs::SystemsCoordinator<T, M, A, S...>::GetSystem()
{
    return *std::get< std::shared_ptr<SY>>(mSystems).get();
}


//-------------------------------------------------------------------------------------------------
template <typename T, typename M, typename A, typename... S>
template <typename SY>
const SY& yaget::comp::gs::SystemsCoordinator<T, M, A, S...>::GetSystem() const
{
    return *std::get< std::shared_ptr<SY>>(mSystems).get();
}


//-------------------------------------------------------------------------------------------------
template <typename T, typename M, typename A, typename ... S>
template <typename CT, typename... Args>
CT* yaget::comp::gs::SystemsCoordinator<T, M, A, S...>::AddComponent(comp::Id_t id, Args&&... args)
{
    CT* component{};
    meta::for_each(mSystems, [&]<typename T0>(T0& system)
    {
        if (!component)
        {
            component = system->template AddComponent<CT>(id, std::forward<Args>(args)...);
        }
    });

    return component;
}


//-------------------------------------------------------------------------------------------------
// on the first call, we'll create system coordinator. This ensures it get's created on the same
// thread as it get's to call Tick on.
template <typename T, typename M, typename A>
void yaget::comp::gs::internal::Updater<T, M, A>::operator()(const time::GameClock& gameClock, metrics::Channel& channel)
{
    if (!mSystemsCoordinator)
    {
        mSystemsCoordinator = std::make_shared<T>(mMessaging, mApplication);
    }

    mSystemsCoordinator->Tick(gameClock, channel);
}