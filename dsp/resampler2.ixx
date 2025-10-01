module;
#include <immintrin.h>

export module dsp:resampler2;
import nemulator.std;
import :audio_filter;

namespace dsp
{
export template <size_t channels, typename pre_f_t, typename post_f_t> class c_resampler2
{
  public:
    using resampler_t = c_resampler2<channels, pre_f_t, post_f_t>;
    static std::unique_ptr<resampler_t> create(float m)
    {
        return std::unique_ptr<resampler_t>(new resampler_t(m));
    }

    void set_m(float value)
    {
        m = value;
    }
    int get_output_buf(size_t channel, const float **sample_buf)
    {
        *sample_buf = this->output_buf[channel].get();
        return output_buf_index;
    }
    void clear_buf()
    {
        output_buf_index = 0;
    }

    __forceinline void process(std::array<float, channels> sample)
    {
        for (int i = 0; i < channels; i++) {
            filtered_buf[i][filtered_buf_index] = filtered_buf[i][filtered_buf_index + FILTERED_BUF_LEN] =
                pre_filters[i]->process(sample[i]);
        }

        if (--samples_required == 0) {
            for (int i = 0; i < channels; i++) {
                float y2 = filtered_buf[i][filtered_buf_index];
                float y1 = filtered_buf[i][filtered_buf_index + 1];
                float y0 = filtered_buf[i][filtered_buf_index + 2];
                float ym1 = filtered_buf[i][filtered_buf_index + 3];

                //4-point, 3rd-order B-spline (x-form)
                //see deip.pdf
                float ym1py1 = ym1 + y1;
                float c0 = 1.0f / 6.0f * ym1py1 + 2.0f / 3.0f * y0;
                float c1 = 1.0f / 2.0f * (y1 - ym1);
                float c2 = 1.0f / 2.0f * ym1py1 - y0;
                float c3 = 1.0f / 2.0f * (y0 - y1) + 1.0f / 6.0f * (y2 - ym1);
                float j = ((c3 * mf + c2) * mf + c1) * mf + c0;
                output_buf[i][output_buf_index] = post_filters[i]->process(j);
            }

            output_buf_index++;
            float extra = 2.0f - mf;
            float n = m - extra;
            mf = n - (int)n;
            samples_required = (int)n + 2;
        }

        filtered_buf_index = (filtered_buf_index - 1) & 0x3;
    }

  private:
    c_resampler2(float m)
    {
        this->m = m;
        mf = 0.0f;
        output_buf_index = 0;
        filtered_buf_index = FILTERED_BUF_LEN - 1;

        mf = m - (int)m;
        samples_required = (int)m + 2;

        for (int i = 0; i < channels; i++) {
            output_buf[i] = std::make_unique<float[]>(OUTPUT_BUF_LEN);
            filtered_buf[i] = std::make_unique<float[]>(FILTERED_BUF_LEN * 2);
            for (int j = 0; j < FILTERED_BUF_LEN * 2; j++) {
                filtered_buf[i][j] = 0.0f;
            }
            pre_filters[i] = std::make_unique<pre_f_t>();
            post_filters[i] = std::make_unique<post_f_t>();
        }
    }
    float m, mf;
    int samples_required;
    int output_buf_index;

    static const int OUTPUT_BUF_LEN = 1024;
    static const int FILTERED_BUF_LEN = 4;
    int filtered_buf_index;

    std::array<std::unique_ptr<float[]>, channels> output_buf;
    std::array<std::unique_ptr<float[]>, channels> filtered_buf;
    std::array<std::unique_ptr<pre_f_t>, channels> pre_filters;
    std::array<std::unique_ptr<post_f_t>, channels> post_filters;
};
} //namespace dsp