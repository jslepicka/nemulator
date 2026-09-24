//shapes drawn over the menus and games in screen space, e.g., the arrow in the quick access menu.  Positions
//arrive already in clip space.  Local is the position within the shape's own square, from -1 to 1 (except
//for the disk indicator, below), and is used to antialias the shape's edges so it stays smooth at any size
//or rotation.

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

//the disk indicator, shown while a disk is being accessed: a panel holding a disk card with a turning hub
//and a bar showing how far the head is across the side.  It's drawn as one quad over the whole panel, with
//Local the position within the panel in pixels.  Sizes below are fractions of the panel's height, and the
//side label is drawn as text to the right of the card, above the bar.  While the disk is written, a pencil
//rides the end of the bar; it can reach past the panel's right edge, so the quad extends past it
float2 disk_size; //the panel's size in pixels
float disk_progress; //how far the head is across the side, from 0 to 1
float disk_writing; //1 while the disk is written, which shows the pencil
float disk_spin; //the hub's rotation, in radians
float disk_alpha; //fades the whole indicator in and out

static const float disk_panel_radius = 0.18;
static const float4 disk_panel_color = float4(0.0, 0.0, 0.0, 0.6);
//the card is drawn in its own units, 28 by 33 with its top left corner at the origin, which is close to the
//proportions of a real disk.  its features follow a real disk as well, but the dips in the top edge are
//deeper, so they show at this size
static const float2 disk_card_origin = float2(0.15, 0.15);
static const float disk_card_scale = 0.7 / 33.0;
static const float2 disk_card_size = float2(28.0, 33.0);
static const float disk_card_radius = 1.0;
static const float disk_card_edge_width = 0.9;
//the top edge dips between tabs at the corners and in the middle.  each dip is centered this far either
//side of the middle, and is dip_half_width wide on each side at the top edge, with sides slanting in by
//dip_slant over its depth
static const float disk_dip_offset = 6.6;
static const float disk_dip_half_width = 3.75;
static const float disk_dip_slant = 0.6;
static const float disk_dip_depth = 1.4;
//the window where the disk shows through, down the middle from the top, and the hub below it, with the
//case between them
static const float2 disk_window_lo = float2(12.7, 1.6);
static const float2 disk_window_hi = float2(15.3, 10.6);
static const float disk_window_radius = 0.4;
static const float2 disk_hub_center = float2(14.0, 14.8);
static const float disk_hub_radius = 3.0; //the disk showing around the metal hub
static const float disk_metal_radius = 2.5;
//a line on the metal hub, from its center almost to its edge, turns with the disk like a clock's hand
static const float disk_hand_length = 1.95;
static const float disk_hand_half_width = 0.4;
//the label, trimmed in black along its top and bottom
static const float2 disk_label_lo = float2(4.4, 20.8);
static const float2 disk_label_hi = float2(23.6, 28.9);
static const float disk_label_radius = 0.4;
static const float disk_label_trim = 1.25;
//the yellow of a real disk (#EFBC1A), which the Disk System's mascot shares
static const float4 disk_card_color = float4(0.937, 0.737, 0.102, 1.0);
static const float4 disk_card_edge_color = float4(0.541, 0.384, 0.0, 1.0);
static const float4 disk_media_color = float4(0.16, 0.14, 0.12, 1.0);
static const float4 disk_metal_color = float4(0.82, 0.82, 0.82, 1.0);
static const float4 disk_label_color = float4(0.784, 0.784, 0.784, 1.0);
static const float4 disk_label_trim_color = float4(0.0, 0.0, 0.0, 1.0);
static const float disk_bar_left = 0.88;
static const float disk_bar_right_margin = 0.15;
static const float disk_bar_top = 0.64;
static const float disk_bar_bottom = 0.72;
static const float4 disk_bar_track_color = float4(0.2, 0.2, 0.2, 1.0);
static const float4 disk_bar_color = float4(0.94, 0.94, 0.94, 1.0);
//the pencil points down and to the left, with its tip where the bar ends.  lengths are measured along it
//from the tip: the sharpened wood ends in the lead, and the body ends in a metal band and the eraser
static const float2 disk_pencil_direction = float2(0.7071, -0.7071); //from the tip toward the eraser
static const float disk_pencil_length = 0.42;
static const float disk_pencil_half_width = 0.055;
static const float disk_pencil_point = 0.09;
static const float disk_pencil_lead = 0.035;
static const float disk_pencil_band = 0.33;
static const float disk_pencil_eraser = 0.37;
static const float disk_pencil_outline = 1.2; //in pixels, so it stands out over the bar and the panel
static const float3 disk_pencil_outline_color = float3(0.08, 0.08, 0.08);
static const float3 disk_pencil_wood_color = float3(0.93, 0.78, 0.55);
static const float3 disk_pencil_lead_color = float3(0.2, 0.2, 0.22);
static const float3 disk_pencil_body_color = float3(0.98, 0.8, 0.1);
static const float3 disk_pencil_shade_color = float3(0.735, 0.6, 0.075); //the body's far side
static const float3 disk_pencil_band_color = float3(0.72, 0.72, 0.76);
static const float3 disk_pencil_eraser_color = float3(0.95, 0.55, 0.6);

//signed distance from p to a rectangle from lo to hi with rounded corners, negative inside
float rounded_rect(float2 p, float2 lo, float2 hi, float radius)
{
    float2 d = abs(p - (lo + hi) * 0.5) - ((hi - lo) * 0.5 - radius);
    return length(max(d, 0.0)) + min(max(d.x, d.y), 0.0) - radius;
}

//signed distance from p to a line with round ends, from the origin to end
float line_from_origin(float2 p, float2 end, float half_width)
{
    float t = saturate(dot(p, end) / dot(end, end));
    return length(p - end * t) - half_width;
}

//signed distance from p to an isosceles trapezoid centered on the origin, with y up: r1 is half its width at
//its bottom (y = -half_height), and r2 half its width at its top
float trapezoid(float2 p, float r1, float r2, float half_height)
{
    float2 k1 = float2(r2, half_height);
    float2 k2 = float2(r2 - r1, 2.0 * half_height);
    p.x = abs(p.x);
    float2 ca = float2(p.x - min(p.x, p.y < 0.0 ? r1 : r2), abs(p.y) - half_height);
    float2 cb = p - k1 + k2 * saturate(dot(k1 - p, k2) / dot(k2, k2));
    float s = (cb.x < 0.0 && ca.y < 0.0) ? -1.0 : 1.0;
    return s * sqrt(min(dot(ca, ca), dot(cb, cb)));
}

//signed distance from c (in the card's units) to the disk card, with the dips cut out of its top edge
float disk_card(float2 c)
{
    float d = rounded_rect(c, float2(0.0, 0.0), disk_card_size, disk_card_radius);
    //each dip is a trapezoid centered on the top edge, reaching as far above it as below, so it's
    //dip_half_width wide at the edge.  y is flipped so the narrow end is the bottom of the dip
    float r1 = disk_dip_half_width - disk_dip_slant;
    float r2 = disk_dip_half_width + disk_dip_slant;
    float middle = disk_card_size.x * 0.5;
    float left = trapezoid(float2(c.x - (middle - disk_dip_offset), -c.y), r1, r2, disk_dip_depth);
    float right = trapezoid(float2(c.x - (middle + disk_dip_offset), -c.y), r1, r2, disk_dip_depth);
    return max(d, -min(left, right));
}

//lays color over the premultiplied color under it, where the shape's signed distance is d pixels, so its
//edges blend over a single pixel
float4 layer(float4 under, float4 color, float d)
{
    float a = color.a * saturate(0.5 - d);
    return float4(color.rgb * a, a) + under * (1.0 - a);
}

float4 PS_Disk(PS_INPUT input) : SV_Target
{
    float height = disk_size.y;
    float2 p = input.Local / height;
    float width = disk_size.x / height;
    float4 color = float4(0.0, 0.0, 0.0, 0.0);

    color = layer(color, disk_panel_color, rounded_rect(p, float2(0.0, 0.0), float2(width, 1.0), disk_panel_radius) * height);

    //in the card's own units
    float2 c = (p - disk_card_origin) / disk_card_scale;
    float to_pixels = disk_card_scale * height;
    float card = disk_card(c);
    color = layer(color, disk_card_edge_color, card * to_pixels);
    color = layer(color, disk_card_color, (card + disk_card_edge_width) * to_pixels);
    color = layer(color, disk_media_color, rounded_rect(c, disk_window_lo, disk_window_hi, disk_window_radius) * to_pixels);

    float2 hub = c - disk_hub_center;
    color = layer(color, disk_media_color, (length(hub) - disk_hub_radius) * to_pixels);
    color = layer(color, disk_metal_color, (length(hub) - disk_metal_radius) * to_pixels);
    float2 hand = disk_hand_length * float2(cos(disk_spin), sin(disk_spin));
    color = layer(color, disk_media_color, line_from_origin(hub, hand, disk_hand_half_width) * to_pixels);

    color = layer(color, disk_label_trim_color, rounded_rect(c, disk_label_lo, disk_label_hi, disk_label_radius) * to_pixels);
    float2 inset = float2(0.0, disk_label_trim);
    color = layer(color, disk_label_color, rounded_rect(c, disk_label_lo + inset, disk_label_hi - inset, 0.0) * to_pixels);

    float2 bar_lo = float2(disk_bar_left, disk_bar_top);
    float2 bar_hi = float2(width - disk_bar_right_margin, disk_bar_bottom);
    float bar_radius = (disk_bar_bottom - disk_bar_top) * 0.5;
    float track = rounded_rect(p, bar_lo, bar_hi, bar_radius);
    color = layer(color, disk_bar_track_color, track * height);
    float fill_right = lerp(bar_lo.x, bar_hi.x, saturate(disk_progress));
    color = layer(color, disk_bar_color, max(track, p.x - fill_right) * height);

    if (disk_writing > 0.0)
    {
        //in the pencil's own space: x along it from the tip, y across it
        float2 q = p - float2(fill_right, (disk_bar_top + disk_bar_bottom) * 0.5);
        float2 across = float2(-disk_pencil_direction.y, disk_pencil_direction.x);
        float2 pencil = float2(dot(q, disk_pencil_direction), dot(q, across));
        float half_width = disk_pencil_half_width;
        float half_point = disk_pencil_point * 0.5;
        float tip = trapezoid(float2(pencil.y, pencil.x - half_point), 0.0, half_width, half_point);
        float rest = rounded_rect(pencil, float2(half_point, -half_width * 1.05),
                                  float2(disk_pencil_length, half_width * 1.05), half_width * 0.6);
        color = layer(color, float4(disk_pencil_outline_color, disk_writing),
                      min(tip, rest) * height - disk_pencil_outline);
        color = layer(color, float4(disk_pencil_wood_color, disk_writing), tip * height);
        color = layer(color, float4(disk_pencil_lead_color, disk_writing),
                      max(tip, pencil.x - disk_pencil_lead) * height);
        float2 body_lo = float2(disk_pencil_point - 0.003, -half_width);
        color = layer(color, float4(disk_pencil_body_color, disk_writing),
                      rounded_rect(pencil, body_lo, float2(disk_pencil_band, half_width), 0.0) * height);
        color = layer(color, float4(disk_pencil_shade_color, disk_writing),
                      rounded_rect(pencil, float2(body_lo.x, half_width * 0.35), float2(disk_pencil_band, half_width), 0.0) * height);
        color = layer(color, float4(disk_pencil_band_color, disk_writing),
                      rounded_rect(pencil, float2(disk_pencil_band, -half_width * 1.05),
                                   float2(disk_pencil_eraser, half_width * 1.05), 0.0) * height);
        color = layer(color, float4(disk_pencil_eraser_color, disk_writing),
                      rounded_rect(pencil, float2(disk_pencil_eraser, -half_width),
                                   float2(disk_pencil_length, half_width), half_width * 0.6) * height);
    }

    return float4(color.rgb / max(color.a, 0.0001), color.a * disk_alpha);
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

technique10 Disk
{
    pass P0
    {
        SetBlendState(AlphaBlend, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);
        SetDepthStencilState(NoDepth, 0);
        SetRasterizerState(NoCull);
        SetVertexShader(CompileShader(vs_4_0, VS()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_4_0, PS_Disk()));
    }
}
