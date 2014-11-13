#pragma once
#include <xmmintrin.h>

class c_biquad8
{
public:
	c_biquad8(float *g, float *b2, float *a2, float *a3);
	~c_biquad8(void);
	float process_df2t(float input);
private:
	__m128 *z11, *z12, *z21, *z22;
	__m128 *d1, *d2;
	__m128 *g1, *g2;
	__m128 *a12, *a22;
	__m128 *b12, *b22;
	__m128 *a13, *a23;
};

inline c_biquad8::c_biquad8(float *g, float *b2, float *a2, float *a3)
{
	d1 = (__m128*)_aligned_malloc(sizeof(__m128), 16);
	d2 = (__m128*)_aligned_malloc(sizeof(__m128), 16);
	g1 = (__m128*)_aligned_malloc(sizeof(__m128), 16);
	g2 = (__m128*)_aligned_malloc(sizeof(__m128), 16);
	z11 = (__m128*)_aligned_malloc(sizeof(__m128), 16);
	z12 = (__m128*)_aligned_malloc(sizeof(__m128), 16);
	z21 = (__m128*)_aligned_malloc(sizeof(__m128), 16);
	z22 = (__m128*)_aligned_malloc(sizeof(__m128), 16);
	a12 = (__m128*)_aligned_malloc(sizeof(__m128), 16);
	a22 = (__m128*)_aligned_malloc(sizeof(__m128), 16);
	b12 = (__m128*)_aligned_malloc(sizeof(__m128), 16);
	b22 = (__m128*)_aligned_malloc(sizeof(__m128), 16);
	a13 = (__m128*)_aligned_malloc(sizeof(__m128), 16);
	a23 = (__m128*)_aligned_malloc(sizeof(__m128), 16);

	*d1 = _mm_setzero_ps();
	*d2 = _mm_setzero_ps();
	*z11 = _mm_setzero_ps();
	*z12 = _mm_setzero_ps();
	*z21 = _mm_setzero_ps();
	*z22 = _mm_setzero_ps();

	*g1 = _mm_setr_ps(g[3], g[2], g[1], g[0]);
	*g2 = _mm_setr_ps(g[7], g[6], g[5], g[4]);
	*b12 = _mm_setr_ps(b2[3], b2[2], b2[1], b2[0]);
	*b22 = _mm_setr_ps(b2[7], b2[6], b2[5], b2[4]);
	*a12 = _mm_setr_ps(a2[3], a2[2], a2[1], a2[0]);
	*a22 = _mm_setr_ps(a2[7], a2[6], a2[5], a2[4]);
	*a13 = _mm_setr_ps(a3[3], a3[2], a3[1], a3[0]);
	*a23 = _mm_setr_ps(a3[7], a3[6], a3[5], a3[4]);
}

inline c_biquad8::~c_biquad8(void)
{
	_aligned_free(z11);
	_aligned_free(z12);
	_aligned_free(z21);
	_aligned_free(z22);
	_aligned_free(d1);
	_aligned_free(d2);
	_aligned_free(g1);
	_aligned_free(g2);
	_aligned_free(a12);
	_aligned_free(a22);
	_aligned_free(b12);
	_aligned_free(b22);
	_aligned_free(a13);
	_aligned_free(a23);
}

__forceinline float c_biquad8::process_df2t(float input)
{
	d1->m128_f32[3] = input;
	__m128 post_gain2 = _mm_mul_ps(*d2, *g2);
	__m128 post_gain1 = _mm_mul_ps(*d1, *g1);
	__m128 out2 = _mm_add_ps(post_gain2, *z12);
	__m128 out1 = _mm_add_ps(post_gain1, *z11);
	__m128 t2 = _mm_mul_ps(out2, *a22);
	__m128 t1 = _mm_mul_ps(out1, *a12);

	__m128 t_z11 = _mm_mul_ps(post_gain1, *b12);
	t_z11 = _mm_sub_ps(t_z11, t1);
	*z11 = _mm_add_ps(t_z11, *z21);

	__m128 t_z12 = _mm_mul_ps(post_gain2, *b22);
	t_z12 = _mm_sub_ps(t_z12, t2);
	*z12 = _mm_add_ps(t_z12, *z22);

	__m128 t3 = _mm_mul_ps(out1, *a13);
	__m128 t4 = _mm_mul_ps(out2, *a23);

	*z21 = _mm_sub_ps(post_gain1, t3);
	*z22 = _mm_sub_ps(post_gain2, t4);

	*d1 = _mm_shuffle_ps(out1, out1, 0x39);
	*d2 = _mm_shuffle_ps(out2, out2, 0x39);
	d2->m128_f32[3] = out1.m128_f32[0];
	float out = out2.m128_f32[0];
	return out;
}