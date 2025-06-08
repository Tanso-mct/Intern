#include "Header.hlsli"

// Constant Buffer Variables
Texture2D sceneTexture : register(t0);
Texture2D vertBlurTexture : register(t1);
SamplerState samLinear : register(s0);

// Pixel Shader
float4 main(VS_FSQOutput input) : SV_TARGET
{
    float3 sceneColor = sceneTexture.Sample(samLinear, input.Tex).rgb;
    float3 bloomColor = vertBlurTexture.Sample(samLinear, input.Tex).rgb;
    
    return float4(sceneColor + bloomColor * BloomIntensity, 1.0f);
}
