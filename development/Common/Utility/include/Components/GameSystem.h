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

#include "YagetCore.h"
#include "Debugging/Assert.h"
#include "Coordinator.h"
#include "Metrics/Concurrency.h"
#include <functional>


namespace yaget
{
    namespace time { class GameClock; }
    namespace metrics { class Channel; }

    namespace comp
    {

        // Called by GameCoordinator for each registered system, the Update method will in turn iterate over all items
        // and will call UpdateFunctor which will have unpacked parameters
        // void UpdateGameItem(const time::GameClock& gameClock, comp::LocationComponent* location, comp::PhysicsComponent* physics);
        // GameSystem<LocationComponent*, PhysicsComponent*> myGameSystem(UpdateGameItem);
        template <typename E, typename... Comps>
        class GameSystem : public Noncopyable<GameSystem<E, Comps...>>
        {
        public:
            using EndMarker = E;
            using RowPolicy = comp::RowPolicy<Comps...>;
            using Row = typename RowPolicy::Row;

            static constexpr int COORDINATOR_ID = EndMarker::id;

            using UpdateFunctor = std::function<void(comp::Id_t id, const time::GameClock& gameClock, metrics::Channel& channel, Comps&... args)>;
            using InitializeFunctor = UpdateFunctor;

            using Functors = std::pair<UpdateFunctor, InitializeFunctor>;

            GameSystem(const char* niceName, UpdateFunctor updateFunctor)
                : GameSystem(niceName, Functors(updateFunctor, nullptr))
            {}

            GameSystem(const char* niceName, Functors functor)
                : mUpdateFunctor(functor.first)
                , mInitializeFunctor(functor.second)
                , mNiceName(conv::safe(niceName))
            {
                YAGET_ASSERT(mUpdateFunctor, "Callback is nullptr, not allowed in GameSystem: '%s'.", mNiceName.c_str());
            }

            template <typename T>
            void Initialize(const time::GameClock& /*gameClock*/, metrics::Channel& /*channel*/, comp::Coordinator<T>& /*coordinator*/)
            {
                mUpdatedItemIds.clear();
            }

            template <typename T>
            void Update(const time::GameClock& gameClock, metrics::Channel& channel, comp::Coordinator<T>& coordinator)
            {
                metrics::Channel channelUpdate(mNiceName.c_str(), YAGET_METRICS_CHANNEL_FILE_LINE);

                [[maybe_unused]] bool updateCalled = coordinator.ForEach<RowPolicy>([&gameClock, &channel, this](comp::Id_t id, const auto& row)
                {
                    Update(id, gameClock, channel, row);
                    return true;
                });

                // we only want to call end id marker if there were any id's processed
                if constexpr (EndMarker::val)
                {
                    if (updateCalled)
                    {
                        Update(comp::END_ID_MARKER, gameClock, channel, Row());
                    }
                }
            }

        private:
            void Update(comp::Id_t id, const time::GameClock& gameClock, metrics::Channel& channel, const Row& row)
            {
                auto newRow = std::tuple_cat(std::tie(id), std::tie(gameClock), std::tie(channel), row);

                // we want to keep track of who needs to be initialized
                if (mUpdatedItemIds.insert(id).second && mInitializeFunctor)
                {
                    std::apply(mInitializeFunctor, newRow);
                }

                std::apply(mUpdateFunctor, newRow);
            }

            UpdateFunctor mUpdateFunctor;
            InitializeFunctor mInitializeFunctor;
            std::string mNiceName;
            comp::ItemIds mUpdatedItemIds;
        };

    } // namespace comp
} // namespace yaget
