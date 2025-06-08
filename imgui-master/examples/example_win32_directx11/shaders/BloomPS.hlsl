#include "Header.hlsli"

// Constant Buffer Variables
Texture2D sceneTexture : register(t0);
SamplerState samLinear : register(s0);

// Pixel Shader
float4 main(VS_FSQOutput input) : SV_TARGET
{
    float3 color = sceneTexture.Sample(samLinear, input.Tex).rgb;

    // 輝度計算 (Luma)
    float brightness = dot(color, float3(0.2126, 0.7152, 0.0722));
    
    float3 bloomColor = (brightness > Threshold) ? color : float3(0.0f, 0.0f, 0.0f);

    return float4(bloomColor, 1.0f);
}
