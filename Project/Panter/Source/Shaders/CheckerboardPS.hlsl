float4 main(float4 position : SV_Position) : SV_Target
{
    uint x = uint(position.x) / 8;
    uint y = uint(position.y) / 8;

    float c = 0.7f + float((x + y) % 2 == 0) * 0.3f;
    return float4(c.xxx, 1.0f);
}