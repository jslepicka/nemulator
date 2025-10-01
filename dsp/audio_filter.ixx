export module dsp:audio_filter;
import std;
namespace dsp
{
export class i_audio_filter
{
  public:
    virtual float process(float sample) { return sample; };
    virtual std::array<float, 2> process(std::array<float, 2>){return {};};
    virtual ~i_audio_filter(){};
};
}