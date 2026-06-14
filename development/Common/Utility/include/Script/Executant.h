///////////////////////////////////////////////////////////////////////
// Executant.h
//
//  Copyright 06/7/2026 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      
//
//  #include "Script/Executant.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "YagetCore.h"


namespace yaget::script
{
    void Initialize();
    void Destroy();
    //--------------------------------------------------------------------------------------------------
    std::string Run(const std::string& sourceText);

}
