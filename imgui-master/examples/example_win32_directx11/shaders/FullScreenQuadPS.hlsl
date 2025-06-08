#include "Header.hlsli"

Texture2D srcTexture : register(t0);
SamplerState samLinear : register(s0);

// Pixel Shader
float4 main(VS_FSQOutput input) : SV_TARGET
{
    return srcTexture.Sample(samLinear, input.Tex);
}
