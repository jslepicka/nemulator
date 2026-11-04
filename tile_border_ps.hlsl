//the border around the selected tile

#include "tile_constants.hlsli"

float4 main(PS_INPUT input) : SV_Target
{
    const float border_width = .01;
    const float lower_border = 0.0 + border_width;
    const float upper_border = 1.0 - border_width;
    bool in_border_range = input.Tex.x < lower_border || input.Tex.x > upper_border || input.Tex.y < lower_border || input.Tex.y > upper_border;
    float4 r = { border_color.r * color, border_color.g * color, border_color.b * color, 1.0 };
    return r * float(in_border_range);
}
