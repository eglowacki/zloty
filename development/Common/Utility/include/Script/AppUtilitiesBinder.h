///////////////////////////////////////////////////////////////////////
// AppUtilitiesBinder.h
//
//  Copyright 11/1/2026 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      Provides binding of application utilities
//      
//
//  #include "Script/AppUtilitiesBinder.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "YagetCore.h"


namespace yaget
{
    class Application;
}

namespace sol
{
    class state;
}


namespace yaget::script
{
    //----------------------------------------------------------------------------------------------------------------------------------
    void BindAppUtilities(sol::state& luaState, const std::string& luaNamespace, Application& app);

}
