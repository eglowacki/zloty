///////////////////////////////////////////////////////////////////////
// NewAllocator.h
//
//  Copyright 12/21/2022 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      Utility functions for memory management
//
//      https://drmemory.org/page_running.html
//      
//  #include "MemoryManager/NewAllocator.h"
// 
// Allocation Hook Functions:
//  https://learn.microsoft.com/en-us/visualstudio/debugger/allocation-hook-functions?view=vs-2022
//
//////////////////////////////////////////////////////////////////////
//! \file

#pragma once

#include "YagetCore.h"

namespace yaget::memory
{
    void InitializeAllocations();

    void StartRecordAllocations();
    void StopRecordAllocations();

    void ReportAllocations();

    struct RecordAllocations
    {
        RecordAllocations()
        {
            StartRecordAllocations();
        }

        ~RecordAllocations()
        {
            StopRecordAllocations();
        }

    };

} // namespace yaget::memory
