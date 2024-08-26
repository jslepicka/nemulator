module;
#include <vector>
export module dsp:biquad;
import :audio_filter;

namespace dsp
{
export class c_biquad : public i_audio_filter
{
  public:
    c_biquad(float g, std::vector<float> b, std::vector<float> a)
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
    std::vector<float> b;
    std::vector<float> a;
    std::vector<float> z{0.0f, 0.0f};
};
} //namespace dsp