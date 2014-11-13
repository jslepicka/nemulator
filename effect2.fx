Texture2D txDiffuse;

SamplerState samLinear
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = Clamp;
	AddressV = Clamp;
};

matrix World;
matrix View;
matrix Projection;


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

PS_INPUT VS (VS_INPUT input)
{
	PS_INPUT output = (PS_INPUT)0;
	output.Pos = mul (input.Pos, World);
	output.Pos = mul (output.Pos, View);
	output.Pos = mul (output.Pos, Projection);
	output.Tex = input.Tex;
	
	return output;
}

float4 PS (PS_INPUT input) : SV_Target
{
return txDiffuse.Sample(samLinear, input.Tex);

}

BlendState SrcAlphaBlendingAdd
{
    BlendEnable[0] = TRUE;
    SrcBlend = SRC_ALPHA;
    DestBlend = INV_SRC_ALPHA;
    BlendOp = ADD;
    RenderTargetWriteMask[0] = 0x0F;
};


technique10 Render
{
	pass P0
	{
		SetBlendState( SrcAlphaBlendingAdd, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
		SetVertexShader(CompileShader(vs_4_0, VS()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_4_0, PS()));
	}
}
