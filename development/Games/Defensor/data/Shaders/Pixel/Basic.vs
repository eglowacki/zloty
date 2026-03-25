// Basic vertex shader ===
struct WorldViewProjection
{
    float4x4 worldViewProj;
};
ConstantBuffer<WorldViewProjection> perObjectConstants : register(b0, space0);


struct Time
{
    float timeValue;
};
ConstantBuffer<Time> timeValueConstants : register(b4, space0);


struct PSInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

PSInput VSMain(float4 position : SV_POSITION, float4 color : COLOR)
{
    PSInput result;

    result.position = position;
    result.color = color * perObjectConstants.worldViewProj[0][0].x + timeValueConstants.timeValue;

    return result;
}
