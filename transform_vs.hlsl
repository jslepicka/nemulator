//draws a quad in the 3d scene: the tiles of the menu and the audio overlays

#pragma pack_matrix(row_major)

cbuffer transform_constants : register(b0)
{
    matrix World;
    matrix View;
    matrix Projection;
};

struct VS_INPUT
{
    float4 Pos : POSITION;
    float2 Tex : TEXCOORD;
};

struct PS_INPUT
{
	float4 Pos : SV_POSITION;
	float2 Tex : TEXCOORD0;
};

PS_INPUT main(VS_INPUT input)
{
	PS_INPUT output = (PS_INPUT)0;
	output.Pos = mul (input.Pos, World);
	output.Pos = mul (output.Pos, View);
	output.Pos = mul (output.Pos, Projection);
	output.Tex = input.Tex;
	
	return output;
}
