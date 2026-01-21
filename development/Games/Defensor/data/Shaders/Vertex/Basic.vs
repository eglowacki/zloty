// Basic vertex shader ===
struct PerObjectConstants
{
    float4x4 worldViewProj;
};
ConstantBuffer<PerObjectConstants> perObjectConstants : register(b3, space0);

struct PSInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

PSInput VSMain(float4 position : SV_POSITION, float4 color : COLOR)
{
    PSInput result;

    result.position = position;
    result.color = color * perObjectConstants.worldViewProj[0][0].x;

    return result;
}
