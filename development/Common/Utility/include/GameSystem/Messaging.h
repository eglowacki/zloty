//////////////////////////////////////////////////////////////////////
// Messaging.h
//
//  Copyright 11/23/2020 Edgar Glowacki
//
//  Maintained by: Edgar
//
//  NOTES:
//
//
//  #include "GameSystem/Messaging.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "Components/ComponentTypes.h"


namespace yaget::comp::gs
{
    template <typename P>
    class Messaging
    {
    public:
        void AddPayload(P payload)
        {
            mPayload = std::move(payload);
        }

        bool IsPayload() const
        {
            return false;
        }

        P GetPayload()
        {
            P retValue = mPayload;
            mPayload = {};
            return retValue;
        }

    private:
        P mPayload{};
    };

}
