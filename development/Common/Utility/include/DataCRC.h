///////////////////////////////////////////////////////////////////////
// DataCRC.h
//
//  Copyright 10/13/2026 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      Provides conversion of any data into crc
//
//
//  #include "DataCRC.h"
//
//////////////////////////////////////////////////////////////////////
//! \file

#pragma once

#include "YagetCore.h"

#include "Streams/Buffers.h"

namespace yaget::crc
{

    namespace internal
    {
        //-------------------------------------------------------------------------------------------------
        using CrcTable = std::array<uint32_t, 256>;

        //-------------------------------------------------------------------------------------------------
        inline CrcTable generate_table()
        {
            CrcTable table;

            constexpr uint32_t polynomial = 0xEDB88320;

            for (uint32_t i = 0; i < 256; i++)
            {
                uint32_t c = i;
                for (size_t j = 0; j < 8; j++)
                {
                    if (c & 1)
                    {
                        c = polynomial ^ (c >> 1);
                    }
                    else
                    {
                        c >>= 1;
                    }
                }

                table[i] = c;
            }

            return table;
        }


        //-------------------------------------------------------------------------------------------------
        inline uint32_t update(const CrcTable& table, uint32_t initial, const void* buf, size_t len)
        {
            uint32_t c = initial ^ 0xFFFFFFFF;
            auto u = static_cast<const uint8_t*>(buf);
            for (size_t i = 0; i < len; ++i)
            {
                c = table[(c ^ u[i]) & 0xFF] ^ (c >> 8);
            }

            return c ^ 0xFFFFFFFF;
        }

    }


    //-------------------------------------------------------------------------------------------------
    inline uint32_t CalculateCRC32(const io::BufferView& buffer)
    {
        using namespace yaget;
        static auto crcTable = internal::generate_table();

        uint32_t crc = internal::update(crcTable, 0, io::cast_data<uint8_t>(buffer), io::size_data(buffer));
        return crc;
    }


    //-------------------------------------------------------------------------------------------------
    inline uint32_t CalculateCRC32(const io::Buffer& buffer)
    {
        return CalculateCRC32(io::cast_to_view(buffer));
    }


    //-------------------------------------------------------------------------------------------------
    inline uint32_t CalculateCRC32(uint8_t* data, size_t size)
    {
        io::BufferView buffer{ data, size };
        return CalculateCRC32(buffer);
    }

}
