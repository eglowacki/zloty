Texture2D ObjTexture;
SamplerState ObjSamplerState;
//
//struct PS_INPUT
//{
//    float4 position : SV_POSITION;
//    float4 color    : COLOR;
//    float2 texCoord : TEXCOORD;
//};
//
//float4 main(const PS_INPUT input) : SV_TARGET
//{
//    float4 diffuse = ObjTexture.Sample(ObjSamplerState, input.texCoord);
//    return diffuse;
//}


struct PixelInputType
{
    float4 position : SV_POSITION;
    float4 color    : COLOR;
    float2 texCoord : TEXCOORD;
};


float4 main(const PixelInputType input) : SV_TARGET
{
    float4 diffuse = ObjTexture.Sample(ObjSamplerState, input.texCoord);
    return input.color * diffuse;
}