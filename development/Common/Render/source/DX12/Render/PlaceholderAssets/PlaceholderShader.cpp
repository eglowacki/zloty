#include "Render\PlaceholderAssets\PlaceholderAssets.h"

namespace
{
    //-------------------------------------------------------------------------------------------------
    auto buildInShaderSource = R"( 
            struct PSInput
            {
                float4 position : SV_POSITION;
                float4 color : COLOR;
            };

            PSInput VSMain(float4 position : SV_POSITION, float4 color : COLOR)
            {
                PSInput result;

                result.position = position;
                result.color = color;

                return result;
            }

            float4 PSMain(PSInput input) : SV_TARGET
            {
                return input.color;
            }
        )";


    //-------------------------------------------------------------------------------------------------
    constexpr size_t length(std::string_view sv)
    {
        return sv.size();
    }

    const std::size_t buildInShaderSourceLen = length(buildInShaderSource);;

}


//-------------------------------------------------------------------------------------------------
yaget::io::BufferView yaget::render::placeholders::GetShaderData()
{
    uint8_t* data = const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(buildInShaderSource));
    return io::BufferView(data, buildInShaderSourceLen);
}
