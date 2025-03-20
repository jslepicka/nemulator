module;
export module dsp:null_filter;
import :audio_filter;

namespace dsp
{
export class c_null_filter : public i_audio_filter
{
  public:
    __forceinline float process(float in)
    {
        return in;
    }
};
} //namespace dsp