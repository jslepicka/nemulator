module;

export module dsp:biquad;
import nemulator.std;
import :audio_filter;

namespace dsp
{
export template <float g, float b0, float b1, float b2, float a0, float a1, float a2>
class c_biquad : public i_audio_filter
{
  public:
    c_biquad()
    {
    };

    __forceinline float process(float in)
    {
        in *= g;
        float out = in * b0 + z0;
        z1 = in * b2 - out * a2;
        z0 = (in * b1 + z1) - (out * a1);
        return out;
    }

  private:
    float z0;
    float z1;
};
} //namespace dsp