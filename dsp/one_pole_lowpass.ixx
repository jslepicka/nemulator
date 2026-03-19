module;
#include <cassert>
export module dsp:one_pole_lowpass;
import nemulator.std;
import :audio_filter;

namespace dsp
{
//MSVC doesn't have constexpr std::exp.  Using claude generated Taylor series expansion.
constexpr double exp_impl(double x, double term, double sum, int n)
{
    return (term < 1e-15 && term > -1e-15) ? sum : exp_impl(x, term * x / n, sum + term, n + 1);
}

constexpr double cx_exp(double x)
{
    return exp_impl(x, 1.0, 0.0, 1);
}

export template <float fs, float fc> class c_one_pole_lowpass : public i_audio_filter
{
  public:
    c_one_pole_lowpass()
    {
        //claude says cx_exp can be inaccurate for large input values.  check that
        //it's generating the same value as std::exp.
        #ifdef _DEBUG
        float a = 1.0 - std::exp(-2.0 * std::numbers::pi * fc / fs);
        assert(a == alpha);
        #endif
        reset();
    };

    __forceinline float process(float x)
    {
        return y = alpha * x + (1.0f - alpha) * y;
    }

    void reset()
    {
        y = 0.0f;
    }

  private:
    static constexpr float alpha = 1.0 - cx_exp(-2.0 * std::numbers::pi * fc / fs);
    float y;
};
} //namespace dsp