Texture2D<float4> defaultTexture : register(t0);

SamplerState defaultSampler : register(s0);

struct PSInput
{
	float4 position : SV_Position;
	float2 texcoord : TEXCOORD;
};

float4 main(PSInput input) : SV_Target
{
    //uint2 px = uint2(input.position.xy) / 16;
    //int a = (px.x + px.y) % 2 == 0;
    //return a == 0 ? float4(1, 1, 1, 1) : float4(0.7f, 0.7f, 0.7f, 1);

	///////
	float4 color = defaultTexture.SampleLevel(defaultSampler, input.texcoord, 0.0f);
    return color;
}