/////////////////////////////////////////////////////////////////////////
// Component.h
//
//  Copyright 7/27/2016 Edgar Glowacki.
//
// NOTES:
//      Base class for derived components, which collection of these components
//      make up an object
//
//
// #include "Components/Component.h"
//
/////////////////////////////////////////////////////////////////////////
//! \file

#pragma once

#include "YagetCore.h"
#include "Components/ComponentTypes.h"
#include "MemoryManager/PoolAllocator.h"
#include "Metrics/Performance.h"
#include <memory>
#include <map>
#include <array>
#include <stack>
#include <queue>
#include <typeindex>

namespace yaget
{
    namespace time { class GameClock; }

    namespace comp
    {
        // just provide basic template when using components. This does not add virtual table (vpt).
        class BaseComponent : public Noncopyable<BaseComponent>
        {
        public:
            static constexpr int Capacity = 64;

            ~BaseComponent() { mId = INVALID_ID; }

            Id_t Id() const { return mId; }
            time::Microsecond_t BeginLife() const { return mBeginLife; }

        protected:
            BaseComponent(Id_t id) : mId(id), mBeginLife(platform::GetRealTime(time::kMicrosecondUnit)) {}

        private:
            Id_t mId = INVALID_ID;
            time::Microsecond_t mBeginLife;
        };


        // Base class for all derive components. It exposes tick and id.
        // NOTE: Do we need anymore base class and virtual methods since we are relaying on pools?
        class Component : public BaseComponent
        {
        public:
            virtual ~Component();

            virtual void Tick(const time::GameClock& gameClock);

            using TriggerFunction = std::function<void(const Component& from)>;
            bool ConnectTrigger(int signalSig, TriggerFunction triggerFunction);

            size_t GetStateHash() const;

            enum class UpdateGuiType { None, Default };
            void UpdateGui(UpdateGuiType updateGuiType);

            // NOTE: May potentially need 
            // bool ClearTrigger(int signalSig);

        protected:
            Component(Id_t id);

            void TriggerSignal(int signalSig);
            void AddSupportedTrigger(int signalSig);

            mutable bool mStateHashDirty = true;

        private:
            virtual size_t CalculateStateHash() const = 0;
            virtual void onUpdateGui(UpdateGuiType /*updateGuiType*/) {}

            bool IsSignalPublished(int signalSig) const;
            bool IsSignalConnected(int signalSig) const;

            struct SignalHeader
            {
                inline bool operator<(const SignalHeader& rhs) const
                {
                    return mSignature < rhs.mSignature;
                }

                int mSignature;
                Component::TriggerFunction mFunction;
            };
            std::set<SignalHeader> mSignalHeaders;

            mutable uint64_t mStateHash = 0;

        };

        // DEPRECATED: 
        // Provides pool of specific component type. Derived classes will pass T as a component type and S will be
        // the pool size
        template <typename T, int S>
        class ComponentPool
        {
        public:
            typedef std::shared_ptr<T> Ptr;

            virtual ~ComponentPool()
            {
                YAGET_ASSERT(mPoolHandleMap.empty(), "There are outstanding '%d' pool components of type '%s'.", mPoolHandleMap.size(), typeid(T).name());
            }

            // Called every game logic loop by system owner
            virtual void Tick(const time::GameClock& /*gameClock*/)
            {
            }

            // Do not cache return pointer and only operate for the duration of the scope
            T* Find(Id_t id)
            {
                std::map<Id_t, size_t>::const_iterator it = mPoolHandleMap.find(id);
                if (it != mPoolHandleMap.end())
                {
                    return Get(it->second);
                }

                return nullptr;
            }

        protected:
            ComponentPool()
            {}

            template<typename... Args>
            Ptr NewComponent(Id_t id, Args&&... args)
            {
                size_t poolIndex = mNextPoolIndex;
                if (!mFreeHandles.empty())
                {
                    poolIndex = mFreeHandles.back();
                    mFreeHandles.pop_back();
                }
                else
                {
                    mNextPoolIndex++;
                    YAGET_ASSERT(mNextPoolIndex < S, "Exceeded pool size '%d' for component type: '%s'.", S, typeid(T).name());
                }

                mPoolHandleMap.insert(std::make_pair(id, poolIndex));

                void* memory = GetRaw(poolIndex);
                try
                {
                    T* component = new(memory) T(id, std::forward<Args>(args)...);
                    std::shared_ptr<T> lcPool(component, [this](T* c)
                    {
                        FreeComponent(c->Id());
                    });

                    // adding custom deleter to shared pointer
                    YLOG_INFO("COMP", "Created pool component '%d:%s'.", id, typeid(T).name());
                    return lcPool;
                }
                catch (const ex::bad_init& /*e*/)
                {
                    FreeComponent(id);
                    throw;
                }
                catch (const ex::bad_resource& /*e*/)
                {
                    FreeComponent(id);
                    throw;
                }
            }

            // Execute callback for each valid component. The iteration order is what's in the pool sequentially
            // callback can return false to stop iteration
            void ForEach(std::function<bool(T*)> callback)
            {
                bool bEmpty = mFreeHandles.empty();
                T* component = Get(0);
                size_t poolIndex = mNextPoolIndex;

                // TODO: maintain separate array of data, where we mark up which component is valid and fully initialize
                // as it stands now, this can actually walk over the component which is in ctor state, thus not finished, if accessed
                // from different threats
                for (size_t i = 0; i < poolIndex; ++i)
                {
                    if (component)
                    {
                        if (bEmpty || std::find(mFreeHandles.begin(), mFreeHandles.end(), i) == mFreeHandles.end())
                        {
                            if (!callback(component))
                            {
                                return;
                            }
                        }
                    }
                    component++;
                }
            }

            size_t NumComponents() const
            {
                return mNextPoolIndex - mFreeHandles.size();
            }

        private:
            void FreeComponent(Id_t id)
            {
                std::map<Id_t, size_t>::const_iterator it = mPoolHandleMap.find(id);
                YAGET_ASSERT(it != mPoolHandleMap.end(), "Trying to free '%d:%s' component when it's not in the pool.", id, typeid(T).name());

                T* c = Get(it->second);
                YAGET_ASSERT(c, "Trying to free component: '%d' but it's nullptr. Is this MT bug?", id);
                c->~T();
                mFreeHandles.push_back(it->second);
                mPoolHandleMap.erase(it);

                YLOG_INFO("COMP", "Destroyed pool component '%d:%s'.", id, typeid(T).name());
            }

            T* Get(size_t index)
            {
                T* c = reinterpret_cast<T*>(GetRaw(index));
                return c;
            }

            void* GetRaw(size_t index)
            {
                size_t poolOffset = index * sizeof(T);
                char* poolPtr = mPool.data();
                return poolPtr + poolOffset;
            }

            std::array<char, sizeof(T) * S> mPool = {};
            std::map<Id_t, size_t> mPoolHandleMap;
            size_t mNextPoolIndex = 0;

            using FreeHandles_t = std::vector<size_t>;
            FreeHandles_t mFreeHandles;
        };

        template <typename T>
        class hasOnTick
        {
            typedef char one;
            typedef long two;

            template <typename C>
            static  one test(decltype(&C::onTick));

            template <typename C>
            static two test(...);

        public:
            static constexpr bool value = sizeof(test<T>(0)) == sizeof(char);
        };


    } // namespace comp
} // namespace yaget
