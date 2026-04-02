#include "Render\PlaceholderAssets\PlaceholderAssets.h"

namespace
{
    yaget::Strings builtInGeometryData = {
        "VertexFormat: VertexPosition | VertexColor | VertexTexture0"
        "Vertices Begin:"
            "{ -1.0f,  1.0f , 0.0f }; { 1.0f, 0.0f, 0.0f, 0.99f }; { 0.0f, 0.0f }"
            "{  1.0f,  1.0f , 0.0f }; { 0.0f, 1.0f, 0.0f, 0.0f };  { 1.0f, 0.0f }"
            "{ -1.0f, -1.0f , 0.0f }; { 0.0f, 0.0f, 1.0f, 0.0f };  { 0.0f, 1.0f }"
            "{  1.0f, -1.0f , 0.0f }; { 0.0f, 0.0f, 1.0f, 0.0f };  { 1.0f, 1.0f }"
        "Vertices End:"
        "Indices Begin:"
            "0, 1, 2"
            "1, 3, 2"
        "Indices End:"
    };
    
}

//-------------------------------------------------------------------------------------------------
yaget::Strings yaget::render::placeholders::GetGeometryData()
{
    return builtInGeometryData;
}
