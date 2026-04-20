#include "Render/UI/FontRender.h"
#include "Metrics/PerformanceTracer.h"
#include "Render/Cache/AssetCache.h"
#include "Render/Pipeline/RenderGeometries.h"
#include "Render/Scene/RenderSceneItems.h"

#include "stb_easy_font.h"
#include <VertexTypes.h>


namespace
{
    constexpr size_t BytesPerCharacter = 300;

    //-------------------------------------------------------------------------------------------------
    struct CharacterGeometryData
    {
        float x;
        float y;
        float z;
        uint8_t Color[4];
    };


}


//-------------------------------------------------------------------------------------------------
yaget::io::Buffer yaget::render::ui::GetText(const std::string& text, int x, int y, float size, const math3d::Color* color)
{
    size_t sizeNeeded = text.size() * BytesPerCharacter;
    std::vector<char> buffer(sizeNeeded);

    auto numQuads = stb_easy_font_print(0, 0, const_cast<char*>(text.c_str()), nullptr, buffer.data(), static_cast<int>(buffer.size()));

    math3d::Color textColor = color ? *color : math3d::Color(colors::White);
    std::vector<DirectX::VertexPositionColor> vertices(numQuads * 4);
    std::vector<uint32_t> indices(numQuads * 6);

    uint16_t currentIndexValue = 0;
    auto vertex = vertices.data();
    auto index = indices.data();

    auto quad = reinterpret_cast<CharacterGeometryData*>(buffer.data());
    for (int i = 0; i < numQuads; ++i)
    {
        for (int v = 0; v < 4; ++v)
        {
            vertex->position.x = quad->x;
            vertex->position.y = quad->y;
            vertex->position.z = 0;

            auto scaleMatrix = math3d::Matrix::CreateScale(size);
            auto positionMatrix = math3d::Matrix::CreateTranslation(static_cast<float>(x), static_cast<float>(y), 0.0f);
            auto vertexMatrix = scaleMatrix * positionMatrix;
            vertex->position = math3d::Vector3::Transform(vertex->position, vertexMatrix);

            vertex->color = textColor;

            vertex++;
            quad++;
        }

        index[0] = currentIndexValue + 0;
        index[1] = currentIndexValue + 1;
        index[2] = currentIndexValue + 2;
        index[3] = currentIndexValue + 0;
        index[4] = currentIndexValue + 2;
        index[5] = currentIndexValue + 3;

        currentIndexValue += 4;
        index += 6;
    }

    auto vertexFormat = AssetCacheType::VertexPosition | AssetCacheType::VertexColor;
    auto fontBuffer = SerializeToBuffer(vertexFormat, vertices, indices, geom::Header::UpdateType::CpuUpload);

    return fontBuffer;
}


//-------------------------------------------------------------------------------------------------
yaget::render::ui::FontStorage::FontStorage(RenderGeometries& renderGeometries, GeometriesResources& geometryResources, scene::SceneItemsStorage& sceneItemsStorage, io::VirtualTransportSystem& vts)
    : mRenderGeometries{ renderGeometries }
    , mGeometryResources{ geometryResources }
    , mSceneItemsStorage{ sceneItemsStorage }
    , mVTS{ vts }
{
}


//-------------------------------------------------------------------------------------------------
yaget::render::ui::FontStorage::~FontStorage() = default;


//-------------------------------------------------------------------------------------------------
void yaget::render::ui::FontStorage::UpdateText(const io::Tag& tag, const std::string& text, int x, int y, float size, const math3d::Color* color, commands::Type commandType)
{
    auto textBuffer = ui::GetText(text, x, y, size, color);

    geom::DataLayout<uint8_t> dataLayout(textBuffer);

    if (auto sceneItem = mSceneItemsStorage.GetSceneItem(tag))
    {
        const auto& geometryTag = sceneItem->GetTags().mGeometryTag;

        if (mGeometryResources.UpdateResourceData(geometryTag, textBuffer))
        {
            auto geometryData = mGeometryResources.GetResource(geometryTag);
            sceneItem->UpdateData(0,constant_shader_types::ConstantTypes::GeometryData, geometryData, commandType);
        }
        else
        {
            mSceneItemsStorage.ClearItem(tag);
        }
    }
    else
    {
        YLOG_ERROR("REND", "Missing Font Asset: '%s'.", conv::ToString(tag));
    }
}
