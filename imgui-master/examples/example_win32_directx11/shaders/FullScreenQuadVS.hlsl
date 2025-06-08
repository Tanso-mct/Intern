#include "Header.hlsli"

VS_FSQOutput main(VS_INPUT input) 
{
    VS_FSQOutput output;
    
    output.Pos = input.Pos;
    output.Tex = (input.Pos + 1.0) * 0.5;
    output.Tex.y = 1.0 - output.Tex.y;

    return output;
}