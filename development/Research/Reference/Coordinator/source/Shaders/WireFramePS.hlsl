///////////////////////////////////////////////////////////////////////
// WireFramePS.hlsl
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



struct PixelInputType
{
    float4 position : SV_POSITION;
    float4 color    : COLOR;
};


float4 main(const PixelInputType input) : SV_TARGET
{
    return float4(input.color.rgba);
}