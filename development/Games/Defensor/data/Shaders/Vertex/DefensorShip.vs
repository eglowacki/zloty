struct WorldViewProjection
{
    float4x4 worldViewProj;
};
ConstantBuffer<WorldViewProjection> perObjectConstants : register(b0, space0);


struct PSInput
{
    float4 position : SV_POSITION;
    float4 color    : COLOR;
    float2 uv       : TEXCOORD0;
};


PSInput VSMain(float3 position : SV_POSITION, float4 color : COLOR, float2 uv : TEXCOORD0)
{
    PSInput result;

    result.position = mul(float4(position, 1), perObjectConstants.worldViewProj);
    result.color = color;
    result.uv = uv;

    return result;
}
