//////////////////////////////////////////////////////////////////////
// DiagnosticVirtualTransportSystem.h
//
//  Copyright 5/06/2018 Edgar Glowacki
//
//  Maintained by: Edgar
//
//  NOTES:
//      Can diagnose and fix VTS db
//
//
//  #include "VTS/DiagnosticVirtualTransportSystem.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "VTS/VirtualTransportSystem.h"

namespace yaget::io::diag
{
    class VirtualTransportSystem : public io::VirtualTransportSystem
    {
    public:
        VirtualTransportSystem(bool diagnosticMode, const char* fileName);

    private:
        bool VerifyDirtyTags() const;
    };

}
