///////////////////////////////////////////////////////////////////////
// Buffers.h
//
//  Copyright 02/23/2018 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      Provides memory buffer data structures used in stream and io operations
//
//
//  #include "Streams/Buffers.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "YagetCore.h"
#include "App/AppUtilities.h"
#include <memory>
#include <fstream>


namespace yaget
{
    namespace io
    {
        using Buffer = std::pair<std::shared_ptr<uint8_t>, size_t>;

        inline const char* BufferPointer(const Buffer& buffer)
        {
            return reinterpret_cast<const char*>(buffer.first.get());
        }

        inline size_t BufferSize(const Buffer& buffer)
        {
            return buffer.second;
        }

        //! Helper to create Buffer of size
        inline Buffer CreateBuffer(size_t size)
        {
            Buffer dataBuffer;

            dataBuffer.first.reset(new uint8_t[size]);
            dataBuffer.second = size;
            return dataBuffer;
        }

        inline Buffer CreateBuffer(const uint8_t* data, size_t size)
        {
            Buffer dataBuffer = CreateBuffer(size);
            std::memcpy(dataBuffer.first.get(), data, size);
            return dataBuffer;
        }

        inline Buffer CreateBuffer(const char* data, size_t size) { return CreateBuffer(reinterpret_cast<const uint8_t*>(data), size); }
        inline Buffer CreateBuffer(const std::string& message) { return CreateBuffer(reinterpret_cast<const uint8_t*>(message.data()), message.size()); }

        //! Helper to clone Buffer and it's content into new object
        inline Buffer CloneBuffer(const Buffer& source)
        {
            Buffer dataBuffer = CreateBuffer(source.first.get(), source.second);
            return dataBuffer;
        }

        struct Tag
        {
            std::string mName;          //! user defined name
            Guid        mGuid;          //! unique id for this asset
            std::string mVTSName;       //! Virtual Transport System data tag,  '$(Levels)/Test/Foo1.pak'
            std::string mSectionName;   //! section name that this tag belongs to it

            inline uint64_t Hash() const 
            {
                std::hash<Guid> hasher;
                return hasher(mGuid);
            }

            inline bool operator<(const Tag& other) const
            {
                return mGuid.bytes() < other.mGuid.bytes();
            }

            inline std::string ResolveVTS() const
            {
                return util::ExpendEnv(mVTSName, nullptr);
            }

            inline bool operator==(const Tag& tag) const
            {
                return mName == tag.mName && mGuid == tag.mGuid && mVTSName == tag.mVTSName && mSectionName == tag.mSectionName;
            }

            inline bool IsValid() const { return mGuid.IsValid(); }

            Tag operator()() const { return*this; }
        };

        using Tags = std::vector<io::Tag>;

    } // namespace io
} // namespace yaget

