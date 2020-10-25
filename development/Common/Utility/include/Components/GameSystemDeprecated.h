//////////////////////////////////////////////////////////////////////
// GameSystemDeprecated.h
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
//  #include "Components/GameSystemDeprecated.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "YagetCore.h"
#include "Debugging/Assert.h"
#include "Components/GameCoordinator.h"
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
            constexpr static bool IsComponent()
            {
                return meta::tuple_is_element_v<T, Row>;
            }

            template <typename T>
            void Initialize(const time::GameClock& /*gameClock*/, metrics::Channel& /*channel*/, comp::Coordinator<T>& /*coordinator*/)
            {
            }

            template <typename T>
            void Shutdown(const time::GameClock& /*gameClock*/, metrics::Channel& /*channel*/, comp::Coordinator<T>& /*coordinator*/)
            {
                mUpdatedItemIds.clear();
                mTimers.clear();
            }

            template <typename T>
            void Update(const time::GameClock& gameClock, metrics::Channel& channel, comp::Coordinator<T>& coordinator)
            {
                metrics::Channel channelUpdate(mNiceName.c_str(), YAGET_METRICS_CHANNEL_FILE_LINE);

                [[maybe_unused]] bool updateCalled = coordinator.ForEach<RowPolicy>([&gameClock, &channel, this](comp::Id_t id, const auto& row)
                    {
                        if (IsUpdateNeeded(id))
                        {
                            Update(id, gameClock, channel, row);
                            // timer needs to be removed if it's stopped. This happens to FireOnce timer when it actually fired
                            if (auto it = mTimers.find(id); it != std::end(mTimers) && it->second.mFireTimer == FireTimer::Stop)
                            {
                                ActivateTimer(id, gameClock, 0, FireTimer::Stop);
                            }

                            return true;
                        }

                        return false;
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

            enum class FireTimer { Once, Repeat, Stop };
            void ActivateTimer(comp::Id_t id, const time::GameClock& gameClock, time::Microsecond_t triggerDuration, FireTimer fireTimer)
            {
                if (triggerDuration == 0)
                {
                    mTimers.erase(id);
                }
                else
                {
                    const auto nowTime = gameClock.GetRealTime();
                    mTimers[id] = TimeTriger{ nowTime + triggerDuration, triggerDuration, fireTimer };
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

            bool IsUpdateNeeded(comp::Id_t id)
            {
                if (auto it = mTimers.find(id); it != std::end(mTimers))
                {
                    auto nowTime = platform::GetRealTime(time::kMicrosecondUnit);

                    return it->second.Update(nowTime);
                }

                return true;
            }

            UpdateFunctor mUpdateFunctor;
            InitializeFunctor mInitializeFunctor;
            std::string mNiceName;
            comp::ItemIds mUpdatedItemIds;

            // Keeps track of which components (by id) to Update and how often
            struct TimeTriger
            {
                time::Microsecond_t mNextTriggerTime;
                time::Microsecond_t mDuration;
                FireTimer mFireTimer;

                // return true if trigger updated
                // false if trigger will not fire
                bool Update(time::Microsecond_t nowTime)
                {
                    if (mFireTimer == FireTimer::Repeat)
                    {
                        if (mNextTriggerTime < nowTime)
                        {
                            // our trigger is in the past and it needs to be executed
                            // most drift you will get is size of frame tick
                            mNextTriggerTime += mDuration;
                            return true;
                        }

                        // trigger is still in the future, do nothing
                    }
                    else if (mFireTimer == FireTimer::Once)
                    {
                        if (mNextTriggerTime < nowTime)
                        {
                            // our trigger is in the past and it needs to be executed
                            // most drift you will get is size of frame tick
                            mFireTimer = FireTimer::Stop;
                            return true;
                        }

                        // trigger is still in the future, do nothing
                    }

                    return false;
                }

            };

            using Timers = std::unordered_map<comp::Id_t, TimeTriger>;
            Timers mTimers;
        };

    } // namespace comp
} // namespace yaget

