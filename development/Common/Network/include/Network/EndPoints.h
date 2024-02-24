///////////////////////////////////////////////////////////////////////
// EndPoints.h
//
//  Copyright 02/05/2024 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      Utilities related to end points
//
//
//  #include "Network/EndPoints.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "YagetCore.h"

#include <boost/asio/ip/tcp.hpp>

namespace yaget::network
{
    boost::asio::ip::tcp::endpoint CreateEndPoint(const std::string& networkAddress, boost::system::error_code& ec);
}
