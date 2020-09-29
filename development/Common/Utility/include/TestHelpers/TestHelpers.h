//////////////////////////////////////////////////////////////////////
// TestHelpers.h
//
//  Copyright 11/27/2019 Edgar Glowacki
//
//  Maintained by: Edgar
//
//  NOTES:
//      Utility and helper functions to simplify using tests
//
//
//  #include "TestHelpers/TestHelpers.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "YagetCore.h"


namespace yaget::test
{
    // Call this to initialize testing environment by adding default log outputs
    void InitializeEnvironment(const char* configBlockData = nullptr, std::size_t size = 0);
    void ResetEnvironment();

    // RTTI support for cleaning up test environment if different between tests.
    class Environment
    {
    public:
        Environment(const char* configBlockData = nullptr, std::size_t size = 0)
        {
            InitializeEnvironment(configBlockData, size);
        }
        
        ~Environment()
        {
            ResetEnvironment();
        }
    };


} // namespace yaget::test
