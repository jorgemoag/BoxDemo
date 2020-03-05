// shaders.hlsl
cbuffer SomeConstants : register(b0)
{
    float4x4 MVP;
}

Texture2D mainTex : register(t0);
SamplerState samplerTex : register(s0);

struct VS2PS
{
    float4 position : SV_Position;
    float2 uv : UV0;
    float4 color : COLOR;
};

VS2PS VSMain(float3 position : POSITION, float2 uv : UV0, float4 color : COLOR)
{
    VS2PS vs2ps;
    vs2ps.position = mul(MVP, float4(position, 1.0));
    vs2ps.uv = uv;
    vs2ps.color = color;
    return vs2ps;
}

float4 PSMain(VS2PS vs2ps) : SV_Target
{
    float4 col = mainTex.Sample(samplerTex, vs2ps.uv);
    return col;
}