//shapes drawn over the menus in screen space, e.g., the arrow in the quick access menu.  Positions arrive
//already in clip space.  Local is the position within the shape's own square, from -1 to 1, and is used to
//antialias the shape's edges so it stays smooth at any size or rotation.

float4 shape_color;
//the shape's rotation (radians, counterclockwise), so its lighting can stay fixed on the screen
float shape_angle;

//where the light comes from on the screen: upper left, in front
static const float2 light_direction = float2(-0.5, 0.7);

//outward normals of the triangle's edges: base, right, left
static const float2 edge_normal_base = float2(0.0, -1.0);
static const float2 edge_normal_right = float2(0.866, 0.5);
static const float2 edge_normal_left = float2(-0.866, 0.5);

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

PS_INPUT VS(VS_INPUT input)
{
    PS_INPUT output = (PS_INPUT)0;
    output.Pos = input.Pos;
    output.Local = input.Local;
    return output;
}

//brightness of a pyramid face that leans out toward the given edge
float face_brightness(float2 edge_normal, float2 light)
{
    float3 normal = normalize(float3(edge_normal * 0.6, 0.8));
    float3 to_light = normalize(float3(light, 1.0));
    //kept light: roughly 80% to 115% of the shape's color
    return 0.2 + 0.9 * saturate(dot(normal, to_light));
}

//an equilateral triangle pointing up: apex at (0, .866), base from (-1, -.866) to (1, -.866), shaded as a
//pyramid seen from the front, with three faces meeting at its center
float4 PS_Triangle(PS_INPUT input) : SV_Target
{
    float2 p = input.Local;

    //signed distance outside each edge; the largest is the distance to the triangle, negative inside
    float d_base = dot(p, edge_normal_base) - 0.866;
    float d_right = dot(p, edge_normal_right) - 0.433;
    float d_left = dot(p, edge_normal_left) - 0.433;
    float outside = max(d_base, max(d_right, d_left));
    //pixels per unit of the shape's own space, so edges blend over a single pixel
    float pixels = 1.0 / max(length(float2(ddx(outside), ddy(outside))), 0.00001);
    float coverage = saturate(0.5 - outside * pixels);

    //the light is fixed on the screen, so turn it into the shape's space
    float s = sin(shape_angle);
    float c = cos(shape_angle);
    float2 light = float2(c * light_direction.x + s * light_direction.y, -s * light_direction.x + c * light_direction.y);

    //each face is the part of the triangle nearest one edge.  weight the faces by distance to the seams
    //between them (1.732 converts to distance across a seam), so the seams blend over a pixel too
    float w_base = saturate(1.0 - (outside - d_base) * pixels / 1.732);
    float w_right = saturate(1.0 - (outside - d_right) * pixels / 1.732);
    float w_left = saturate(1.0 - (outside - d_left) * pixels / 1.732);
    float shade = (w_base * face_brightness(edge_normal_base, light) +
                   w_right * face_brightness(edge_normal_right, light) +
                   w_left * face_brightness(edge_normal_left, light)) /
                  (w_base + w_right + w_left);

    return float4(shape_color.rgb * shade, shape_color.a * coverage);
}

BlendState AlphaBlend
{
    BlendEnable[0] = TRUE;
    SrcBlend = SRC_ALPHA;
    DestBlend = INV_SRC_ALPHA;
    BlendOp = ADD;
};

DepthStencilState NoDepth
{
    DepthEnable = FALSE;
    DepthWriteMask = ZERO;
};

//a rotated shape can face either way
RasterizerState NoCull
{
    CullMode = NONE;
};

technique10 Triangle
{
    pass P0
    {
        SetBlendState(AlphaBlend, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);
        SetDepthStencilState(NoDepth, 0);
        SetRasterizerState(NoCull);
        SetVertexShader(CompileShader(vs_4_0, VS()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_4_0, PS_Triangle()));
    }
}
