//////////////////////////////////////////////////////////////////////
// GameCoordinator.h
//
//  Copyright 7/5/2019 Edgar Glowacki
//
//  Maintained by: Edgar
//
//  NOTES:
//      
//
//
//  #include "Components/GameCoordinator.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "Components/Coordinator.h"


namespace yaget
{
    namespace time { class GameClock; }
    namespace metrics { class Channel; }

    namespace comp
    {
        // Used in GameCoordinator to differentiate between regular entities and global ones
        template <typename T, typename G>
        struct CoordinatorPolicy
        {
            using Entity = T;
            using Global = G;
        };

    } // namespace comp

    namespace internal
    {
        //template <typename T>
        //struct HasSetCoordinators
        //{
        //    enum { value = false };
        //};

        template<typename T>
        concept has_set_coordinators = requires { typename T::set_coordinators; };
    }

    //template <class T> struct HasSetCoordinator
    //{
    //    // We test if the type has serialize using decltype and declval.
    //    template <typename C>
    //    static constexpr decltype(std::declval<C>().SetCoordinator(), bool()) test(int /* unused */)
    //    {
    //        // We can return values, thanks to constexpr instead of playing with sizeof.
    //        return true;
    //    }

    //    template <typename C>
    //    static constexpr bool test(...)
    //    {
    //        return false;
    //    }

    //    // int is used to give the precedence!
    //    static constexpr bool value = test<T>(int());
    //};


    class IGameCoordinator
    {
    public:
        static constexpr int GLOBAL_ID = 0;
        static constexpr int ENTITY_ID = 1;

        virtual void GameInitialize(const time::GameClock& /*gameClock*/, metrics::Channel& /*channel*/) = 0;
        virtual void GameUpdate(const time::GameClock& /*gameClock*/, metrics::Channel& /*channel*/) = 0;
        virtual void GameShutdown(const time::GameClock& /*gameClock*/, metrics::Channel& /*channel*/) = 0;
        virtual void RenderUpdate(const time::GameClock& /*gameClock*/, metrics::Channel& /*channel*/) = 0;
    };


    // Helper types to provide two basic types of how the ForEach loop in GameSystem is treated in respect
    // to end of the list, and which coordinator to use
    using NoEndMarkerGlobal = comp::gs::EndMarkerNo<IGameCoordinator::GLOBAL_ID>;
    using EndMarkerEntity = comp::gs::EndMarkerYes<IGameCoordinator::ENTITY_ID>;
    using NoEndMarkerEntity = comp::gs::EndMarkerNo<IGameCoordinator::ENTITY_ID>;

    //std::forward<Args>(args)...
    // P - CoordinatorPolicy
    // S - GameSystem
    template <typename P, typename... S>
    class GameCoordinator : public IGameCoordinator
    {
    public:
        using Entity = typename P::Entity;
        using Global = typename P::Global;
        using Systems = std::tuple<S...>;
        using RenderCallback = std::function<void(const time::GameClock& /*gameClock*/, metrics::Channel& /*channel*/)>;

        using GlobalCoordinator = comp::Coordinator<Global>;
        using EntityCoordinator = comp::Coordinator<Entity>;
        using Coordinators = std::tuple<GlobalCoordinator, EntityCoordinator>;

        GameCoordinator(RenderCallback renderCallback, S... args) 
            : mRenderCallback(renderCallback)
            , mSystems(args...)
        {
            meta::for_each_type<typename Entity::Row>([]<typename T0>(const T0&)
            {
                typename Global::Row row{};
                YAGET_ASSERT(!meta::check_for_type<T0>(row), "You can not have same types of row property between Global and Entity Coordinators. Type: '%s'.", meta::ViewToString(meta::type_name<T0>()).c_str());
            });

            meta::for_each_type<typename Global::Row>([]<typename T0>(const T0&)
            {
                typename Entity::Row row{};
                YAGET_ASSERT(!meta::check_for_type<T0>(row), "You can not have same types of row property between Entity and Global Coordinators. Type: '%s'.", meta::ViewToString(meta::type_name<T0>()).c_str());
            });
        }

        GameCoordinator(S... args)
            : GameCoordinator([](const time::GameClock&, metrics::Channel&) {}, args...)
        {}

        //template<typename T>
        //comp::Coordinator<T>& GetCoordinator()
        //{
        //    return std::get<comp::Coordinator<T>>(mCoordinators);
        //}

        void GameInitialize(const time::GameClock& gameClock, metrics::Channel& channel) override;
        void GameUpdate(const time::GameClock& gameClock, metrics::Channel& channel) override;
        void GameShutdown(const time::GameClock& gameClock, metrics::Channel& channel) override;
        void RenderUpdate(const time::GameClock& gameClock, metrics::Channel& channel) override;

    private:
        Coordinators mCoordinators;

        RenderCallback mRenderCallback;
        Systems mSystems;
    };

} // namespace yaget


template <typename P, typename... S>
void yaget::GameCoordinator<P, S...>::GameInitialize(const time::GameClock& gameClock, metrics::Channel& channel)
{
    meta::for_each(mSystems, [this, &gameClock, &channel](auto gameSystem)
    {
        if (gameSystem)
        {
            using GS = std::decay_t<decltype(*gameSystem)>;

            if constexpr (internal::has_set_coordinators<GS>)
            {
                using gs_type = typename std::tuple_element<gameSystem->COORDINATOR_ID, decltype(mCoordinators)>::type;

                gameSystem->SetCoordinators(gameClock, channel, std::get<GLOBAL_ID>(mCoordinators), std::get<ENTITY_ID>(mCoordinators));
                gameSystem->Initialize<gs_type::Policy>(gameClock, channel);
            }
            else
            {
                gameSystem->Initialize(gameClock, channel, std::get<gameSystem->COORDINATOR_ID>(mCoordinators));
            }
        }
    });
}

template <typename P, typename... S>
void yaget::GameCoordinator<P, S...>::GameUpdate(const time::GameClock& gameClock, metrics::Channel& channel)
{
    meta::for_each(mSystems, [this, &gameClock, &channel](auto gameSystem)
    {
        if (gameSystem)
        {
            gameSystem->Update(gameClock, channel, std::get<gameSystem->COORDINATOR_ID>(mCoordinators));
        }
    });
}

template <typename P, typename... S>
void yaget::GameCoordinator<P, S...>::GameShutdown(const time::GameClock& gameClock, metrics::Channel& channel)
{
    meta::for_each(mSystems, [this, &gameClock, &channel](auto gameSystem)
    {
        if (gameSystem)
        {
            using GS = std::decay_t<decltype(*gameSystem)>;

            if constexpr (internal::has_set_coordinators<GS>)
            {
                using gs_type = typename std::tuple_element<gameSystem->COORDINATOR_ID, decltype(mCoordinators)>::type;

                gameSystem->Shutdown<gs_type::Policy>(gameClock, channel);
            }
            else
            {
                gameSystem->Shutdown(gameClock, channel, std::get<gameSystem->COORDINATOR_ID>(mCoordinators));
            }
        }
    });
}

template <typename P, typename... S>
void yaget::GameCoordinator<P, S...>::RenderUpdate(const time::GameClock& gameClock, metrics::Channel& channel)
{
    mRenderCallback(gameClock, channel);
}

