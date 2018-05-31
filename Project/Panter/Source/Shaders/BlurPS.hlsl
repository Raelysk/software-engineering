cbuffer Constants : register(b0)
{
    int radius;
    float multiplier;
}

Texture2D<float4> defaultTexture : register(t0);

float4 main(float4 position : SV_Position) : SV_TARGET
{
    int2 pixelCoord = int2(position.xy);

    float4 sum = float4(0.0f, 0.0f, 0.0f, 0.0f);

    for (int i = -radius; i <= radius; i++)
    {
        for (int j = -radius; j <= radius; j++)
            sum += defaultTexture.Load(int3(pixelCoord.x + j, pixelCoord.y + i, 0));
    }

    return sum * multiplier;
}