///////////////////////////////////////////////////////////////////////
//  GeometryStream.h
//
//  Copyright 02/05/2018 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//     Optimized geometry format for yaget engine (using flatbuffers)
//
//
//  #include "Streams/GeometryStream.h"
//
//////////////////////////////////////////////////////////////////////
//! \file

#pragma once

#include "Base.h"
#include "RenderMathFacade.h"
#include "Streams/Buffers.h"
#include <memory>
#include <vector>
#include <fstream>

namespace yaget
{
    namespace render
    {
        class GeometryConvertor;
        class PakConverter;
    } // namespace render

    namespace io
    {
        size_t size(std::istream& stream);
        size_t size(std::ostream& stream);

        //! Memory for holding native format geometry data. mBufferHandle holds and owns the flat memory buffer.
        //! mVerticies, mUVs and mIndices point into mBufferHandle memory area
        struct GeometryData
        {
            // returns pair for pointer to floats and number of float elements. floats are set as (x, y, z)
            using Verticies_t = std::pair<const float*, size_t>;
            using Indicies_t = std::pair<const uint32_t*, size_t>;

            io::Tag mMaterialTag;
            Verticies_t mVerticies;
            Verticies_t mUVs;
            Indicies_t mIndices;
            math::Box mBoundingBox;
            io::Buffer mBufferHandle;
        };

        //! Read file data into buffer, only reason here since in release mode it can produce internal linker error
        void ReadStream(std::istream& file, io::Buffer& buffer);
        bool WriteStream(std::ofstream& outStream, const io::Buffer& dataBuffer);

        //! Used in converting from fbx format using asimp to load into yaget format and returns flat memory buffer representing geometry
        io::Buffer SerializeConvertor(const yaget::render::GeometryConvertor& convertor);
        //! Parse buffer data into geometry
        std::vector<GeometryData> DeserializeBuffer(io::Buffer buffer);

        // Convert PakConverter to pak type of data format
        io::Buffer ConvertToPak(const yaget::render::PakConverter& pakConverter);

    } // namespace io
} // namespace yaget
