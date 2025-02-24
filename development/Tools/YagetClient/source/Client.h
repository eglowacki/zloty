///////////////////////////////////////////////////////////////////////
// Client.h
//
//  Copyright 01/14/2024 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      Main entry point for client initialization and kick of running logic loop
//
//
//  #include "Client.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "YagetCore.h"

namespace yaget::client
{
    int Run(const yaget::args::Options& options);
}
