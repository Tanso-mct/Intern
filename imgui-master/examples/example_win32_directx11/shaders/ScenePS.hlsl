#include "Header.hlsli"

// Constant Buffer Variables
Texture2D txDiffuse : register(t0);
SamplerState samLinear : register(s0);

// Pixel Shader
float4 main(VS_OUTPUT input) : SV_TARGET
{
    float4 output = txDiffuse.Sample(samLinear, input.Tex) * input.ColorScale + input.Col * (1 - input.ColorScale);

    if (colorFilterType == 1) // Sepia
    {
        float4 srcColor = output;
        output.r = (srcColor.r * 0.393) + (srcColor.g * 0.769) + (srcColor.b * 0.189);
        output.g = (srcColor.r * 0.349) + (srcColor.g * 0.686) + (srcColor.b * 0.168);
        output.b = (srcColor.r * 0.272) + (srcColor.g * 0.534) + (srcColor.b * 0.131);
    }
    else if (colorFilterType == 2) // Grayscale
    {
        float gray = (output.r * 0.299) + (output.g * 0.587) + (output.b * 0.114);
        output.r = gray;
        output.g = gray;
        output.b = gray;
    }

    if (useLightingType == 1) // 疑似的なライティング
    {
        // 法線を正規化
        float3 normal = normalize(input.Normal);

        // 擬似的な「光」の方向（カメラ方向に固定）
        float3 lightDir = float3(0.0, 0.0, -1.0);

        // ドット積を計算
        float intensity = max(dot(normal, lightDir), 0.0);

        // 色を陰影に基づいて変化
        float3 baseColor = output.rgb;
        float3 shadedColor = baseColor * intensity;

        output.rgb = shadedColor;

        return output;
    }
    else if (useLightingType == 2) // Phong shading
    {
        // 法線を正規化
        float3 normal = normalize(input.Normal);

        // ライト方向ベクトルを正規化
        float3 lightDirVec = normalize(LightPos - input.WorldPos);
        float3 viewDirVec = normalize(ViewPos - input.WorldPos);

        // 環境光の設定
        float3 ambient = AmbientStrength * LightColor;

        // 拡散反射光の計算
        float diff = max(dot(normal, lightDirVec), 0.0);
        float3 diffuse = diff * LightColor;

        // 鏡面反射光の計算
        float3 reflectDir = reflect(lightDirVec, normal);
        float spec = pow(max(dot(viewDirVec, reflectDir), 0.0f), Glossiness);
        float3 specular = spec * LightColor;

        // 最終的な色の計算
        float3 result = (ambient + diffuse + specular) * output.rgb;
        output.rgb = result;

        return output;
    }

    return output;
}
