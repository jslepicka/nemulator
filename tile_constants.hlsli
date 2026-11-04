//what tile_ps.hlsl and tile_border_ps.hlsl share

cbuffer tile_constants : register(b1)
{
    float3 border_color;
    float color;
    float2 output_size;
    float max_x;
    float max_y;
    bool scanlines;
    //0 = bilinear
    //.499999 = nearest neighbor
    float sharpness;
};

struct PS_INPUT
{
	float4 Pos : SV_POSITION;
	float2 Tex : TEXCOORD0;
};
