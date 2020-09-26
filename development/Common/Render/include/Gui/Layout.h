//////////////////////////////////////////////////////////////////////
// Layout.h
//
//  Copyright 7/24/2018 Edgar Glowacki
//
//  Maintained by: Edgar
//
//  NOTES:
//      Gui layout helper functions
//
//
//  #include "Gui/Layout.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "YagetCore.h"


namespace yaget
{
    namespace gui
    {
        enum class Border { Top, Right, Bottom, Left };

        void SnapTo(Border border, const std::string& name);

    } // namespace gui
} // namespace yaget
