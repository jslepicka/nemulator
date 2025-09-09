module;
#include <immintrin.h>

export module dsp:resampler;
import nemulator.std;
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

        output_buf = std::make_unique<float[]>(OUTPUT_BUF_LEN);
        filtered_buf = std::make_unique<float[]>(FILTERED_BUF_LEN * 2);

        for (int i = 0; i < FILTERED_BUF_LEN * 2; i++)
            filtered_buf[i] = 0.0f;

        this->pre_filter = pre_filter;
        this->post_filter = post_filter;
    }

    void set_m(float m)
    {
        this->m = m;
    }
    int get_output_buf(const float **sample_buf)
    {
        *sample_buf = this->output_buf.get();
        return output_buf_index;
    }
    void clear_buf()
    {
        output_buf_index = 0;
    }

    __forceinline float dot_product(__m128 a, __m128 b)
    {
        __m128 r1 = _mm_mul_ps(a, b);
        __m128 shuf = _mm_shuffle_ps(r1, r1, _MM_SHUFFLE(2, 3, 0, 1));
        __m128 sums = _mm_add_ps(r1, shuf);
        shuf = _mm_movehl_ps(shuf, sums);
        sums = _mm_add_ss(sums, shuf);
        return _mm_cvtss_f32(sums);
    }

    __forceinline void process(float sample)
    {
        filtered_buf[filtered_buf_index] = filtered_buf[filtered_buf_index + FILTERED_BUF_LEN] =
            pre_filter->process(sample);
        #if 1
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

            output_buf[output_buf_index++] = post_filter->process(j);
        }
        #else
        if (--samples_required == 0) {
            //https://en.wikipedia.org/wiki/B-spline#Cubic_B-Splines
            float mf2 = mf * mf;
            float mf3 = mf * mf * mf;
            __m128 a = _mm_set_ps(mf3, mf2, mf, 1.0f);
            __m128 c = _mm_loadu_ps(&filtered_buf[filtered_buf_index]);
            c = _mm_shuffle_ps(c, c, _MM_SHUFFLE(0, 1, 2, 3));
            __m128 b[4] = {
                _mm_set_ps(-1.0f / 6.0f, .5f, -.5f, 1.0f / 6.0f),
                _mm_set_ps(.5f, -1.0f, .5f, 0.0f),
                _mm_set_ps(-.5f, 0.0f, .5f, 0.0f),
                _mm_set_ps(1.0f / 6.0f, 2.0f / 3.0f, 1.0f / 6.0f, 0.0f)
            };

            __m128 ab = _mm_setzero_ps();
            for (int i = 0; i < 4; i++) {
                ab = _mm_add_ps(ab, _mm_mul_ps(_mm_set1_ps(a.m128_f32[3 - i]), b[i]));
            }

            float d = dot_product(ab, c);

            float extra = 2.0f - mf;
            float n = m - extra;
            mf = n - (int)n;
            samples_required = (int)n + 2;

            static const float max_out = 32767.0f;
            int s = (int)round(post_filter->process(d) * max_out);
            s = std::clamp(s, -32768, 32767);

            output_buf[output_buf_index++] = (short)s;
        }
        #endif
        filtered_buf_index = (filtered_buf_index - 1) & 0x3;
    }

  private:
    float m, mf;
    int samples_required;
    int output_buf_index;

    static const int OUTPUT_BUF_LEN = 1024;
    static const int FILTERED_BUF_LEN = 4;
    int filtered_buf_index;
    std::unique_ptr<float[]> output_buf;
    std::unique_ptr<float[]> filtered_buf;

    i_audio_filter *pre_filter;
    i_audio_filter *post_filter;
};
} //namespace dsp