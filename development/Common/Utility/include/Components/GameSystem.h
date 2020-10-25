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
#include <functional>


namespace yaget::comp::gs
{
    // TODO add timers support for entities
    template <typename E, typename... Comps>
    class GameSystem : public Noncopyable<GameSystem<E, Comps...>>
    {
    public:
        using EndMarker = E;
        using RowPolicy = comp::RowPolicy<Comps...>;
        using Row = typename RowPolicy::Row;

        using UpdateFunctor = std::function<void(yaget::comp::Id_t id, Comps&... args)>;

        // framework calls this on same cadence (every tick...)
        // In default case that is GameSystemsCoordinator class
        template <typename CS>
        void Tick(CS& coordinatorSet)
        {
            coordinatorSet.template ForEach<Row>([this](yaget::comp::Id_t id, const auto& row)
            {
                Update(id, row);
                return true;
            });
        }

    protected:
        GameSystem(const char* niceName, UpdateFunctor updateFunctor)
            : mNiceName(niceName)
            , mUpdateFunctor(updateFunctor)
        {}


    private:
        void Update(yaget::comp::Id_t id, const Row& row)
        {
            auto newRow = std::tuple_cat(std::tie(id), row);
            std::apply(mUpdateFunctor, newRow);
        };

        const char* mNiceName = nullptr;
        UpdateFunctor mUpdateFunctor;
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
#include "Components/GameSystemDeprecated.h"
