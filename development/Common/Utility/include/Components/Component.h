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

#include <typeindex>

namespace yaget::comp
{
    // This wraps Id_t type (in most cases it will be 64 bit, but it may expend to 128 bit number)
    class ItemId
    {
    public:
        ItemId(comp::Id_t id) : mId(id) {}
        operator comp::Id_t() const { return mId; }

        std::string ToString() const
        {
            if (IsIdPersistent(mId))
            {
                return "P:" + std::to_string(StripQualifiers(mId));
            }

            return "B:" + std::to_string(mId);
        }

    private:
        comp::Id_t mId{ comp::INVALID_ID };
    };

    constexpr int DefaultPoolSize = 64;
    constexpr int GlobalPoolSize = 1;
    constexpr int SmallPoolSize = 8;

    namespace db
    {
        namespace internal
        {
            template<typename T>
            concept HasCompRow = requires (T)
            {
                typename T::Row;
                typename T::Types;
            };

            template <typename T>
            auto GetPropRow()
            {
                if constexpr (internal::HasCompRow<T>)
                {
                    struct ComponentProperties
                    {
                        using Row = typename T::Row;
                        using Types = typename T::Types;
                    };

                    return ComponentProperties{};
                }
                else
                {
                    struct ComponentProperties
                    {
                        using Row = std::tuple<>;
                        using Types = std::tuple<>;
                    };

                    return ComponentProperties{};
                }
            }

        }

        template <typename T>
        struct RowDescription
        {
            using Type = decltype(internal::GetPropRow<T>());
        };

        template <typename T>
        using RowDescription_t = typename RowDescription<T>::Type;

    }

    // just provide basic template when using components. This does not add virtual table (vpt).
    template <int Cap = DefaultPoolSize>
    class BaseComponent : public Noncopyable<BaseComponent<Cap>>
    {
    public:
        static constexpr int Capacity = Cap;

        ~BaseComponent() = default;

        ItemId Id() const { return mId; }
        time::Microsecond_t BeginLife() const { return mBeginLife; }

    protected:
        BaseComponent(Id_t id) : mId(id), mBeginLife(platform::GetRealTime(time::kMicrosecondUnit)) {}

    private:
        const ItemId mId{ INVALID_ID };
        const time::Microsecond_t mBeginLife;
    };


    // Base class for all derive components. It exposes tick and id.
    // NOTE: Do we need anymore base class and virtual methods since we are relaying on pools?
    class Component : public BaseComponent<DefaultPoolSize>
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

            int mSignature = 0;
            Component::TriggerFunction mFunction;
        };
        std::set<SignalHeader> mSignalHeaders{};

        mutable uint64_t mStateHash = 0;

    };

} // namespace yaget::comp
