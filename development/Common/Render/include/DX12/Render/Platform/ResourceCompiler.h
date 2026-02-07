// ResourceCompiler.h
//
//  Copyright 08/29/2022 Edgar Glowacki.
//
// NOTES:
//     Exposes compiler facility
//
// #include "Render/Platform/ResourceCompiler.h"
//
/////////////////////////////////////////////////////////////////////////
//! \file

#pragma once

#include "Render/RenderCore.h"
#include "Streams/Buffers.h"

struct ID3D12LibraryReflection;
struct ID3D10Blob;

namespace yaget::render
{
    //-------------------------------------------------------------------------------------------------
    class ResourceCompiler
    {
    public:
        ResourceCompiler(io::BufferView data, const char* entryName, const char* target, bool useOldCompiler, bool debugShaders);
        io::Buffer GetCompiled() const;

    private:
        io::Buffer mBinaryBlob;
    };


    //-------------------------------------------------------------------------------------------------
    class ResourceReflector
    {
    public:
        ResourceReflector(io::BufferView data);

    private:
        ComPtr<ID3D12LibraryReflection> mReflector;
    };
}
