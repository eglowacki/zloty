//////////////////////////////////////////////////////////////////////
// tinyevents.h
//
//  Copyright 04/28/2026 Edgar Glowacki
//
//  Maintained by: Edgar
//
//  NOTES:
//      from https://github.com/KyrietS/tinyevents
//
//
//  #include "GameSystem/tinyevents/tinyevents.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include <algorithm>
#include <functional>
#include <queue>
#include <set>
#include <map>
#include <ranges>
#include <typeindex>
#include <utility>

namespace tinyevents
{
    class Token;

    //-------------------------------------------------------------------------------------------------
    class Dispatcher
    {
        using ListenerHandle = std::uint64_t;
        using Listeners = std::map<ListenerHandle, std::function<void(const void*)>>;

    public:
        Dispatcher() = default;
        Dispatcher(Dispatcher&&) noexcept = default;
        Dispatcher(const Dispatcher&) = delete;
        Dispatcher& operator=(Dispatcher&&) noexcept = default;

        template <typename T>
        std::uint64_t Listen(const std::function<void(const T&)>& listener)
        {
            auto& listeners = mListenersByType[std::type_index(typeid(T))];
            const auto listenerHandle = ListenerHandle{ mNextListenerId++ };

            listeners[listenerHandle] = [listener](const auto& msg)
            {
                const T* concreteMessage = static_cast<const T*>(msg);
                listener(*concreteMessage);
            };
            return listenerHandle;
        }

        template <typename T>
        std::uint64_t ListenOnce(const std::function<void(const T&)>& listener)
        {
            const auto listenerId = mNextListenerId;
            return Listen<T>([this, listenerId, listener](const T& msg)
            {
                ListenerHandle handle{ listenerId };
                mListenersScheduledForRemoval.emplace(handle); // Fix for nested ListenOnce
                listener(msg);
                mListenersScheduledForRemoval.erase(handle);
                this->Remove(handle);
            });
        }

        template <typename T>
        void Dispatch(const T& msg)
        {
            const auto& listenersIter = mListenersByType.find(std::type_index(typeid(T)));
            if (listenersIter == mListenersByType.end())
            {
                return; // No listeners for this type of message
            }

            const auto& [msgType, listeners] = *listenersIter;

            // Cache handles to avoid iterator invalidation. This way listeners can safely Remove themselves.
            std::vector<ListenerHandle> handles;
            handles.reserve(listeners.size());
            for (const auto& handle : listeners | std::views::keys)
            {
                handles.push_back(handle);
            }

            for (auto& handle : handles)
            {
                const auto& handleAndListener = listeners.find(handle);
                const bool isListenerPresent = handleAndListener != listeners.end();
                if (isListenerPresent && !IsScheduledForRemoval(handle))
                {
                    const auto& listener = handleAndListener->second;
                    listener(&msg);
                }
            }
        }

        template <typename T>
        void Queue(T&& msg)
        {
            mQueuedDispatches.push([m = std::forward<T>(msg)](Dispatcher& dispatcher)
            {
                dispatcher.Dispatch(m);
            });
        }

        void Process()
        {
            while (!mQueuedDispatches.empty())
            {
                std::function<void(Dispatcher&)> queuedDispatch = std::move(mQueuedDispatches.front());
                mQueuedDispatches.pop();
                queuedDispatch(*this);
            }
        }

        void Remove(const std::uint64_t handle)
        {
            if (IsScheduledForRemoval(handle))
            {
                return;
            }

            for (auto& val : mListenersByType | std::views::values)
            {
                val.erase(handle);
            }
        }

        [[nodiscard]] bool HasListener(std::uint64_t handle) const
        {
            if (IsScheduledForRemoval(handle))
            {
                return false;
            }

            return std::ranges::any_of(mListenersByType, [&handle](const auto& listeners)
            {
                return listeners.second.find(handle) != listeners.second.end();
            });
        }

    private:
        [[nodiscard]] bool IsScheduledForRemoval(const std::uint64_t handle) const
        {
            return mListenersScheduledForRemoval.contains(handle);
        }

        std::map<std::type_index, Listeners> mListenersByType;
        std::queue<std::function<void(Dispatcher&)>> mQueuedDispatches;
        std::set<ListenerHandle> mListenersScheduledForRemoval;

        std::uint64_t mNextListenerId = 1;
    };


    //-------------------------------------------------------------------------------------------------
    // RAII wrapper for listener Handle.
    class Token
    {
    public:
        Token(Dispatcher& dispatcher, const std::uint64_t handle)
            : mDispatcher(dispatcher), mHandle(handle), mHoldsResource(true)
        {
        }

        ~Token()
        {
            if (mHoldsResource)
            {
                mDispatcher.get().Remove(mHandle);
            }
        }

        // Disable copy operations
        Token(const Token&) = delete;
        Token& operator=(const Token&) = delete;

        // Enable move operations
        Token(Token&& other) noexcept
            : mDispatcher(other.mDispatcher), mHandle(other.mHandle), mHoldsResource(other.mHoldsResource)
        {
            other.mHoldsResource = false;
        }

        Token& operator=(Token&& other) noexcept
        {
            if (this != &other)
            {
                if (this->mHoldsResource)
                {
                    mDispatcher.get().Remove(mHandle);
                }
                mDispatcher = other.mDispatcher;
                mHandle = other.mHandle;
                mHoldsResource = other.mHoldsResource;
                other.mHoldsResource = false;
            }
            return *this;
        }

        [[nodiscard]] std::uint64_t Handle() const
        {
            return mHandle;
        }

        void Remove()
        {
            mDispatcher.get().Remove(mHandle);
            mHoldsResource = false;
        }

    private:
        std::reference_wrapper<Dispatcher> mDispatcher;
        std::uint64_t mHandle;
        bool mHoldsResource;
    };

}
