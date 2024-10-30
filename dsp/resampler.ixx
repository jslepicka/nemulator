module;
#include <algorithm>
#include <cmath>
#include <memory>
#include <stdlib.h>

export module dsp:resampler;
import :audio_filter;

namespace dsp
{
export class c_resampler
{
  public:
    c_resampler(float m, i_audio_filter *pre_filter, i_audio_filter *post_filter)
    {
        this->m = m;
        mf = 0.0f;
        output_buf_index = 0;
        filtered_buf_index = FILTERED_BUF_LEN - 1;

        mf = m - (int)m;
        samples_required = (int)m + 2;

        output_buf = std::make_unique<short[]>(OUTPUT_BUF_LEN);
        filtered_buf = std::make_unique<float[]>(FILTERED_BUF_LEN * 2);

        for (int i = 0; i < FILTERED_BUF_LEN * 2; i++)
            filtered_buf[i] = 0.0f;

        this->pre_filter = pre_filter;
        this->post_filter = post_filter;
    }

    virtual ~c_resampler()
    {
    }
    void set_m(float m)
    {
        this->m = m;
    }
    int get_output_buf(const short **sample_buf)
    {
        *sample_buf = this->output_buf.get();
        return output_buf_index;
    }
    void clear_buf()
    {
        output_buf_index = 0;
    }
    __forceinline void process(float sample)
    {
        filtered_buf[filtered_buf_index] = filtered_buf[filtered_buf_index + FILTERED_BUF_LEN] =
            pre_filter->process(sample);

        if (--samples_required == 0) {
            float y2 = filtered_buf[filtered_buf_index];
            float y1 = filtered_buf[filtered_buf_index + 1];
            float y0 = filtered_buf[filtered_buf_index + 2];
            float ym1 = filtered_buf[filtered_buf_index + 3];

            //4-point, 3rd-order B-spline (x-form)
            //see deip.pdf
            float ym1py1 = ym1 + y1;
            float c0 = 1.0f / 6.0f * ym1py1 + 2.0f / 3.0f * y0;
            float c1 = 1.0f / 2.0f * (y1 - ym1);
            float c2 = 1.0f / 2.0f * ym1py1 - y0;
            float c3 = 1.0f / 2.0f * (y0 - y1) + 1.0f / 6.0f * (y2 - ym1);
            float j = ((c3 * mf + c2) * mf + c1) * mf + c0;

            float extra = 2.0f - mf;
            float n = m - extra;
            mf = n - (int)n;
            samples_required = (int)n + 2;

            static const float max_out = 32767.0f;
            int s = (int)round(post_filter->process(j) * max_out);
            s = std::clamp(s, -32768, 32767);

            output_buf[output_buf_index++] = (short)s;
        }
        filtered_buf_index = (filtered_buf_index - 1) & 0x3;
    }

  private:
    float m, mf;
    int samples_required;
    int output_buf_index;

    static const int OUTPUT_BUF_LEN = 1024;
    static const int FILTERED_BUF_LEN = 4;
    int filtered_buf_index;
    std::unique_ptr<short[]> output_buf;
    std::unique_ptr<float[]> filtered_buf;

    i_audio_filter *pre_filter;
    i_audio_filter *post_filter;
};
} //namespace dsp