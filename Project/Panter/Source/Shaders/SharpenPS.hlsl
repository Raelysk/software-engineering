cbuffer Constants : register(b0)
{
    float intensity;
}

Texture2D<float4> defaultTexture : register(t0);

float4 main(float4 position : SV_Position) : SV_Target
{
    int2 pixelCoord = int2(position.xy);

    float4 center = defaultTexture.Load(int3(pixelCoord.x, pixelCoord.y, 0));

    float3 sum = center.xyz * 25.0f;

    sum.xyz -= defaultTexture.Load(int3(pixelCoord.x - 2, pixelCoord.y - 2, 0)).xyz;
    sum.xyz -= defaultTexture.Load(int3(pixelCoord.x - 1, pixelCoord.y - 2, 0)).xyz;
    sum.xyz -= defaultTexture.Load(int3(pixelCoord.x,     pixelCoord.y - 2, 0)).xyz;
    sum.xyz -= defaultTexture.Load(int3(pixelCoord.x + 1, pixelCoord.y - 2, 0)).xyz;
    sum.xyz -= defaultTexture.Load(int3(pixelCoord.x + 2, pixelCoord.y - 2, 0)).xyz;

	sum.xyz -= defaultTexture.Load(int3(pixelCoord.x - 2, pixelCoord.y - 1, 0)).xyz;
    sum.xyz -= defaultTexture.Load(int3(pixelCoord.x - 1, pixelCoord.y - 1, 0)).xyz;
    sum.xyz -= defaultTexture.Load(int3(pixelCoord.x,     pixelCoord.y - 1, 0)).xyz;
    sum.xyz -= defaultTexture.Load(int3(pixelCoord.x + 1, pixelCoord.y - 1, 0)).xyz;
    sum.xyz -= defaultTexture.Load(int3(pixelCoord.x + 2, pixelCoord.y - 1, 0)).xyz;

	sum.xyz -= defaultTexture.Load(int3(pixelCoord.x - 2, pixelCoord.y, 0)).xyz;
    sum.xyz -= defaultTexture.Load(int3(pixelCoord.x - 1, pixelCoord.y, 0)).xyz;
    sum.xyz -= defaultTexture.Load(int3(pixelCoord.x + 1, pixelCoord.y, 0)).xyz;
    sum.xyz -= defaultTexture.Load(int3(pixelCoord.x + 2, pixelCoord.y, 0)).xyz;

	sum.xyz -= defaultTexture.Load(int3(pixelCoord.x - 2, pixelCoord.y + 1, 0)).xyz;
    sum.xyz -= defaultTexture.Load(int3(pixelCoord.x - 1, pixelCoord.y + 1, 0)).xyz;
    sum.xyz -= defaultTexture.Load(int3(pixelCoord.x,     pixelCoord.y + 1, 0)).xyz;
    sum.xyz -= defaultTexture.Load(int3(pixelCoord.x + 1, pixelCoord.y + 1, 0)).xyz;
    sum.xyz -= defaultTexture.Load(int3(pixelCoord.x + 2, pixelCoord.y + 1, 0)).xyz;

	sum.xyz -= defaultTexture.Load(int3(pixelCoord.x - 2, pixelCoord.y + 2, 0)).xyz;
    sum.xyz -= defaultTexture.Load(int3(pixelCoord.x - 1, pixelCoord.y + 2, 0)).xyz;
    sum.xyz -= defaultTexture.Load(int3(pixelCoord.x,     pixelCoord.y + 2, 0)).xyz;
    sum.xyz -= defaultTexture.Load(int3(pixelCoord.x + 1, pixelCoord.y + 2, 0)).xyz;
    sum.xyz -= defaultTexture.Load(int3(pixelCoord.x + 2, pixelCoord.y + 2, 0)).xyz;

    return float4(lerp(center.xyz, sum, intensity), center.w);
}