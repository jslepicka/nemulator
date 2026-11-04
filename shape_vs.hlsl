//shapes drawn over the menus and games in screen space, e.g., the arrow in the quick access menu.  Positions
//arrive already in clip space.  Local is the position within the shape's own square, from -1 to 1 (except
//for the disk indicator, below), and is used to antialias the shape's edges so it stays smooth at any size
//or rotation.

struct VS_INPUT
{
    float4 Pos : POSITION;
    float2 Local : TEXCOORD;
};

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float2 Local : TEXCOORD0;
};

PS_INPUT main(VS_INPUT input)
{
    PS_INPUT output = (PS_INPUT)0;
    output.Pos = input.Pos;
    output.Local = input.Local;
    return output;
}
