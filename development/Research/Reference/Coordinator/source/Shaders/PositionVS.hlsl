//cbuffer PerFrame : register(b0)
//{
//    matrix projectionMatrix;
//    matrix viewMatrix;
//};
//
//cbuffer PerObject : register(b1)
//{
//    matrix worldMatrix;
//    float4 color;
//};
//
//
//struct VS_INPUT
//{
//    float3 position : SV_POSITION;
//    float3 texCoord : TEXCOORD;
//};
//
//struct VS_OUTPUT
//{
//    float4 position : SV_POSITION;
//    float4 color    : COLOR;
//    float2 texCoord : TEXCOORD;
//};
//
//
//VS_OUTPUT main(const VS_INPUT input)
//{
//    VS_OUTPUT output;
//
//    matrix mvp = mul(projectionMatrix, mul(viewMatrix, worldMatrix));
//    output.position = mul(mvp, float4(input.position, 1));
//    output.color.xyzw = 1;
//    output.texCoord = input.texCoord.xy;
//
//    return output;
//}


cbuffer PerObject : register(b1)
{
    /*row_major*/ matrix worldMatrix;
    float4 color;
};


struct VertexInputType
{
    float3 position : SV_POSITION;
    float3 texCoord : TEXCOORD;
};

struct VertexOutpuType
{
    float4 position : SV_POSITION;
    float4 color    : COLOR;
    float2 texCoord : TEXCOORD;
};

VertexOutpuType main(const VertexInputType input)
{
    VertexOutpuType output;

    // used thiw with row_major key qualifier for matrix
    //output.position = mul(float4(input.position, 1), worldMatrix);

    // used this as default matrix
    output.position = mul(worldMatrix, float4(input.position, 1));
    output.color = color;
    output.texCoord = input.texCoord.xy;

    return output;
}
