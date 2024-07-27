module;
#include <numbers>
#include <algorithm>

export module interpolate;

export namespace interpolate
{
template <typename T> T lerp(T start, T end, double mu)
{
    mu = std::clamp(mu, 0.0, 1.0);
    return (T)(start * (1.0 - mu) + end * mu);
}

template <typename T> T interpolate_cosine(T start, T end, double mu)
{
    mu = std::clamp(mu, 0.0, 1.0);
    double mu2 = (1.0 - cos(mu * std::numbers::pi)) / 2.0;
    return lerp(start, end, mu2);
}
} //namespace interpolate