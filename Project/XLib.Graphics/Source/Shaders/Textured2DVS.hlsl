cbuffer Constants : register(b0)
{
	float3x2 transform;
}

struct VSInput
{
	float2 position : POSITION;
    float2 texcoord : TEXCOORD;
};

struct VSOutput
{
	float4 position : SV_Position;
	float2 texcoord : TEXCOORD;
};

VSOutput main(VSInput input)
{
	VSOutput output;
	output.position = float4(mul(float3(input.position, 1.0f), transform), 0.0f, 1.0f);
	output.texcoord = input.texcoord;
	return output;
}