module;

export module vec;

//plain vectors for vertices and positions.  matrix math is done with DirectXMath, which has no operators on its
//storage types, and interpolate::lerp needs these
export struct s_vec2
{
    float x;
    float y;
};

export struct s_vec3
{
    float x;
    float y;
    float z;

    s_vec3 operator+(const s_vec3 &v) const
    {
        return {x + v.x, y + v.y, z + v.z};
    }

    s_vec3 operator*(float f) const
    {
        return {x * f, y * f, z * f};
    }
};

//linear rgb, and alpha.  the back buffer is srgb, so these are not the values that end up on the screen.
export struct s_color
{
    float r;
    float g;
    float b;
    float a;
};
