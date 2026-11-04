//text from c_font.  The atlas holds only each glyph's alpha, and glyphs are drawn one texel to one pixel.

Texture2D atlas : register(t0);
SamplerState point_sampler : register(s0);

struct PS_INPUT
{
    float4 pos : SV_POSITION;
    float2 tex : TEXCOORD0;
    float4 color : COLOR;
};

float4 main(PS_INPUT input) : SV_Target
{
    return float4(input.color.rgb, input.color.a * atlas.Sample(point_sampler, input.tex).r);
}
