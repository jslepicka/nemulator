Texture2D txDiffuse;

SamplerState samLinear
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = Border;
	AddressV = Border;
	BorderColor = float4(0.0f, 0.0f, 0.0f, 255.0f);
};

matrix World;
matrix View;
matrix Projection;
float color;
bool showBorder;
float3 border_color;
float time;
float2 output_size;
float max_x;
float max_y;
bool scanlines;

//0 = bilinear
//.499999 = nearest neighbor
float sharpness;

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
 float2 scale = {output_size.x / max_x, output_size.y / max_y};
 float2 interp = (scale - lerp(scale, 1.0, sharpness))/(scale * 2.0);
 //interp *= sharpness;
 //saturate(interp);

 float2 p = input.Tex.xy;

 p = p * 512.0 + .5;
 float2 i = floor(p);
 float2 f = p - i;

 f = (f-interp)/(1.0-interp*2.0);
 f = saturate(f);

 p = i + f;
 p = (p - .5) / 512.0;
 float4 r = txDiffuse.Sample(samLinear, p, 0) * color;
 //r = pow(r, 1.0/2.2);
 r.w = 1.0;
	
	if (scanlines)
	{
    
		static const float scanline_gamma = 0.9; // sharpness of the bright band; higher = thinner
		static const float scanline_min = 0.65; // darkest point between scanlines (0=black, 1=no effect)
		float scan_phase = frac(input.Tex.y * 512) * 3.14159;
		float scanline_raw = pow(sin(scan_phase), scanline_gamma);

		// Attenuate scanline effect in the interpolated fringe vs. pixel center
		float pixel_center = 1.0 - abs(f.y * 2.0 - 1.0); // 1 at center, 0 at edges
		float scanline = lerp(scanline_raw, 1.0, pixel_center * 0.3);

		r.rgb *= lerp(scanline_min, 1.0, scanline);
	}

 return r;
}

float4 PS1 (PS_INPUT input) : SV_Target
{
    const float border_width = .01;
    const float lower_border = 0.0 + border_width;
    const float upper_border = 1.0 - border_width;
    bool in_border_range = input.Tex.x < lower_border || input.Tex.x > upper_border || input.Tex.y < lower_border || input.Tex.y > upper_border;
    float4 r = { border_color.r * color, border_color.g * color, border_color.b * color, 1.0 };
    return r * float(in_border_range);
}

DepthStencilState DisableDepth
{
    DepthFunc = LESS_EQUAL;
};

DepthStencilState EnableDepth
{
    DepthFunc = LESS;
};

BlendState NoBlend
{
 BlendEnable[0] = FALSE;
};

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
	   	SetBlendState(NoBlend, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);
		SetVertexShader(CompileShader(vs_4_0, VS()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_4_0, PS()));
		SetDepthStencilState( EnableDepth, 0 );
	}
	pass P1
	{
		SetBlendState( SrcAlphaBlendingAdd, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
		SetVertexShader(CompileShader(vs_4_0, VS()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_4_0, PS1()));
		SetDepthStencilState( DisableDepth, 0 );
	}
}

