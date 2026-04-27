// Basic texture vertex shader ===
struct WorldViewProjection
{
    float4x4 worldViewProj;
};
ConstantBuffer<WorldViewProjection> perObjectConstants : register(b4, space0);


struct Time
{
    float timeValue;
};
ConstantBuffer<Time> timeValueConstants : register(b0, space0);


struct PSInput
{
    float4 position : SV_POSITION;
    float4 color    : COLOR;
    float2 uv       : TEXCOORD0;
};

PSInput VSMain(float3 position : SV_POSITION, float4 color : COLOR, float2 uv : TEXCOORD0)
{
    PSInput result;

    result.position = float4(position, 1);
    //result.color = color * perObjectConstants.worldViewProj[0][0].x + timeValueConstants.timeValue;
    result.color = color * timeValueConstants.timeValue;
    result.uv = uv;

    return result;
}
