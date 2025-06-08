#include "Header.hlsli"

// Constant Buffer Variables
Texture2D bloomTexture : register(t0);
SamplerState samLinear : register(s0);

float4 main(VS_OUTPUT input) : SV_TARGET
{
    float weights[5] = { 0.252035, 0.215441, 0.134143, 0.059103, 0.017278 };
    float2 texelSize = 1.0 / TexSize;
    
    float3 color = bloomTexture.Sample(samLinear, input.Tex).rgb * weights[0];

    for (int i = 1; i < 5; i++) 
    {
        float2 offset = float2(texelSize.x * i * BlurScale, 0); // スケール
        color += bloomTexture.Sample(samLinear, input.Tex + offset).rgb * weights[i];
        color += bloomTexture.Sample(samLinear, input.Tex - offset).rgb * weights[i];
    }
    
    return float4(color, 1.0f);
}
