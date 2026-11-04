//text from c_font.  Positions arrive already in clip space.

struct VS_INPUT
{
    float2 pos : POSITION;
    float2 tex : TEXCOORD;
    float4 color : COLOR;
};

struct PS_INPUT
{
    float4 pos : SV_POSITION;
    float2 tex : TEXCOORD0;
    float4 color : COLOR;
};

PS_INPUT main(VS_INPUT input)
{
    PS_INPUT output;
    output.pos = float4(input.pos, 0.5, 1.0);
    output.tex = input.tex;
    output.color = input.color;
    return output;
}
