Texture2D<float4> defaultTexture : register(t0);
SamplerState defaultSampler : register(s0);

struct PSInput
{
	float4 position : SV_Position;
	float2 texcoord : TEXCOORD;
};

float4 main(PSInput input) : SV_Target
{
    return defaultTexture.SampleLevel(defaultSampler, input.texcoord, 0.0f);;
}