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

#include "App/Application.h"

#include "VTS/VirtualTransportSystem.h"


namespace sol
{
    class state;
}

namespace yaget::script
{
    //--------------------------------------------------------------------------------------------------
    //std::string Run(const std::string& sourceText);

    //--------------------------------------------------------------------------------------------------
    enum class Execute
    {
        AsIs,
        Safe
    };

    //--------------------------------------------------------------------------------------------------
    bool Run(sol::state& luaState, const std::string& source, const io::Tag& tag = {}, Execute execute = Execute::AsIs);

}
