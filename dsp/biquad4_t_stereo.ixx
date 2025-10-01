/*
Copyright(c) 2016, James Slepicka
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met :

1. Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and / or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED.IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
module;
#include <xmmintrin.h>
#include <immintrin.h>

export module dsp:biquad4_t_stereo;
import nemulator.std;
import :audio_filter;

namespace dsp
{
export template <float g_0, float g_1, float g_2, float g_3, float b2_0, float b2_1, float b2_2, float b2_3, float a2_0,
                 float a2_1, float a2_2, float a2_3, float a3_0, float a3_1, float a3_2, float a3_3>
class c_biquad4_t_stereo : public i_audio_filter
{
  public:
    c_biquad4_t_stereo()
    {
        d = _mm256_setzero_ps();
        z1 = _mm256_setzero_ps();
        z2 = _mm256_setzero_ps();
    }

    __forceinline std::array<float, 2> process(std::array<float, 2> input)
    {
        const __m256 g = _mm256_set_ps(g_3, g_2, g_1, g_0, g_3, g_2, g_1, g_0);
        const __m256 b2 = _mm256_set_ps(b2_3, b2_2, b2_1, b2_0, b2_3, b2_2, b2_1, b2_0);
        const __m256 a2 = _mm256_set_ps(a2_3, a2_2, a2_1, a2_0, a2_3, a2_2, a2_1, a2_0);
        const __m256 a3 = _mm256_set_ps(a3_3, a3_2, a3_1, a3_0, a3_3, a3_2, a3_1, a3_0);

        d.m256_f32[0] = input[0];
        d.m256_f32[4] = input[1];
        __m256 post_gain1 = _mm256_mul_ps(d, g);
        __m256 out = _mm256_add_ps(post_gain1, z1);
        __m256 t_z1_1 = _mm256_mul_ps(post_gain1, b2);
        __m256 t_z2 = _mm256_mul_ps(out, a3);
        t_z1_1 = _mm256_add_ps(t_z1_1, z2);
        __m256 t_z1_2 = _mm256_mul_ps(out, a2);
        d = _mm256_permute_ps(out, 0x93);
        z2 = _mm256_sub_ps(post_gain1, t_z2);
        z1 = _mm256_sub_ps(t_z1_1, t_z1_2);
        return {d.m256_f32[0], d.m256_f32[4]};
    }

  private:
    alignas(16) __m256 z1;
    alignas(16) __m256 z2;
    alignas(16) __m256 d;
};
} //namespace dsp
