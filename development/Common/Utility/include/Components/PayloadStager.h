//////////////////////////////////////////////////////////////////////
// PayloadStager.h
//
//  Copyright 7/2/2019 Edgar Glowacki
//
//  Maintained by: Edgar
//
//  NOTES:
//      Class that handles atomic smart pointer exchange
//      between threads
//
//
//  #include "Components/PayloadStager.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "YagetCore.h"
#include "GameSystem/tinyevents/tinyevents.h"
#include <memory>
#include <stack>

namespace yaget::comp
{
    // Provides ability to move payloads between threads, primary usage is Logic Thread -> Render Thread
    // It also provides ability to listen and dispatch events between threads
    template<typename S>
    class PayloadStager
    {
    public:
        using Storage = S;
        using Payload = std::shared_ptr<Storage>;
        using ConstPayload = std::shared_ptr<const Storage>;
    
        // Create new and blank payload ready to be filled and called to SetPayload
        template<typename... Args>
        Payload CreatePayload(Args&&... args);
        // Set this payload as the active one and return the previous one.
        // This call makes payload available to GetPayload and ConsumePayload methods.
        Payload SetPayload(const Payload& payload);
    
        // Returns active payload and clears current one to empty
        ConstPayload ConsumePayload();

        enum class DispatcherType
        {
            Render,
            Logic
        };

        template <typename T>
        std::uint64_t Listen(const std::function<void(const T&)>& listener, DispatcherType dispatcherType)
        {
            std::lock_guard lock(mDispatcherMutex);
            return Dispatcher(dispatcherType).Listen(listener);
        }

        template <typename T>
        void Queue(T&& msg, DispatcherType dispatcherType)
        {
            std::lock_guard lock(mDispatcherMutex);
            Dispatcher(dispatcherType).Queue(std::forward<T>(msg));
        }

        void Process(DispatcherType dispatcherType)
        {
            std::lock_guard lock(mDispatcherMutex);
            Dispatcher(dispatcherType).Process();
        }

        template <typename T>
        void Dispatch(const T& msg, DispatcherType dispatcherType)
        {
            std::lock_guard lock(mDispatcherMutex);
            return Dispatcher(dispatcherType).Dispatch(msg);
        }

        void Remove(const std::uint64_t handle, DispatcherType dispatcherType)
        {
            std::lock_guard lock(mDispatcherMutex);
            Dispatcher(dispatcherType).Remove(handle);
        }

    private:
        tinyevents::Dispatcher& Dispatcher(DispatcherType dispatcherType)
        {
            return mDispatcher[static_cast<int>(dispatcherType)];
        }
        // return blank payload, ready to be filled 
        // and used in SetPayload(...) method
        template<typename... Args>
        Payload GetNextFreePayload(Args&&... args);
    
        // current payload returned by GetPayload and ConsumePayload
        // and set by SetPayload
        std::atomic<Payload> mActivePayload;
        
        std::mutex mDispatcherMutex;
        tinyevents::Dispatcher mDispatcher[2];
    };
    
    
    //-------------------------------------------------------------------------------------------
    // put this into .inl file
    template<typename S>
    template<typename... Args>
    PayloadStager<S>::Payload PayloadStager<S>::CreatePayload(Args&&... args)
    {
        Payload payload = GetNextFreePayload(std::forward<Args>(args)...);
        return payload;
    }
    
    template<typename S>
    PayloadStager<S>::Payload PayloadStager<S>::SetPayload(const Payload& newPayload)
    {
        Payload oldPayload = mActivePayload.exchange(newPayload);
        return oldPayload;
    }
    
    template<typename S>
    PayloadStager<S>::ConstPayload PayloadStager<S>:: ConsumePayload()
    {
        ConstPayload currentPayload = mActivePayload.exchange(Payload{});
        return currentPayload;
    }
    
    template<typename S>
    template<typename... Args>
    PayloadStager<S>::Payload PayloadStager<S>::GetNextFreePayload(Args&&... args)
    {
        Storage* newPayload = Storage::Allocate(std::forward<Args>(args)...);
        auto payload = std::shared_ptr<Storage>(newPayload, [](auto* oldObject)
        {
            Storage::Free(oldObject);
        });

        return payload;
    }

} // namespace yaget
