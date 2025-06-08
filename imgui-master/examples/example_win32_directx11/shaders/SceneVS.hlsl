#include "Header.hlsli"

// Vertex Shader
VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output = (VS_OUTPUT)0;

    float4 worldPos = mul(input.Pos, World);
    output.Pos = mul(worldPos, View);
    output.Pos = mul(output.Pos, Projection);

    output.Tex = input.Tex;
    output.WorldPos = worldPos.xyz;

    // 法線をワールド座標系に変換
    float4 normal = float4(input.Normal, 0.0f);
    normal = mul(normal, World);

    output.Normal = normal.xyz;

    output.Col = input.Col;
    output.ColorScale = input.ColorScale;

    return output;
}