/////////////////////////////////////////////////////////////////////////
// GeometryConvertor.h
//
//  Copyright 2/10/2017 Edgar Glowacki.
//
// NOTES:
//     SImplifies loading and converting geometry into render formats (platform specific???)
//
//
// #include "Loaders/GeometryConvertor.h"
//
/////////////////////////////////////////////////////////////////////////
//! \file

#pragma once
#ifndef YAGET_RENDER_GEOMETRY_CONVERTOR_H
#define YAGET_RENDER_GEOMETRY_CONVERTOR_H

//#include "Math/Vector.h"
#include "MathFacade.h"
#include "Streams/Buffers.h"
#include <memory>
#include <assimp/Importer.hpp>
#include <vector>

struct aiNode;

namespace yaget
{
    namespace render
    {
        /// It throws ex::bad_init excpetion from ctor, if geometry is not valid
        class GeometryConvertor
        {
        public:
            using Ptr = std::shared_ptr<GeometryConvertor>;
            using Verticies = std::vector<math3d::Vector3>;
            using Colors = std::vector<colors::Color>;
            using Indicies = std::vector<unsigned int>;

            struct GeometryData
            {
                // returns pair for pointer to floats and number of float elements. floats are set as (x, y, z)
                typedef std::pair<const float*, size_t> Verticies_t;
                Verticies_t mVerticies;
                Verticies_t mUVs;
                std::vector<unsigned int> mIndices;
                io::Tag mMaterialTag;
                math::Box mBoundingBox;
                math3d::Matrix mMatrix;
            };

            struct MeshCounters
            {
                long mVerticies = 0;
                long mIndices = 0;
                long mFaces = 0;
            };

            GeometryConvertor(const char* fileName, const char* textureFileName);
            GeometryConvertor(const uint8_t* dataBuffer, size_t sizeBuffer, const char* textureFileName);
            GeometryConvertor(Verticies& verticies, Verticies& uvs, Indicies& indicies);

            ~GeometryConvertor() = default;


            const math::Box& BoundingBox() const;

            size_t NumMeshes() const { return mVertextData.size(); }
            const GeometryData& GetVertexData(size_t index) const;
            GeometryData& GetMutableVertexData(size_t index);
            //const MeshCounters& MeshCounter() const { return mMeshCounters; }

        private:
            void Parse(const char* textureFileName);
            void ParseNode(const aiNode* node, math::Box& boundingBox, math3d::Matrix& transformation, std::vector<GeometryData>& vertextData, const std::string& textureFileName);

            MeshCounters mMeshCounters;

            Assimp::Importer mImporter;
            const aiScene* mLoaderScene;
            math::Box mBoundingBox;
            std::vector<GeometryData> mVertextData;

            Verticies mVerticies;
            Verticies mUvs;
            Indicies mIndicies;
        };

        class PakConverter
        {
        public:
            using Verticies = std::vector<math3d::Vector3>;
            using Uvs = std::vector<math3d::Vector3>;
            using Colors = std::vector<colors::Color>;
            using Indicies = std::vector<unsigned int>;

            struct PakData
            {
                using FloatBuffer = std::pair<const float*, size_t>;
                using IndexBuffer = std::pair<const unsigned int*, size_t>;

                FloatBuffer mVertextBuffer;
                FloatBuffer mUVBuffer;
                FloatBuffer mColorBuffer;
                IndexBuffer mIndexBuffer;

                math::Box mBoundingBox;
                math3d::Matrix mMatrix;
            };

            PakConverter(Verticies&& verticies, Uvs&& uvs, Colors&& colors, Indicies&& indicies);

            size_t NumPaks() const { return mPaks.size(); }
            const PakData& GetPakData(size_t index) const;

        private:
            Verticies mVerticies;
            Uvs mUvs;
            Colors mColors;
            Indicies mIndicies;

            std::vector<PakData> mPaks;
        };
    } // namespace render

} // namespace yaget

#endif // YAGET_RENDER_GEOMETRY_CONVERTOR_H
