///////////////////////////////////////////////////////////////////////
// Editor.h
//
//  Copyright 06/06/2021 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      Main entry point for editor initialization and kick of running logic loop
//
//
//  #include "Editor.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "YagetCore.h"

namespace yaget::editor
{
    int Run(const yaget::args::Options& options);
}
