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
#include <array>
#include <span>
#include <xmmintrin.h>

export module dsp:biquad4;
import :audio_filter;

namespace dsp
{
export class c_biquad4 : public i_audio_filter
{
  public:
    c_biquad4(const std::array<float, 4> &g, const std::array<float, 4> &b2, const std::array<float, 4> &a2, const std::array<float, 4> &a3)
    {
        d = _mm_setzero_ps();
        z1 = _mm_setzero_ps();
        z2 = _mm_setzero_ps();

        this->g = _mm_loadu_ps(&g[0]);
        this->b2 = _mm_loadu_ps(&b2[0]);
        this->a2 = _mm_loadu_ps(&a2[0]);
        this->a3 = _mm_loadu_ps(&a3[0]);
    }
    ~c_biquad4(void){};
    __forceinline float process(float input)
    {
        d.m128_f32[0] = input;
        __m128 post_gain1 = _mm_mul_ps(d, g);
        __m128 out = _mm_add_ps(post_gain1, z1);
        __m128 t_z1_1 = _mm_mul_ps(post_gain1, b2);
        __m128 t_z2 = _mm_mul_ps(out, a3);
        t_z1_1 = _mm_add_ps(t_z1_1, z2);
        __m128 t_z1_2 = _mm_mul_ps(out, a2);
        d = _mm_shuffle_ps(out, out, 0x93);
        z2 = _mm_sub_ps(post_gain1, t_z2);
        z1 = _mm_sub_ps(t_z1_1, t_z1_2);
        return _mm_cvtss_f32(d);
    }

  private:
    alignas(16) __m128 z1;
    alignas(16) __m128 z2;
    alignas(16) __m128 d;
    alignas(16) __m128 g;
    alignas(16) __m128 a2;
    alignas(16) __m128 b2;
    alignas(16) __m128 a3;
};
} //namespace dsp
