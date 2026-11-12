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

#include "App/Application.h"
#include "Components/CoordinatorSet.h"
#include "Meta/CompilerAlgo.h"


namespace yaget::metrics { class Channel; }

namespace yaget::comp::gs
{
    namespace internal
    {

        template <typename S, typename R>
        constexpr bool SystemsMatchCoordinator()
        {
            bool result = true;
            meta::for_loop<S>([&result]<std::size_t T0>()
            {
                using BaseSystemType = std::tuple_element_t<T0, S>;
                using SystemRow = typename BaseSystemType::Row;

                if constexpr (std::tuple_size_v<std::remove_reference_t<SystemRow>> == 0)
                {
                    // if system does not have any components, then it can be used with any coordinator
                    return;
                }

                using RequestedRow = tuple_get_union_t<SystemRow, typename R::FullRow>;

                if (result)
                {
                    result = std::tuple_size_v<RequestedRow> > 0;
                }
            });

            return result;
        }
    }


    //-------------------------------------------------------------------------------------------------
    // Create coordinator for system and call each for update
    // T is GameCoordinatorSet and ...S are Systems (classes that follow yaget::comp::gs::GameSystem)
    // M is Messaging and A represents Application
    template <typename T, typename M, typename A, typename... S>
    class SystemsCoordinator
    {
    public:
        using CoordinatorSet_t = T;
        using Messaging = M;

        using Systems = std::tuple<S...>;
        static constexpr size_t NumSystems = std::tuple_size_v<std::remove_reference_t<Systems>>;
        static_assert(NumSystems, "User must provide at least 1 GameSystem");
        static_assert(internal::SystemsMatchCoordinator<Systems, CoordinatorSet_t>(), "CoordinatorSet_t does not support requested GameSystem component.");

        struct GetItemPattern;
        using PatternFunction = GetItemPattern;

        SystemsCoordinator(M& messaging, A& app);

        void Tick(const time::GameClock& gameClock, metrics::Channel& channel);

        template <typename C, typename... Args>
        C* AddComponent(comp::Id_t id, Args&&... args);

        template <typename C>
        C* LoadComponent(comp::Id_t id);

        template <typename C>
        bool SaveComponent(const C* component);

        template <typename C>
        C* FindComponent(comp::Id_t id) const;

        template <typename C>
        bool RemoveComponent(comp::Id_t id);

        template <typename TT = typename CoordinatorSet_t::FullRow>
        TT LoadItem(comp::Id_t id);

        template <typename TT = typename CoordinatorSet_t::FullRow>
        bool RemoveItem(comp::Id_t id);

        items::Director& Director() { return mApp.Director(); }
        const items::Director& Director() const { return mApp.Director(); }

        // Returns a reference to a specific game system (const and non-const versions)
        template <typename GS>
        const GS& GetGameSystem() const { return *std::get<std::shared_ptr<GS>>(mSystems); }

        template <typename GS>
        GS& GetGameSystem() { return *std::get<std::shared_ptr<GS>>(mSystems); }

        CoordinatorSet_t& GetCoordinatorSet() { return mCoordinatorSet; }
        const CoordinatorSet_t& GetCoordinatorSet() const { return mCoordinatorSet; }

    protected:
        Messaging& mMessaging;
        A& mApp;

    private:
        using ManagedSystems = std::tuple<std::shared_ptr<S>...>;

        CoordinatorSet_t mCoordinatorSet;
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
    int RunGame(M& messaging, A& app, typename A::TickIdle tickIdle = {})
    {
        return app.Run(internal::Updater<TG, M, A>(messaging, app), internal::Updater<TR, M, A>(messaging, app), tickIdle);
    }

    template <typename TG, typename M, typename A>
    int RunGame(M& messaging, A& app, typename A::TickIdle tickIdle = {})
    {
        return app.Run(internal::Updater<TG, M, A>(messaging, app), tickIdle);
    }

    template <typename TG, typename M, typename A>
    int RunGame(M& messaging, A& app, auto renderCallback, typename A::TickIdle tickIdle = {})
    {
        return app.Run(internal::Updater<TG, M, A>(messaging, app), renderCallback, tickIdle);
    }

} // namespace yaget::comp::gs


//-------------------------------------------------------------------------------------------------
template <typename T, typename M, typename A, typename... S>
yaget::comp::gs::SystemsCoordinator<T, M, A, S...>::SystemsCoordinator(M& messaging, A& app)
    : mMessaging(messaging)
    , mApp(app)
    , mCoordinatorSet(&app.Director())
{
    meta::for_loop<ManagedSystems>([this]<std::size_t T0>()
    {
        using BaseType = std::tuple_element_t<T0, ManagedSystems>;
        using SystemType = typename BaseType::element_type;

        auto& system = std::get<T0>(mSystems);
        system = std::make_shared<SystemType>(mMessaging, mApp, mCoordinatorSet);
    });

    constexpr bool autoCreateGlobalComponents = true;
    if constexpr (autoCreateGlobalComponents)
    {
        using GlobalComponentsRow = metatypes::global_components_row_t<SystemsCoordinator>;

        LoadItem<GlobalComponentsRow>(comp::GLOBAL_ID_MARKER);
    }
}


//-------------------------------------------------------------------------------------------------
template <typename T, typename M, typename A, typename... S>
void yaget::comp::gs::SystemsCoordinator<T, M, A, S...>::Tick(const time::GameClock& gameClock, metrics::Channel& channel)
{
    // possibly run each system on own thread, taking Policy (usage) into account
    meta::for_each(mSystems, [this, &gameClock, &channel](auto& gameSystem)
    {
        const auto& message = std::format("System Tick {}", gameSystem->NiceName());
        metrics::Channel systemChannel(message);

        gameSystem->Tick(gameClock, channel);
    });
}


//-------------------------------------------------------------------------------------------------
template <typename T, typename M, typename A, typename ... S>
template <typename C, typename... Args>
C* yaget::comp::gs::SystemsCoordinator<T, M, A, S...>::AddComponent(comp::Id_t id, Args&&... args)
{
    return mCoordinatorSet.template AddComponent<C>(id, std::forward<Args>(args)...);
}


//-------------------------------------------------------------------------------------------------
template <typename T, typename M, typename A, typename ... S>
template <typename C>
C* yaget::comp::gs::SystemsCoordinator<T, M, A, S...>::LoadComponent(comp::Id_t id)
{
    return mCoordinatorSet.template LoadComponent<C>(id);
}


//-------------------------------------------------------------------------------------------------
template <typename T, typename M, typename A, typename ... S>
template <typename C>
bool yaget::comp::gs::SystemsCoordinator<T, M, A, S...>::SaveComponent(const C* component)
{
    return mCoordinatorSet.template SaveComponent<C>(component);
}


//-------------------------------------------------------------------------------------------------
template <typename T, typename M, typename A, typename ... S>
template <typename C>
C* yaget::comp::gs::SystemsCoordinator<T, M, A, S...>::FindComponent(comp::Id_t id) const
{
    return mCoordinatorSet.template FindComponent<C>(id);
}


//-------------------------------------------------------------------------------------------------
template <typename T, typename M, typename A, typename ... S>
template <typename C>
bool yaget::comp::gs::SystemsCoordinator<T, M, A, S...>::RemoveComponent(comp::Id_t id)
{
    return mCoordinatorSet.template RemoveComponent<C>(id);
}


//-------------------------------------------------------------------------------------------------
template <typename T, typename M, typename A, typename ... S>
template <typename TT>
TT yaget::comp::gs::SystemsCoordinator<T, M, A, S...>::LoadItem(comp::Id_t id)
{
    return mCoordinatorSet.template LoadItem<TT>(id);
}


//-------------------------------------------------------------------------------------------------
template <typename T, typename M, typename A, typename ... S>
template <typename TT>
bool yaget::comp::gs::SystemsCoordinator<T, M, A, S...>::RemoveItem(comp::Id_t id)
{
    return mCoordinatorSet.template RemoveItem<TT>(id);
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
