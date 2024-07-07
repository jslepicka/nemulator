#pragma once
#define _USE_MATH_DEFINES
#include <math.h>
#include <D3DX10math.h>
#include <algorithm>

template<typename T> T lerp(T start, T end, double mu)
{
    mu = std::clamp(mu, 0.0, 1.0);
    return (T)(start * (1.0 - mu) + end * mu);
}

template<> D3DXVECTOR3 lerp(D3DXVECTOR3 start, D3DXVECTOR3 end, double mu)
{
    mu = std::clamp(mu, 0.0, 1.0);
    D3DXVECTOR3 ret;
    for (int i = 0; i < 3; i++)
    {
        ret[i] = (float)(start[i] * (1.0 - mu) + end[i] * mu);
    }
    return ret;
}

template<typename T> T interpolate_cosine(T start, T end, double mu)
{
    mu = std::clamp(mu, 0.0, 1.0);
    double mu2 = (1.0 - cos(mu * M_PI)) / 2.0;
    return lerp(start, end, mu2);
}