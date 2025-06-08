cbuffer cbViewMatrix : register(b0)
{
    matrix View;
};

cbuffer cbProjectionMatrix : register(b1)
{
    matrix Projection;
};

cbuffer cbWorldMatrix : register(b2)
{
    matrix World;
};

cbuffer psConstantBuffer : register(b3)
{
    int1 colorFilterType;
    int1 useLightingType;
    float2 padding[2];
};

cbuffer cbLight : register(b4)
{
    float3 LightPos;
    float lightPad1;

    float3 LightColor;
    float lightPad2;

    float3 ViewPos;
    float lightPad3;
    
    float AmbientStrength;
    int Glossiness;
    float lightPad4[2];
};

cbuffer cbBlurParams : register(b5) 
{
    float Threshold;  // 閾値
    float2 TexSize; // (1 / textureWidth, 1 / textureHeight)
    float BloomIntensity;

    float BlurScale;
    float blurPad1[3];
}

struct VS_INPUT
{
    float4 Pos : POSITION;
    float2 Tex : TEXCOORD0;
    float3 Normal : NORMAL0;
    float4 Col : COLOR0;
    float4 ColorScale : COLOR1;
};

struct VS_OUTPUT
{
    float4 Pos : SV_POSITION;
    float2 Tex : TEXCOORD0;
    float3 WorldPos : POSITION;
    float3 Normal : NORMAL0;
    float4 Col : COLOR0;
    float4 ColorScale : COLOR1;
};

struct VS_FSQOutput 
{
    float4 Pos : SV_POSITION;
    float2 Tex : TEXCOORD;
};