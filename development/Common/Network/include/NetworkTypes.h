///////////////////////////////////////////////////////////////////////
// NetworkTypes.h
//
//  Copyright 01/29/2024 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      Helpers for networking
//
//
//  #include "NetworkTypes.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "YagetCore.h"

namespace yaget::network
{
    // Client will aquire this ticket and will send to server to get authorization
    using Ticket_t = uint32_t;
    
}
