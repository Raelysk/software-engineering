cbuffer Constants : register(b0)
{
	float3x2 transform;
}

struct VSInput
{
	float2 position : POSITION;
	float4 color : COLOR;
};

struct VSOutput
{
	float4 position : SV_Position;
	float4 color : COLOR;
};

VSOutput main(VSInput input)
{
	VSOutput output;
	output.position = float4(mul(float3(input.position, 1.0f), transform), 0.0f, 1.0f);
	output.color = input.color;
	return output;
}