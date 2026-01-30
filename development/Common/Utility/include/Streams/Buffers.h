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
        // .first - points to data
        // .second - size of data
        using Buffer = std::pair<std::shared_ptr<uint8_t>, size_t>;
        // This represents memory with size but it does not own, thus does not delete/cleanup
        using BufferView = std::pair<const char*, size_t>;

        template<typename T>
        T* cast_data(const Buffer& buffer)
        {
            return reinterpret_cast<T*>(buffer.first.get());
        }

        template<typename T>
        size_t size_data(const T& buffer)
        {
            return buffer.second;
        }

        inline const char* BufferPointer(const Buffer& buffer)
        {
            return cast_data<const char>(buffer);
        }

        inline char* BufferPointer(Buffer& buffer)
        {
            return cast_data<char>(buffer);
        }

        inline size_t BufferSize(const Buffer& buffer)
        {
            return buffer.second;
        }

        template<typename T>
        const T* cast_data(const BufferView& buffer)
        {
            return reinterpret_cast<const T*>(buffer.first);
        }

        inline BufferView cast_to_view(const Buffer& buffer)
        {
            return { cast_data<const char>(buffer), size_data(buffer) };
        }


        inline const char* BufferPointer(const BufferView& buffer)
        {
            return cast_data<const char>(buffer);
        }

        inline size_t BufferSize(const BufferView& buffer)
        {
            return buffer.second;
        }

        inline bool operator==(const Buffer& lhs, const Buffer& rhs) 
        {
            return size_data(lhs) == size_data(rhs) && std::memcmp(cast_data<const char>(lhs), cast_data<const char>(rhs), size_data(lhs)) == 0;
        }
        inline bool operator!=(const Buffer& lhs, const Buffer& rhs) 
        {
            return !(lhs == rhs);
        }


        //! Helper to create Buffer of size
        inline Buffer CreateBuffer(size_t size)
        {
            Buffer dataBuffer{ new uint8_t[size], size };
            return dataBuffer;
        }

        inline Buffer CreateBuffer(const uint8_t* data, size_t size)
        {
            Buffer dataBuffer = CreateBuffer(size);
            std::memcpy(dataBuffer.first.get(), data, size);
            return dataBuffer;
        }

        inline Buffer ResizeBuffer(const Buffer& buffer, size_t size)
        {
            if (size <= BufferSize(buffer))
            {
                auto retValue = buffer;
                retValue.second = size;
                return retValue;
            }

            Buffer dataBuffer = CreateBuffer(size);
            std::memcpy(dataBuffer.first.get(), BufferPointer(buffer), BufferSize(buffer));
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

        //! Helper to copy content from source buffer into destination buffer at a given offset
        inline Buffer CopyBuffer(const Buffer& source, Buffer& destination, size_t offset)
        {
            YAGET_ASSERT(size_data(destination) >= size_data(source), "Destination buffer is smaller than source buffer during copy operation.");
            std::memcpy(cast_data<char>(destination) + offset, cast_data<const char>(source), size_data(source));
            return destination;
        }

        // setup basic mem buffer with write and (maybe) read pointers
        struct MessagingBuffer
        {
            MessagingBuffer(size_t bufferSize = 0)
                : mBuffer(io::CreateBuffer(bufferSize))
            {}

            void AssureWriteSize(size_t additionalSize)
            {
                const auto currentCapacity = io::BufferSize(mBuffer);
                if (mWriteOffset + additionalSize > currentCapacity)
                {
                    // standard doubling of required memory allocation
                    const size_t nextBufferSize = (currentCapacity * 2) + additionalSize;
                    mBuffer = io::ResizeBuffer(mBuffer, nextBufferSize);
                }
            }

            void WriteDataChunk(const auto* dataChunk, size_t dataSize)
            {
                YAGET_ASSERT(mWriteOffset + dataSize <= io::BufferSize(mBuffer), "Messaging buffer does not have enough space to write dataChunk out.");

                std::memcpy(io::BufferPointer(mBuffer) + mWriteOffset, dataChunk, dataSize);
                mWriteOffset += dataSize;
            }

            void WriteDataChunk(const io::Buffer& buffer)
            {
                WriteDataChunk(cast_data<const char>(buffer), size_data(buffer));
            }

            io::Buffer mBuffer;
            size_t mWriteOffset = 0;
            // Increment for every entity during frame. Allows us to allocate memory 
            // based on how many entities there were processed during frame
            size_t mNumEntities = 0;
        };

        struct Tag
        {
            std::string mName;          //! user defined name
            Guid        mGuid;          //! unique id for this asset
            std::string mVTSName;       //! Virtual Transport System data tag,  '$(Levels)/Test/Foo1.pak'
            std::string mSectionName;   //! section name that this tag belongs to it

            inline uint64_t Hash() const 
            {
                const std::hash<Guid> hasher;
                return hasher(mGuid);
            }

            inline bool operator<(const Tag& other) const
            {
                return mGuid < other.mGuid;
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

