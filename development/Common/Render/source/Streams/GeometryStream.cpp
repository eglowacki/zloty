#include "Streams/GeometryStream.h"
#include "Loaders/GeometryConvertor.h"
#include "App/AppUtilities.h"
#include "geometry_stream_generated.h"
#include <fstream>
#include <filesystem>


using namespace yaget;
namespace fs = std::filesystem;

namespace
{
    size_t GetIStreamSize(std::istream& istream)
    {
        size_t length = istream.tellg();
        if (length != static_cast<size_t>(-1))
        {
            istream.seekg(0, std::ios::end);
            length = istream.tellg();
            istream.seekg(0, std::ios::beg);
            return length;
        }

        return 0;
    }

    size_t GetOStreamSize(std::ostream& ostream)
    {
        size_t length = ostream.tellp();
        return length = (length != static_cast<size_t>(-1)) ? length : 0;
    }

#ifdef YAGET_RELEASE
    constexpr bool verify_geometry_buffer = false;
#else
    constexpr bool verify_geometry_buffer = true;
#endif // YAGET_RELEASE

} // namespace

size_t io::size(std::istream& stream) 
{ 
    return GetIStreamSize(stream); 
}

size_t io::size(std::ostream& stream)
{
    return GetOStreamSize(stream);
}

void io::ReadStream(std::istream& file, io::Buffer& buffer)
{
    char* pBuffer = reinterpret_cast<char*>(buffer.first.get());
    file.read(pBuffer, buffer.second);
}

bool io::WriteStream(std::ofstream& outStream, const io::Buffer& dataBuffer)
{
    const char* data = reinterpret_cast<const char*>(dataBuffer.first.get());
    outStream.write(data, dataBuffer.second);
    return outStream.good();
}

std::vector<io::GeometryData> io::DeserializeBuffer(io::Buffer buffer)
{
    std::vector<io::GeometryData> mesh;

    bool validMesh = true;
    if constexpr (verify_geometry_buffer)
    {
        flatbuffers::Verifier verifier(buffer.first.get(), buffer.second);
        validMesh = io::fb::VerifyMeshBuffer(verifier);
    }

    if (validMesh)
    {
        auto fbMesh = io::fb::GetMesh(buffer.first.get());

        auto fbStreams = fbMesh->Streams();
        for (auto&& it : (*fbStreams))
        {
            auto position = it->Position();
            auto indices = it->Indices();

            auto bbMin = it->BoundingMin();
            auto bbMax = it->BoundingMax();


            io::GeometryData geometryData;
            if (const io::fb::Tag* materialTag = it->MaterialTag())
            {
                geometryData.mMaterialTag = { materialTag->Name()->c_str(), Guid(materialTag->Guid()->c_str()), materialTag->VTSKey()->c_str() };
            }

            geometryData.mBoundingBox.mMin = math3d::Vector3(bbMin->x(), bbMin->y(), bbMin->z());
            geometryData.mBoundingBox.mMax = math3d::Vector3(bbMax->x(), bbMax->y(), bbMax->z());

            geometryData.mVerticies.first = position->data();
            geometryData.mVerticies.second = position->size();

            if (auto uv = it->UV())
            {
                geometryData.mUVs.first = uv->data();
                geometryData.mUVs.second = uv->size();
            }

            geometryData.mIndices.first = indices->data();
            geometryData.mIndices.second = indices->size();
            geometryData.mBufferHandle = buffer;

            mesh.push_back(geometryData);
        }
    }

    return mesh;
}

io::Buffer io::ConvertToPak(const yaget::render::PakConverter& pakConverter)
{
    math::Box boundingBox;

    flatbuffers::FlatBufferBuilder builder;
    std::vector<flatbuffers::Offset<io::fb::GeometryStream>> streams;

    for (size_t i = 0; i < pakConverter.NumPaks(); ++i)
    {
        const auto& pakData = pakConverter.GetPakData(i);

        // geometry points
        auto posBuffer = builder.CreateVector(pakData.mVertextBuffer.first, pakData.mVertextBuffer.second);
        auto uvBuffer = builder.CreateVector(pakData.mUVBuffer.first, pakData.mUVBuffer.second);
        auto indicesBuffer = builder.CreateVector(pakData.mIndexBuffer.first, pakData.mIndexBuffer.second);

        // extends (bounding box) and matrix
        boundingBox.GrowExtends(pakData.mBoundingBox);
        auto bbMin = io::fb::Vec3(pakData.mBoundingBox.mMin.x, pakData.mBoundingBox.mMin.y, pakData.mBoundingBox.mMin.z);
        auto bbMax = io::fb::Vec3(pakData.mBoundingBox.mMax.x, pakData.mBoundingBox.mMax.y, pakData.mBoundingBox.mMax.z);

        math3d::Vector3 upMatrix = pakData.mMatrix.Up();
        math3d::Vector3 rightMatrix = pakData.mMatrix.Right();
        math3d::Vector3 backwardMatrix = pakData.mMatrix.Backward();
        math3d::Vector3 translationMatrix = pakData.mMatrix.Translation();
        auto up = io::fb::Vec3(upMatrix.x, upMatrix.y, upMatrix.z);
        auto right = io::fb::Vec3(rightMatrix.x, rightMatrix.y, rightMatrix.z);
        auto backward = io::fb::Vec3(backwardMatrix.x, backwardMatrix.y, backwardMatrix.z);
        auto translation = io::fb::Vec3(translationMatrix.x, translationMatrix.y, translationMatrix.z);

        // build actual flat buffer based on data created above
        io::fb::GeometryStreamBuilder geometryBuilder(builder);
        //geometryBuilder.add_MaterialTag(materialTag);
        geometryBuilder.add_Position(posBuffer);
        geometryBuilder.add_UV(uvBuffer);
        geometryBuilder.add_Indices(indicesBuffer);
        geometryBuilder.add_BoundingMin(&bbMin);
        geometryBuilder.add_BoundingMax(&bbMax);
        geometryBuilder.add_MatrixUp(&up);
        geometryBuilder.add_MatrixRight(&right);
        geometryBuilder.add_MatrixBackward(&backward);
        geometryBuilder.add_MatrixTranslation(&translation);
        auto geometryStream = geometryBuilder.Finish();

        streams.push_back(geometryStream);
    }

    auto geomteryStreams = builder.CreateVector(streams);
    auto bbMin = io::fb::Vec3(boundingBox.mMin.x, boundingBox.mMin.y, boundingBox.mMin.z);
    auto bbMax = io::fb::Vec3(boundingBox.mMax.x, boundingBox.mMax.y, boundingBox.mMax.z);

    io::fb::MeshBuilder meshBuilder(builder);
    meshBuilder.add_Streams(geomteryStreams);
    meshBuilder.add_BoundingMin(&bbMin);
    meshBuilder.add_BoundingMax(&bbMax);
    auto mesh = meshBuilder.Finish();
    io::fb::FinishMeshBuffer(builder, mesh);

    // we are finished with flat buffer, so now create our own memory buffer and copy content into it
    // NOTE: There is a way to detach memory from flat buffer, but it returns it's own structure to manage memory
    io::Buffer geometryBuffer = io::CreateBuffer(builder.GetSize());
    std::memcpy(geometryBuffer.first.get(), builder.GetBufferPointer(), geometryBuffer.second);

    return geometryBuffer;
}

io::Buffer io::SerializeConvertor(const render::GeometryConvertor& convertor)
{
    // look for answer to array of tables
    // https://github.com/google/flatbuffers/issues/4151
    
    math::Box boundingBox;

    flatbuffers::FlatBufferBuilder builder;
    std::vector<flatbuffers::Offset<io::fb::GeometryStream>> streams;

    for (size_t i = 0; i < convertor.NumMeshes(); ++i)
    {
        const render::GeometryConvertor::GeometryData& geometryData = convertor.GetVertexData(i);

        // geometry points
        auto posBuffer = builder.CreateVector(geometryData.mVerticies.first, geometryData.mVerticies.second);
        auto uvBuffer = builder.CreateVector(geometryData.mUVs.first, geometryData.mUVs.second);
        auto indicesBuffer = builder.CreateVector(reinterpret_cast<const unsigned int*>(geometryData.mIndices.data()), geometryData.mIndices.size());

        auto materialName = builder.CreateString(geometryData.mMaterialTag.mName);
        auto materialGuid = builder.CreateString(geometryData.mMaterialTag.mGuid.str());
        auto materialVTS = builder.CreateString(geometryData.mMaterialTag.mVTSName);

        // extends (bounding box) and matrix
        boundingBox.GrowExtends(geometryData.mBoundingBox);
        auto bbMin = io::fb::Vec3(geometryData.mBoundingBox.mMin.x, geometryData.mBoundingBox.mMin.y, geometryData.mBoundingBox.mMin.z);
        auto bbMax = io::fb::Vec3(geometryData.mBoundingBox.mMax.x, geometryData.mBoundingBox.mMax.y, geometryData.mBoundingBox.mMax.z);

        math3d::Vector3 upMatrix = geometryData.mMatrix.Up();
        math3d::Vector3 rightMatrix = geometryData.mMatrix.Right();
        math3d::Vector3 backwardMatrix = geometryData.mMatrix.Backward();
        math3d::Vector3 translationMatrix = geometryData.mMatrix.Translation();
        auto up = io::fb::Vec3(upMatrix.x, upMatrix.y, upMatrix.z);
        auto right = io::fb::Vec3(rightMatrix.x, rightMatrix.y, rightMatrix.z);
        auto backward = io::fb::Vec3(backwardMatrix.x, backwardMatrix.y, backwardMatrix.z);
        auto translation = io::fb::Vec3(translationMatrix.x, translationMatrix.y, translationMatrix.z);

        io::fb::TagBuilder tagBuilder(builder);
        tagBuilder.add_Name(materialName);
        tagBuilder.add_Guid(materialGuid);
        tagBuilder.add_VTSKey(materialVTS);
        auto materialTag = tagBuilder.Finish();

        // build actual flat buffer based on data created above
        io::fb::GeometryStreamBuilder geometryBuilder(builder);
        geometryBuilder.add_MaterialTag(materialTag);
        geometryBuilder.add_Position(posBuffer);
        geometryBuilder.add_UV(uvBuffer);
        geometryBuilder.add_Indices(indicesBuffer);
        geometryBuilder.add_BoundingMin(&bbMin);
        geometryBuilder.add_BoundingMax(&bbMax);
        geometryBuilder.add_MatrixUp(&up);
        geometryBuilder.add_MatrixRight(&right);
        geometryBuilder.add_MatrixBackward(&backward);
        geometryBuilder.add_MatrixTranslation(&translation);
        auto geometryStream = geometryBuilder.Finish();

        streams.push_back(geometryStream);
    }

    auto geomteryStreams = builder.CreateVector(streams);
    auto bbMin = io::fb::Vec3(boundingBox.mMin.x, boundingBox.mMin.y, boundingBox.mMin.z);
    auto bbMax = io::fb::Vec3(boundingBox.mMax.x, boundingBox.mMax.y, boundingBox.mMax.z);

    io::fb::MeshBuilder meshBuilder(builder);
    meshBuilder.add_Streams(geomteryStreams);
    meshBuilder.add_BoundingMin(&bbMin);
    meshBuilder.add_BoundingMax(&bbMax);
    auto mesh = meshBuilder.Finish();
    io::fb::FinishMeshBuffer(builder, mesh);

    // we are finished with flat buffer, so now create our own memory buffer and copy content into it
    // NOTE: There is a way to detach memory from flat buffer, but it returns it's own structure to manage memory
    io::Buffer geometryBuffer = io::CreateBuffer(builder.GetSize());
    std::memcpy(geometryBuffer.first.get(), builder.GetBufferPointer(), geometryBuffer.second);

    return geometryBuffer;
}
