/////////////////////////////////////////////////////////////////////////
// Script.h
//
//  Copyright 4/5/2009 Edgar Glowacki.
//
// NOTES:
//
//
// #include "Script.h"
//
/////////////////////////////////////////////////////////////////////////
//! \file

#ifndef SCRIPT_H
#define SCRIPT_H
#pragma once

#include "Base.h"

namespace eg 
{ 
    class AssetLoadSystem;

    namespace script 
    {
        //! This is used to pass AssetLoadSystem as
        //! a user data
        struct AssetLoadSystem
        {
            eg::AssetLoadSystem *als;
        };

        static const char *yaget = "yaget";


    } // namespace script 
} // namespace eg



#endif // SCRIPT_H

