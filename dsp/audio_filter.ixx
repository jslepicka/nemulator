export module dsp:audio_filter;

namespace dsp
{
export class i_audio_filter
{
  public:
    virtual float process(float sample) = 0;
    virtual ~i_audio_filter(){};
};
}