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

#include "YagetCore.h"
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


    class IGameCoordinator
    {
    public:
        static constexpr int GLOBAL_ID = 0;
        static constexpr int ENTITY_ID = 1;

        virtual void GameInitialize(const time::GameClock& /*gameClock*/, metrics::Channel& /*channel*/) = 0;
        virtual void GameUpdate(const time::GameClock& /*gameClock*/, metrics::Channel& /*channel*/) = 0;
        virtual void RenderUpdate(const time::GameClock& /*gameClock*/, metrics::Channel& /*channel*/) = 0;
    };


    // Helper types to provide two basic types of how the ForEach loop in GameSystem is treated in respect
    // to end if the list, and which coordinator to use
    using NoEndMarkerGlobal = comp::gs::EndMarkerNo<IGameCoordinator::GLOBAL_ID>;
    using EndMarkerEntity = comp::gs::EndMarkerYes<IGameCoordinator::ENTITY_ID>;
    using NoEndMarkerEntity = comp::gs::EndMarkerNo<IGameCoordinator::ENTITY_ID>;

    //std::forward<Args>(args)...
    // P - CoordinatorPolicy
    // S - GameSytems
    template <typename P, typename... S>
    class GameCoordinator : public IGameCoordinator
    {
    public:
        using Entity = typename P::Entity;
        using Global = typename P::Global;
        using Systems = std::tuple<S...>;
        using RenderCallback = std::function<void(const time::GameClock& /*gameClock*/, metrics::Channel& /*channel*/)>;

        GameCoordinator(RenderCallback renderCallback, S... args) 
            : mRenderCallback(renderCallback)
            , mSystems(args...)
        {}

        GameCoordinator(S... args)
            : GameCoordinator([](const time::GameClock&, metrics::Channel&) {}, args...)
        {}

        template<typename T>
        comp::Coordinator<T>& GetCoordinator()
        {
            return std::get<comp::Coordinator<T>>(mCoordinators);
        }

        void GameInitialize(const time::GameClock& gameClock, metrics::Channel& channel) override;
        void GameUpdate(const time::GameClock& gameClock, metrics::Channel& channel) override;
        void RenderUpdate(const time::GameClock& gameClock, metrics::Channel& channel) override;

    private:
        using GlobalCoordinator = comp::Coordinator<Global>;
        using EntityCoordinator = comp::Coordinator<Entity>;
        using Coordinators = std::tuple<GlobalCoordinator, EntityCoordinator>;

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
            gameSystem->Initialize(gameClock, channel, std::get<gameSystem->COORDINATOR_ID>(mCoordinators));
            //gameSystem->Update(gameClock, channel, std::get<gameSystem->COORDINATOR_ID>(mCoordinators));
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
void yaget::GameCoordinator<P, S...>::RenderUpdate(const time::GameClock& gameClock, metrics::Channel& channel)
{
    mRenderCallback(gameClock, channel);
}
