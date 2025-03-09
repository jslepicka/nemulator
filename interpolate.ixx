module;

export module interpolate;
import nemulator.std;

export namespace interpolate
{
template <typename T> T lerp(T start, T end, double mu)
{
    #pragma warning(push)
    #pragma warning(disable : 4244)
    mu = std::clamp(mu, 0.0, 1.0);
    T ret = start * (1.0 - mu) + end * mu;
    #pragma warning(pop)
    return ret;
}

template <typename T> T interpolate_cosine(T start, T end, double mu)
{
    mu = std::clamp(mu, 0.0, 1.0);
    double mu2 = (1.0 - std::cos(mu * std::numbers::pi)) / 2.0;
    return lerp(start, end, mu2);
}
} //namespace interpolate