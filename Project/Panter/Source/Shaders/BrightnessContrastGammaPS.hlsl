cbuffer Constants : register(b0)
{
    float brightness;
    float contrast;
    float gamma;
}

Texture2D<float4> defaultTexture : register(t0);
SamplerState defaultSampler : register(s0);

struct PSInput
{
    float4 position : SV_Position;
    float2 texcoord : TEXCOORD;
};

float4 main(PSInput input) : SV_Target
{
    float4 color = defaultTexture.SampleLevel(defaultSampler, input.texcoord, 0.0f);

    color.xyz = (color.xyz - 0.5f) * contrast + 0.5f;
    color.xyz += brightness;
    color.xyz = pow(color.xyz, gamma);

    return color;
}