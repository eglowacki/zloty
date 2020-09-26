#include "Logger/YLog.h"
#include "Loaders/GeometryConvertor.h"
#include "App/AppUtilities.h"
#include "Streams/GeometryStream.h"
#include "Debugging/Assert.h"
#include "Fmt/format.h"
#include "StringHelpers.h"
#include "Exception/Exception.h"
#include <assimp/scene.h>           // Output data structure
#include <assimp/postprocess.h>     // Post processing flags
#include <assimp/cimport.h>
#include <filesystem>


using namespace yaget;
namespace fs = std::filesystem;

namespace
{
    const math3d::Matrix kSwapYZMatrix(math3d::Vector3(1, 0, 0), math3d::Vector3(0, 0, 1), math3d::Vector3(0, -1, 0));

} // namespace

namespace conv
{

    math3d::Matrix ToMatrix(const aiMatrix4x4& assimpMatrix)
    {
        aiMatrix4x4 source(assimpMatrix);
        source.Transpose();
        math3d::Matrix matrix(source[0]);
        return matrix;
    }

} // namespace conv



render::GeometryConvertor::GeometryConvertor(Verticies& verticies, Verticies& uvs, Indicies& indicies)
    : mVerticies(std::move(verticies))
    , mUvs(std::move(uvs))
    , mIndicies(std::move(indicies))
{

    GeometryData geometryData;
    geometryData.mVerticies.first = reinterpret_cast<const float *>(&mVerticies[0]);
    geometryData.mVerticies.second = mVerticies.size() * 3;

    geometryData.mUVs.first = reinterpret_cast<const float *>(&mUvs[0]);
    geometryData.mUVs.second = mUvs.size() * 3;

    geometryData.mIndices = mIndicies;

    for (const auto& it : mVerticies)
    {
        geometryData.mBoundingBox.GrowExtends(it);
    }

    geometryData.mMatrix = math3d::Matrix::Identity;

    mVertextData.emplace_back(geometryData);
}

render::GeometryConvertor::GeometryConvertor(const char* fileName, const char* textureFileName)
    : mLoaderScene(mImporter.ReadFile(fileName, aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_ConvertToLeftHanded))
{
    Parse(textureFileName);
}

render::GeometryConvertor::GeometryConvertor(const uint8_t* dataBuffer, size_t sizeBuffer, const char* textureFileName)
    : mLoaderScene(mImporter.ReadFileFromMemory(dataBuffer, sizeBuffer, aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_ConvertToLeftHanded))
{
    Parse(textureFileName);
}

void render::GeometryConvertor::Parse(const char* textureFileName)
{
    if (mLoaderScene && mLoaderScene->mRootNode && mLoaderScene->mNumMeshes)
    {
        mVertextData.resize(mLoaderScene->mNumMeshes);

        // if we do not have texture file, then just use placeholder, TODO: should add checking if file exists on the disk
        std::string textureAssetPath;
        if (!textureFileName || *textureFileName == '\0')
        {
            if (mLoaderScene->HasMaterials())
            {
                for (unsigned int i = 0; i < mLoaderScene->mNumMaterials; ++i)
                {
                    aiMaterial* material = mLoaderScene->mMaterials[i];

                    aiString textureName;
                    if (material->Get(AI_MATKEY_TEXTURE_DIFFUSE(0), textureName) == aiReturn_SUCCESS)
                    {
                        textureAssetPath = textureName.C_Str();
                    }
                    else if (material->Get(AI_MATKEY_NAME, textureName) == aiReturn_SUCCESS)
                    {
                        textureAssetPath = textureName.C_Str();
                    }
                }

                //aiMaterial* material = mLoaderScene->mMaterials[0];
                //aiString textureName;
                //if (material->Get(AI_MATKEY_TEXTURE_DIFFUSE(0), textureName) == aiReturn_SUCCESS)
                //{
                //    textureAssetPath = textureName.C_Str();
                //}
            }

            //textureAssetPath = textureAssetPath.empty() ? util::ExpendEnv("$(AssetFolder)/Textures/Placeholder.png", nullptr) : textureAssetPath;
        }
        else
        {
            textureAssetPath = textureFileName;
        }

        math3d::Matrix transformation;
        ParseNode(mLoaderScene->mRootNode, mBoundingBox, transformation, mVertextData, textureAssetPath);

        math3d::Vector3 bSize = mBoundingBox.GetSize();
        aiMemoryInfo memoryInfo;
        mImporter.GetMemoryRequirements(memoryInfo);
        std::string memSizeString = conv::ToThousandsSep(memoryInfo.total);

        YLOG_DEBUG("GEOM", "Loaded Geometry from asset data stream...");
        YLOG_DEBUG("GEOM", "Mesh Counters: Faces: %d, Verticies: %d, Mem: '%s' bytes.", mMeshCounters.mFaces, mMeshCounters.mVerticies, memSizeString.c_str());
        YLOG_DEBUG("GEOM", "Min Corner: x: %.2f + %.2f, y: %.2f + %.2f, z: %.2f + %.2f, Center: %s.",
            mBoundingBox.mMin.x, bSize.x,
            mBoundingBox.mMin.y, bSize.y,
            mBoundingBox.mMin.z, bSize.z,
            conv::Convertor<math3d::Vector3>::ToString(mBoundingBox.GetMid()).c_str());
    }
    else
    {
        std::string textError = mLoaderScene ? fmt::format("No geometry associated with asset data stream") : mImporter.GetErrorString();
        YAGET_UTIL_THROW("GEOM", textError);
    }
}

void render::GeometryConvertor::ParseNode(const aiNode* node, math::Box& boundingBox, math3d::Matrix& transformation, std::vector<GeometryConvertor::GeometryData>& vertextData, const std::string& textureFileName)
{
    YAGET_ASSERT(node, "Assimp Node from a scene is nullptr");
    math3d::Matrix prevTransformation(transformation);

    transformation = transformation * ::conv::ToMatrix(node->mTransformation);

    for (unsigned int n = 0; n < node->mNumMeshes; ++n)
    {
        math::Box currentBoundingBox;

        int meshIndex = node->mMeshes[n];
        const aiMesh* mesh = mLoaderScene->mMeshes[meshIndex];

        vertextData[meshIndex] = GeometryConvertor::GeometryData();

        aiVector3D* cverticies = mesh->mVertices;
        for (unsigned int tc = 0; tc < mesh->mNumVertices; ++tc)
        {
            math3d::Vector3* pos = reinterpret_cast<math3d::Vector3*>(cverticies);
            *pos = math3d::Vector3::Transform(*pos, kSwapYZMatrix);

            currentBoundingBox.GrowExtends(*pos);
            cverticies++;
        }

        boundingBox.GrowExtends(currentBoundingBox);

        // point verticies to asimp model vertex data
        vertextData[meshIndex].mVerticies.first = reinterpret_cast<const float *>(mesh->mVertices);
        vertextData[meshIndex].mVerticies.second = mesh->mNumVertices * 3;

        // point uv's to asimp model uv data if channel exist
        const bool bValidUV = mesh->GetNumUVChannels() && mesh->HasTextureCoords(0);
        const unsigned int numUVs = bValidUV ? mesh->mNumVertices * 3 : 0;
        vertextData[meshIndex].mUVs.first = reinterpret_cast<const float *>(mesh->mTextureCoords[0]);
        vertextData[meshIndex].mUVs.second = numUVs;

        vertextData[meshIndex].mIndices.reserve(mesh->mNumFaces * 3);
        for (unsigned int f = 0; f < mesh->mNumFaces; ++f)
        {
            aiFace* face = &mesh->mFaces[f];
            for (unsigned int i = 0; i < face->mNumIndices; ++i)
            {
                unsigned int vertexIndex = face->mIndices[i];
                vertextData[meshIndex].mIndices.push_back(vertexIndex);
            }

            mMeshCounters.mIndices += face->mNumIndices;
        }

        // extract texture/material name
        if (mLoaderScene->HasMaterials())
        {
            Guid guid = NewGuid();
            const aiMaterial* material = mLoaderScene->mMaterials[mesh->mMaterialIndex];
            aiString textureName;
            if (material->Get(AI_MATKEY_TEXTURE_DIFFUSE(0), textureName) == aiReturn_SUCCESS)
            {
                material; textureName;
                //textureAssetPath = textureName.C_Str();
                //std::string mName;          //! user defined name
                //Guid mGuid;                 //! unique id for this asset

                //std::string mVTSName;       //! Virtual Transport System data tag,                 '%{Levels}/Test/Foo1.pak'
                //std::string mSourceName;    //! Source file which was used to create this data,    Absolute path of source data for this asset, 'c:/Development/Tiles/Test/Foo1.fbx'

                // we do not now vts for it or possibly guid yet, 
                // NOTE: how do we resolve this?
                vertextData[meshIndex].mMaterialTag = { fs::path(textureName.C_Str()).filename().stem().string(), guid, "", textureName.C_Str() };
            }
        }

        vertextData[meshIndex].mBoundingBox = currentBoundingBox;
        vertextData[meshIndex].mMatrix = math3d::Matrix::Identity;

        mMeshCounters.mVerticies += mesh->mNumVertices;
        mMeshCounters.mFaces += mesh->mNumFaces;
    }

    for (unsigned int n = 0; n < node->mNumChildren; ++n)
    {
        ParseNode(node->mChildren[n], boundingBox, transformation, vertextData, textureFileName);
    }

    transformation = prevTransformation;
}

const render::GeometryConvertor::GeometryData& render::GeometryConvertor::GetVertexData(size_t index) const
{
    return mVertextData[index];
}

render::GeometryConvertor::GeometryData& render::GeometryConvertor::GetMutableVertexData(size_t index)
{
    return mVertextData[index];
}

const math::Box& render::GeometryConvertor::BoundingBox() const 
{
    return mBoundingBox;
}


yaget::render::PakConverter::PakConverter(Verticies&& verticies, Uvs&& uvs, Colors&& colors, Indicies&& indicies)
    : mVerticies(verticies)
    , mUvs(uvs)
    , mColors(colors)
    , mIndicies(indicies)
{
    PakData newPak;
    newPak.mVertextBuffer.first = reinterpret_cast<const float *>(&mVerticies[0]);
    newPak.mVertextBuffer.second = mVerticies.size() * 3;

    if (!mUvs.empty())
    {
        newPak.mUVBuffer.first = reinterpret_cast<const float *>(&mUvs[0]);
        newPak.mUVBuffer.second = mUvs.size() * 3;
    }

    if (!mColors.empty())
    {
        newPak.mColorBuffer.first = reinterpret_cast<const float *>(&mColors[0]);
        newPak.mColorBuffer.second = mColors.size() * 4;
    }

    newPak.mIndexBuffer.first = reinterpret_cast<const unsigned int *>(&mIndicies[0]);
    newPak.mIndexBuffer.second = mIndicies.size();

    for (const auto& it : mVerticies)
    {
        newPak.mBoundingBox.GrowExtends(it);
    }

    newPak.mMatrix = math3d::Matrix::Identity;

    mPaks.emplace_back(std::move(newPak));
}

const yaget::render::PakConverter::PakData& yaget::render::PakConverter::GetPakData(size_t index) const
{
    return mPaks[index];
}
