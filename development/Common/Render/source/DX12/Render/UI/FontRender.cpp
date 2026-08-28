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


    //-------------------------------------------------------------------------------------------------
    void UpdateVertexText(std::vector<DirectX::VertexPositionColor>& vertices, std::vector<uint32_t>& indices, const CharacterGeometryData* quad, size_t numQuads, math3d::Color textColor, uint32_t currentIndexValue, const math3d::Matrix& vertexMatrix)
    {
        auto vertex = vertices.data();
        auto index = indices.data();

        for (int i = 0; i < numQuads; ++i)
        {
            for (int v = 0; v < 4; ++v)
            {
                vertex->position.x = quad->x;
                vertex->position.y = quad->y;
                vertex->position.z = 0;
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
    }


}


//-------------------------------------------------------------------------------------------------
yaget::io::Buffer yaget::render::ui::GetText(const TextPrinters& textPrinters)
{
    size_t sizeNeeded = std::accumulate(textPrinters.begin(), textPrinters.end(), 0uz, [](size_t currentSize, const TextPrinter& textPrinter)
    {
        return currentSize + textPrinter.mText.size() * BytesPerCharacter;
    });

    io::Buffer result;

    if (sizeNeeded)
    {
        std::vector<char> buffer(sizeNeeded);

        std::vector<DirectX::VertexPositionColor> finalVertices;
        std::vector<uint32_t> finalIndices;

        const auto& textPrinterValues = textPrinters.front();
        auto x = textPrinterValues.mX;
        auto y = textPrinterValues.mY;
        auto size = textPrinterValues.mSize;
        auto textColor = textPrinterValues.mColor;
        
        uint32_t currentIndexValue = 0;
        char* bufferPtr = buffer.data();
        int bufferSize = static_cast<int>(buffer.size());
        int textOffsetX = 0;
        int textOffsetY = 0;

        for (const auto& textPrinter : textPrinters)
        {
            size = textPrinter.mSize == -1 ? size : textPrinter.mSize;
            x = textPrinter.mX == TextPrinter::PreviousValue ? static_cast<int>(x + textOffsetX * size): textPrinter.mX;
            y = textPrinter.mY == TextPrinter::PreviousValue ? static_cast<int>(y + textOffsetY * size) : textPrinter.mY;
            textColor = textPrinter.mColor == TextPrinter::PreviousColor ? textColor : textPrinter.mColor;

            textOffsetX += stb_easy_font_width(const_cast<char*>(textPrinter.mText.c_str()));
            //textOffsetY += stb_easy_font_width(const_cast<char*>(textPrinter.mText.c_str()));

            auto numQuads = stb_easy_font_print(0, 0, const_cast<char*>(textPrinter.mText.c_str()), nullptr, bufferPtr, bufferSize);

            std::vector<DirectX::VertexPositionColor> vertices(numQuads * 4);
            std::vector<uint32_t> indices(numQuads * 6);

            auto quad = reinterpret_cast<CharacterGeometryData*>(bufferPtr);

            auto scaleMatrix = math3d::Matrix::CreateScale(size);
            auto positionMatrix = math3d::Matrix::CreateTranslation(static_cast<float>(x), static_cast<float>(y), 0.0f);
            auto vertexMatrix = scaleMatrix * positionMatrix;

            UpdateVertexText(vertices, indices, quad, numQuads, textColor, currentIndexValue, vertexMatrix);

            finalVertices.insert(finalVertices.end(), vertices.begin(), vertices.end());
            finalIndices.insert(finalIndices.end(), indices.begin(), indices.end());

            currentIndexValue += static_cast<uint32_t>(numQuads * 4);
            bufferPtr += numQuads * sizeof(CharacterGeometryData) * 4;
            bufferSize -= numQuads * sizeof(CharacterGeometryData) * 4;
        }

        auto vertexFormat = AssetCacheType::VertexPosition | AssetCacheType::VertexColor;
        result = SerializeToBuffer(vertexFormat, finalVertices, finalIndices, geom::Header::UpdateType::CpuUpload);
    }

    return result;
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
void yaget::render::ui::FontStorage::UpdateText(const io::Tag& tag, const TextPrinters& textPrinters, commands::Type commandType)
{
    auto textBuffer = ui::GetText(textPrinters);

    geom::DataLayout<uint8_t> dataLayout(textBuffer);

    if (auto sceneItem = mSceneItemsStorage.GetSceneItem(tag))
    {
        const auto& geometryTag = sceneItem->GetTags().mGeometriesTags.front();

        if (mGeometryResources.UpdateResourceData(geometryTag, textBuffer))
        {
            auto geometryData = mGeometryResources.GetResource(geometryTag);
            sceneItem->UpdateData(0, constant_shader_types::ConstantTypes::GeometryData, geometryData, commandType);
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
