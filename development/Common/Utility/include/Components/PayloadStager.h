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
#include <memory>


namespace yaget::comp
{
    // Provides ability to move payloads between threads, primary usage is Logic Thread -> Render Thread
    template<typename T>
    class PayloadStager
    {
    public:
        using Storage = T;
        using Payload = std::shared_ptr<Storage>;
        using ConstPayload = std::shared_ptr<const Storage>;
    
        // Create new and blank payload ready to be filled and called to SetPayload
        template<typename... Args>
        Payload CreatePayload(Args&&... args);
        // Set this payload as the active one and return old previous one
        Payload SetPayload(const Payload& payload);
    
        // Returns active payload without changing/clearing
        ConstPayload GetPayload() const;
        // Returns active payload and clears current one to empty
        ConstPayload ConsumePayload();
    
    private:
        // return blank payload, ready to be filled 
        // and used in SetPayload(...) method
        template<typename... Args>
        Payload GetNextFreePayload(Args&&... args);
    
        // current payload returned by GetPayload and ConsumePayload
        // and set by SetPayload
        std::atomic<Payload> mActivePayload;
    };
    
    
    //-------------------------------------------------------------------------------------------
    // put this into .inl file
    template<typename T>
    template<typename... Args>
    typename PayloadStager<T>::Payload PayloadStager<T>::CreatePayload(Args&&... args)
    {
        Payload payload = GetNextFreePayload(std::forward<Args>(args)...);
        return payload;
    }
    
    template<typename T>
    typename PayloadStager<T>::Payload PayloadStager<T>::SetPayload(const Payload& newPayload)
    {
        Payload oldPayload = mActivePayload.exchange(newPayload);
        return oldPayload;
    }
    
    template<typename T>
    typename PayloadStager<T>::ConstPayload PayloadStager<T>::GetPayload() const
    {
        ConstPayload currentPayload = mActivePayload.load();
        return currentPayload;
    }
    
    template<typename T>
    typename PayloadStager<T>::ConstPayload PayloadStager<T>::ConsumePayload()
    {
        ConstPayload currentPayload = mActivePayload.exchange(Payload{});
        return currentPayload;
    }
    
    template<typename T>
    template<typename... Args>
    typename PayloadStager<T>::Payload PayloadStager<T>::GetNextFreePayload(Args&&... args)
    {
        return std::make_shared<Storage>(std::forward<Args>(args)...);
    }

} // namespace yaget
