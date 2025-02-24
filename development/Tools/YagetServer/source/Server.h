///////////////////////////////////////////////////////////////////////
// Server.h
//
//  Copyright 01/10/2024 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      Main entry point for server initialization and kick of running logic loop
//
//
//  #include "Server.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "YagetCore.h"

namespace yaget::server
{
    int Run(const yaget::args::Options& options);
}
