module;
#include <immintrin.h>
#include <cassert>

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

    int get_output_buf(const float **sample_buf)
    {
        *sample_buf = (float*)&output_buf;
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

                float j;
                if constexpr (method == COMPUTATION_METHOD::horner) {
                    j = ((c3 * mf + c2) * mf + c1) * mf + c0;
                }
                else if constexpr (method == COMPUTATION_METHOD::estrin) {
                    float t = mf * mf;
                    j = (c0 + c1 * mf) + (c2 + c3 * mf) * t;
                }
                else {
                    static_assert(0);
                }
                output_buf[output_buf_index].channel[i] = post_filters[i]->process(j);
            }

            output_buf_index++;

            mf += m;
            samples_required = (int)mf;
            mf -= samples_required;

            assert(mf >= 0.0f);

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

    struct s_output_frame
    {
        float channel[channels];
    };
    
    std::array<s_output_frame, OUTPUT_BUF_LEN> output_buf{};

    std::array<float[FILTERED_BUF_LEN * 2], channels> filtered_buf{};
    std::array<std::unique_ptr<pre_f_t>, channels> pre_filters;
    std::array<std::unique_ptr<post_f_t>, channels> post_filters;

    enum class COMPUTATION_METHOD
    {
        horner,
        estrin
    };

    static constexpr COMPUTATION_METHOD method = COMPUTATION_METHOD::estrin;
};
} //namespace dsp