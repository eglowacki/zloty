///////////////////////////////////////////////////////////////////////
// PipelineTags.h
//
//  Copyright 04/12/2026 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//
//  #include "Render/Pipeline/PipelineTags.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "Render/RenderCore.h"
#include "Streams/Buffers.h"

#include <shared_mutex>


namespace yaget::io
{
    class VirtualTransportSystem;
}

namespace yaget::render
{
    class PipelineTags
    {
    public:
        PipelineTags(io::VirtualTransportSystem& vts);
        ~PipelineTags();

        io::Tag ResolveTag(uint64_t hashValue, const std::string& tagName);
        io::Tag GetTag(uint64_t hashValue) const;

    private:
        io::VirtualTransportSystem& mVTS;

        using ResolveTags = std::map<uint64_t, io::Tag>;
        ResolveTags mResolveTags;
        mutable std::shared_mutex mMutex;
    };

}
