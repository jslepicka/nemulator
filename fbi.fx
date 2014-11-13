/*
Copyright (c) 2014 James Slepicka

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

Texture2D tex;

SamplerState s0
{
 Filter = MIN_MAG_MIP_LINEAR;
 AddressU = Clamp;
 AddressV = Clamp;
};

matrix World;
matrix View;
matrix Projection;

float2 texture_size;
float2 input_size;
float2 output_size;
float sharpness = 1.0;

struct VS_INPUT
{
 float4 pos : POSITION;
 float2 tex : TEXCOORD;
};

struct PS_INPUT
{
 float4 pos : SV_POSITION;
 float2 tex : TEXCOORD0;
};

PS_INPUT VS (VS_INPUT input)
{
 PS_INPUT output = (PS_INPUT)0;
 output.pos = mul (input.pos, World);
 output.pos = mul (output.pos, View);
 output.pos = mul (output.pos, Projection);
 output.tex = input.tex;	
 return output;
}

float4 PS (PS_INPUT input) : SV_Target
{
 float2 scale = output_size / input_size;
 float2 interp = saturate((scale - 1.0)/(scale * 2.0) * sharpness);
 
 float2 p = input.tex.xy;
 p = p * texture_size + .5;
 float2 i = floor(p);
 float2 f = p - i;

 f = saturate((f-interp)/(1.0-interp*2.0));

 p = i + f;
 p = (p - .5) / texture_size;
 float4 r = tex.Sample(s0, p);
 r.a = 1.0;

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
}
