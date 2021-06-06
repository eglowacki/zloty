///////////////////////////////////////////////////////////////////////
// WireFrameVS.hlsl
//
//  Copyright 8/10/2019 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      Used to render in wire frame, only position and flat color
//
//
//////////////////////////////////////////////////////////////////////

cbuffer PerObject : register(b1)
{
    /*row_major*/ matrix worldMatrix;
    float4 color;
};


struct VertexInputType
{
    float3 position : SV_POSITION;
};

struct VertexOutpuType
{
    float4 position : SV_POSITION;
    float4 color    : COLOR;
};

VertexOutpuType main(const VertexInputType input)
{
    VertexOutpuType output;

    // used this as default matrix
    output.position = mul(worldMatrix, float4(input.position, 1));
    output.color = color;

    return output;
}
