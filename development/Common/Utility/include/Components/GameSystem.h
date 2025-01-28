//////////////////////////////////////////////////////////////////////
// GameSystem.h
//
//  Copyright 6/22/2019 Edgar Glowacki
//
//  Maintained by: Edgar
//
//  NOTES:
//      Class to iterate over set of components per item/entity level
//      Provides callback methods with specific component type parameters,
//      including const and non-cont qualifiers.
//
//
//  #include "Components/GameSystem.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "Components/ComponentTypes.h"
#include "Components/Coordinator.h"
#include "Metrics/Concurrency.h"
#include <functional>

namespace yaget
{
    class Application;
    namespace metrics { class Channel; }
    namespace time { class GameClock; }
}

namespace yaget::comp::gs
{
    struct NoEndMarker {};
    struct GenerateEndMarker {};

    // TODO add timers support for entities
    // https://github.com/eglowacki/zloty/issues/44#issue-1174664933
    // Example:
    //  class ScoreSystem : public yaget::comp::gs::GameSystem<NoEndMarker, Messaging, ScoreComponent*>
    //  {
    //  public:
    //      ScoreSystem(Messaging& messaging)
    //          : GameSystem("ScoreSystem", messaging, [this](auto&&... params) {OnUpdate(params...); })
    //      {}
    //
    //  private:
    //      void OnUpdate(yaget::comp::Id_t id, const yaget::time::GameClock& gameClock, yaget::metrics::Channel& channel, ScoreComponent* boardComponent);
    //};
    //
    template <typename CS, typename E, typename M, typename... Comps>
    class GameSystem : public Noncopyable<GameSystem<CS, E, M, Comps...>>
    {
    public:
        using EndMarker = E;
        using Messaging = M;
        using RowPolicy = comp::RowPolicy<Comps...>;
        using Row = typename RowPolicy::Row;
        using CoordinatorSet = CS;

        using UpdateFunctor = std::function<void(yaget::comp::Id_t id, const time::GameClock& gameClock, metrics::Channel& channel, Comps&... args)>;

        // framework calls this on same cadence (every tick...)
        // In default case that is SystemsCoordinator class
        void Tick(const time::GameClock& gameClock, metrics::Channel& channel);

        const char* NiceName() const { return mNiceName; }

        template <typename C>
        comp::Coordinator<C>& GetCoordinator();

        template <typename C>
        const comp::Coordinator<C>& GetCoordinator() const;

        template <typename CT, typename... Args>
        CT* AddComponent(comp::Id_t id, Args&&... args);

        template <typename C, typename CT>
        void RemoveComponent(comp::Id_t id);

        template <typename C>
        void RemoveComponents(comp::Id_t id);

        ~GameSystem();

    protected:
        GameSystem(const char* niceName, Messaging& messaging, Application& app, UpdateFunctor updateFunctor, CS& coordinatorSet);

        Messaging& mMessaging;

    private:
        void Update(yaget::comp::Id_t id, const time::GameClock& gameClock, metrics::Channel& channel, const Row& row);

        const char* mNiceName = nullptr;
        UpdateFunctor mUpdateFunctor;
        CoordinatorSet& mCoordinatorSet;
    };

} // namespace yaget::comp::gs

#define YAGET_GAME_SYSTEM_INCLUDE_IMPLEMENTATION
#include "Components/GameSystemInline.h"
#undef YAGET_GAME_SYSTEM_INCLUDE_IMPLEMENTATION
