module;

export module dsp:biquad;
import nemulator.std;
import :audio_filter;

namespace dsp
{
export class c_biquad : public i_audio_filter
{
  public:
    c_biquad(float g, const std::array<float, 3> &b, const std::array<float, 3> &a)
    {
        this->g = g;
        this->b = b;
        this->a = a;
    };
    ~c_biquad(){};
    __forceinline float process(float in)
    {
        in *= g;
        float out = in * b[0] + z[0];
        z[1] = in * b[2] - out * a[2];
        z[0] = (in * b[1] + z[1]) - (out * a[1]);
        return out;
    }

  private:
    float g;
    std::array<float, 3> b;
    std::array<float, 3> a;
    std::array<float, 2> z{0.0f, 0.0f};
};
} //namespace dsp