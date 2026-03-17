/////////////////////////////////////////////////////////////////////////
// Zipper.h
//
//  Copyright 03/17/2026 Edgar Glowacki.
//
// NOTES:
//      
//
// #include "Compression/Zipper.h"
//
/////////////////////////////////////////////////////////////////////////
//! \file

#pragma once

#include "Streams/Buffers.h"

namespace yaget::compression
{
    io::Buffer ZipBuffer(const io::BufferView sourceBuffer);
    io::Buffer UnzipBuffer(const io::BufferView sourceBuffer);
    
}
