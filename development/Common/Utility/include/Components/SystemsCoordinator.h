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
        using CoordinatorSet = T;
        using Messaging = M;

        using Systems = std::tuple<S...>;
        static constexpr size_t NumSystems = std::tuple_size_v<std::remove_reference_t<Systems>>;
        static_assert(NumSystems, "User must provide at least 1 GameSystem");
        static_assert(internal::SystemsMatchCoordinator<Systems, CoordinatorSet>(), "CoordinatorSet does not support requested GameSystem component.");

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

        template <typename TT = typename CoordinatorSet::FullRow>
        TT LoadItem(comp::Id_t id);

        template <typename TT = typename CoordinatorSet::FullRow>
        bool RemoveItem(comp::Id_t id);

        items::Director& Director() { return mApp.Director(); }
        const items::Director& Director() const { return mApp.Director(); }

        // Returns a reference to a specific game system (const and non-const versions)
        template <typename GS>
        const GS& GetGameSystem() const { return *std::get<std::shared_ptr<GS>>(mSystems); }

        template <typename GS>
        GS& GetGameSystem() { return *std::get<std::shared_ptr<GS>>(mSystems); }

        // Save component persistent data to DB
        bool PersistComponent(yaget::comp::Id_t id, const std::string& componentType, const std::string& params);
        // Returns true of componentName does exist, otherwise false
        bool IsComponentTyped(const std::string& componentName) const;

    private:
        using ManagedSystems = std::tuple<std::shared_ptr<S>...>;

        Messaging& mMessaging;
        A& mApp;
        CoordinatorSet mCoordinatorSet;
        ManagedSystems mSystems;

        using CreationFunction = std::function<void(yaget::comp::Id_t id, const std::string& params)>;
        std::map<std::string, CreationFunction> mCreationFunctions;
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

    template <typename TG, typename M, typename A>
    int RunGame(M& messaging, A& app, auto renderCallback)
    {
        return app.Run(internal::Updater<TG, M, A>(messaging, app), renderCallback);
    }

} // namespace yaget::comp::gs


//-------------------------------------------------------------------------------------------------
template <typename T, typename M, typename A, typename... S>
yaget::comp::gs::SystemsCoordinator<T, M, A, S...>::SystemsCoordinator(M& messaging, A& app)
    : mMessaging(messaging)
    , mApp(app)
    , mCoordinatorSet(&app.Director())
{
    meta::for_loop<CoordinatorSet::FullRow>([this]<std::size_t T0>()
    {
        constexpr std::size_t index = T0;
        using ComponentType = meta::strip_qualifiers_t<std::tuple_element_t<index, typename CoordinatorSet::FullRow>>;

        if constexpr (has_component_types<ComponentType>)
        {
            const auto componentName = db::ResolveName<ComponentType>();
            using ParamTypes = typename ComponentType::Types;

            mCreationFunctions[componentName] = [this](Id_t id, [[maybe_unused]]const std::string& params)
            {
                auto componentParams = yaget::conv::Convertor<ParamTypes>::FromString(params.c_str());
                ComponentType* component = std::apply([this, id, &componentParams](auto&&... params)
                {
                    return AddComponent<ComponentType>(id, params...);

                }, componentParams);

                if (component)
                {
                    SaveComponent(component);
                    component = nullptr;
                    // since we saved component data, component itself is no longer needed
                    RemoveComponent<ComponentType>(id);
                }
            };
        }
        else
        {
            if constexpr (std::is_constructible_v<ComponentType, comp::Id_t>)
            {
                // let's add non-persistent component, only if we can be constructed with Id as parameter to ctor
                const auto componentName = db::ResolveName<ComponentType>();
                mCreationFunctions[componentName] = [this]([[maybe_unused]]Id_t id, [[maybe_unused]]const std::string&)
                {
                    if (ComponentType* component = AddComponent<ComponentType>(id))
                    {
                    //    SaveComponent(component);
                    //    component = nullptr;
                    //    // since we saved component data, component itself is no longer needed
                    //    RemoveComponent<ComponentType>(id);
                    }
                };
            }
        }
    });

    auto This = this;
    meta::for_loop<ManagedSystems>([This, this]<std::size_t T0>()
    {
        using BaseType = std::tuple_element_t<T0, ManagedSystems>;
        using SystemType = typename BaseType::element_type;

        auto& system = std::get<T0>(mSystems);
        system = std::make_shared<SystemType>(mMessaging, mApp, mCoordinatorSet);
    });
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
template <typename T, typename M, typename A, typename ... S>
bool yaget::comp::gs::SystemsCoordinator<T, M, A, S...>::PersistComponent(yaget::comp::Id_t id, const std::string& componentType, const std::string& params)
{
    if (auto it = mCreationFunctions.find(componentType); it != mCreationFunctions.end())
    {
        auto& creationFunction = it->second;
        creationFunction(id, params);

        return true;
    }

    return false;
}


//-------------------------------------------------------------------------------------------------
template <typename T, typename M, typename A, typename ... S>
bool yaget::comp::gs::SystemsCoordinator<T, M, A, S...>::IsComponentTyped(const std::string& componentName) const
{
    return mCreationFunctions.contains(componentName);
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