Texture2D txDiffuse;

SamplerState samLinear
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = Clamp;
	AddressV = Clamp;
};

SamplerState samPoint
{
	Filter = MIN_MAG_MIP_POINT;
	AddressU = Clamp;
	AddressV = Clamp;
};

BlendState Blend
{
	BlendEnable[0] = True;
};

matrix World;
matrix View;
matrix Projection;
float color;
bool showBorder;
bool maskSides;
float borderR;
float borderG;
float borderB;
float dim;
float2 output_size;
float3 border_color;

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

float4 PS_BILINEAR (PS_INPUT input) : SV_Target
{
 float4 r = txDiffuse.Sample(samLinear, input.Tex) * color * dim;
 r.w = 1.0;
 return r;
}

float4 PS_SELECTED (PS_INPUT input) : SV_Target
{
 float4 r = { 1.0, 0.0, 0.0, 1.0 };
 return r;
}

float4 PS_POINT (PS_INPUT input) : SV_Target
{
 float4 r = txDiffuse.Sample(samPoint, input.Tex) * color * dim;
 r.w = 1.0;
 return r;
}

float4 PS (PS_INPUT input) : SV_Target
{
if (showBorder && (input.Tex[0] < .01 || input.Tex[0] > .99 || input.Tex[1] < .04125 || input.Tex[1] > .89625))
{
 //float4 r = {borderR * color, borderG * color, borderB * color, 1.0};
 float4 r = {border_color.r * color, border_color.g * color, border_color.b * color, 1.0};
 return r;
}

float2 p = input.Tex.xy;
float2 s = {256.0, 256.0};

p = p * s + .5;

float2 i = floor(p);
float2 f = p - i;

//f = f * f * f * (f * (f * 6.0 - 15.0) + 10.0);

f = 1.0 + -(.5+.5*cos(f*3.141593));

//f = -4.5066 * pow(f, 3) + 6.7599 * pow(f, 2) - 1.3232 * f + .035;
//clamp(f, 0.0, 1.0);

p = i + f;

p = (p - .5) / s;

p = input.Tex.xy;

float4 r = txDiffuse.Sample(samLinear, p) * color;
r.w = 1.0;

return r;
}

float4 PS_BORDER (PS_INPUT input) : SV_TARGET
{
 float4 r = { 1.0, 0.0, 0.0, 0.5 };
 return r;
}


#define FIX(c) max(abs(c), 1e-6)
#define PI 3.141592653589
#define TEX2D(c) pow(txDiffuse.Sample(samLinear, c), float4(inputGamma, inputGamma, inputGamma, inputGamma))
#define inputGamma 2.2
#define outputGamma 2.5
#define distortion 0.00

float2 radialDistortion(float2 coord)
{
 coord *= 1;
 float2 cc = coord - float2(.5, .5);
 float dist = dot(cc, cc) * distortion;
 return (coord + cc * (1.0 + dist) * dist) * 1;
}

float4 scanlineWeights(float distance, float4 color)
{
 float4 wid = 2.0 + 2.0 * pow(color, float4(4.0, 4.0, 4.0, 4.0));
 float4 weights = distance * 3.33333333;
 return 1.7 * exp(-pow(weights * rsqrt(.5 * wid), wid)) / (.6 + .2 * wid);
}


float4 CRT (PS_INPUT input) : SV_Target
{
 float2 one = 1.0 / 256;
 float2 xy = radialDistortion(input.Tex.xy);
 xy = input.Tex.xy;
 float2 ratio_scale = xy * float2(256, 256);
 //float2 uv_ratio = frac(xy * float2(256, 256));// - float2(0.5, .5);
 float2 uv_ratio = frac(ratio_scale);
 xy = (floor(xy * float2(256, 256)) + float2(.5, .5)) / float2(256, 256);
 float4 coeffs = PI * float4(1.0 + uv_ratio.x, uv_ratio.x, 1.0 - uv_ratio.x, 2.0 - uv_ratio.x);
 coeffs = FIX(coeffs);
 coeffs = 2.0 * sin(coeffs) * sin(coeffs / 2.0) / (coeffs * coeffs);
 coeffs /= dot(coeffs, float4(1.0, 1.0, 1.0, 1.0));

 //float4 col = clamp(coeffs.x * TEX2D(xy + float2(-one.x, 0.0)) + coeffs.y * TEX2D(xy) + coeffs.z * TEX2D(xy + float2(one.x, 0.0)) + coeffs.w * TEX2D(xy + float2(one.x, 0.0)), 0.0, 1.0);
 //float4 col2= clamp(coeffs.x * TEX2D(xy + float2(-one.x, one.y)) + coeffs.y * TEX2D(xy + float2(0.0, one.y)) + coeffs.z * TEX2D(xy + one) + coeffs.w * TEX2D(xy + float2(2.0 * one.x, one.y)), 0.0, 1.0);

 float4 col = clamp(mul(coeffs,
	float4x4(
	TEX2D(xy + float2(-one.x, 0.0)),
	TEX2D(xy),
	TEX2D(xy + float2(one.x, 0.0)),
	TEX2D(xy + float2(2.0 * one.x, 0.0))
	)),
	0.0, 1.0);

 float4 col2 = clamp(mul(coeffs,
	float4x4(
	TEX2D(xy + float2(-one.x, one.y)),
	TEX2D(xy + float2(0.0, one.y)),
	TEX2D(xy + one),
	TEX2D(xy + float2(2.0 * one.x, one.y))
	)),
	0.0, 1.0);

 float4 weights = scanlineWeights(abs(uv_ratio.y), col);
 float4 weights2 = scanlineWeights(1.0 - uv_ratio.y, col2);
 float3 mul_res = (col * weights + col2 * weights2).xyz;
 float mod_factor = input.Tex.x * output_size.x * 1;

 float3 dotMaskWeights = lerp (
  float3(1.0, 0.1, 1.0),
  float3(0.1, 1.0, 0.7),
  floor(fmod(mod_factor, 2.0))
 );

 mul_res *= dotMaskWeights;

 mul_res = pow(mul_res, 1.0/outputGamma);
 float4 r = float4(mul_res, 1.0);
 return r;

}


technique10 Render
{
	pass P0
	{
		SetVertexShader(CompileShader(vs_4_0, VS()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_4_0, PS()));
	}

	pass P1
	{

		SetVertexShader(CompileShader(vs_4_0, VS()));
		SetGeometryShader(NULL);
		SetBlendState(Blend, float4 (.5f, .5f, .5f, .5f), 0xFFFFFFFF);
		SetPixelShader(CompileShader(ps_4_0, PS_BORDER()));
	}
}