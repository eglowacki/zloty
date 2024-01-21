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
#include <fmt/format.h>


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
    class GameSystem : public Noncopyable<GameSystem<E, M, Comps...>>
    {
    public:
        using EndMarker = E;
        using Messaging = M;
        using RowPolicy = comp::RowPolicy<Comps...>;
        using Row = typename RowPolicy::Row;

        using UpdateFunctor = std::function<void(yaget::comp::Id_t id, const time::GameClock& gameClock, metrics::Channel& channel, Comps&... args)>;

        // framework calls this on same cadence (every tick...)
        // In default case that is SystemsCoordinator class
        void Tick(const time::GameClock& gameClock, metrics::Channel& channel)
        {
            mCoordinatorSet.template ForEach<Row>([this, &gameClock, &channel](yaget::comp::Id_t id, const auto& row)
            {
                Update(id, gameClock, channel, row);
                return true;
            });

            if constexpr (std::is_same_v<EndMarker, GenerateEndMarker>)
            {
                Update(END_ID_MARKER, gameClock, channel, {});
            }
        }

        const char* NiceName() const { return mNiceName; }

        template <typename C>
        comp::Coordinator<C>& GetCoordinator()
        {
            return mCoordinatorSet.template GetCoordinator<C>();
        }

        template <typename C>
        const comp::Coordinator<C>& GetCoordinator() const
        {
            return mCoordinatorSet.template GetCoordinator<C>();
        }

        //CS& CoordinatorSet()
        //{
        //    return mCoordinatorSet;
        //}

        //const CS& CoordinatorSet() const
        //{
        //    return mCoordinatorSet;
        //}

    protected:
        GameSystem(const char* niceName, Messaging& messaging, Application& /*app*/, UpdateFunctor updateFunctor, CS& coordinatorSet)
            : mMessaging(messaging)
            , mNiceName(niceName)
            , mUpdateFunctor(updateFunctor)
            , mCoordinatorSet(coordinatorSet)
        {}

        Messaging& mMessaging;

    private:
        void Update(yaget::comp::Id_t id, const time::GameClock& gameClock, metrics::Channel& channel, const Row& row)
        {
            //const auto& message = fmt::format("Update Entity Id: {}", id);
            //metrics::Channel systemChannel(message, YAGET_METRICS_CHANNEL_FILE_LINE);

            // NOTE how can we convert row to new row with some of them being const modified?
            auto newRow = std::tuple_cat(std::tie(id, gameClock, channel), row);
            std::apply(mUpdateFunctor, newRow);
        }

        const char* mNiceName = nullptr;
        UpdateFunctor mUpdateFunctor;
        CS& mCoordinatorSet;
    };

}


////#include "Components/GameSystemInl.h"
//namespace yaget::comp_new
//{
//    template <typename E, typename ... Comps>
//    template <typename T>
//    void GameSystem<E, Comps...>::Tick(T& /*coordinatorSet*/)
//    {
//        //coordinatorSet.ForEach<RowPolicy>([](comp::Id_t id, RowPolicy row)
//        //{
//        //    id;
//        //    row;
//        //    return true;
//
//        //});
//    }
//
//}
//
//#if 0
//metrics::Channel channelUpdate(mNiceName.c_str(), YAGET_METRICS_CHANNEL_FILE_LINE);
//
//[[maybe_unused]] bool updateCalled = coordinator.ForEach<RowPolicy>([&gameClock, &channel, this](comp::Id_t id, const auto& row)
//    {
//        if (IsUpdateNeeded(id))
//        {
//            Update(id, gameClock, channel, row);
//            // timer needs to be removed if it's stopped. This happens to FireOnce timer when it actually fired
//            if (auto it = mTimers.find(id); it != std::end(mTimers) && it->second.mFireTimer == FireTimer::Stop)
//            {
//                ActivateTimer(id, gameClock, 0, FireTimer::Stop);
//            }
//
//            return true;
//        }
//
//        return false;
//    });
//
//// we only want to call end id marker if there were any id's processed
//if constexpr (EndMarker::val)
//{
//    if (updateCalled)
//    {
//        Update(comp::END_ID_MARKER, gameClock, channel, Row());
//    }
//}
//#endif
//#include "Components/GameSystemDeprecated.h"
