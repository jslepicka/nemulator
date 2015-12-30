#pragma once
#include <xmmintrin.h>
//#define AVX 1
#ifdef AVX
#include <immintrin.h>
#endif


class c_biquad8
{
public:
	c_biquad8(const float *g, const float *b2, const float *a2, const float *a3);
	~c_biquad8(void);
	float process_df2t(float input);
#ifdef AVX
	float process_df2t_avx(float input);
#endif
private:
	__m128 *z11, *z12, *z21, *z22;
	__m128 *d1, *d2;
	__m128 *g1, *g2;
	__m128 *a12, *a22;
	__m128 *b12, *b22;
	__m128 *a13, *a23;

#ifdef AVX
	__m256 *z1, *z2;
	__m256 *d;
	__m256 *g;
	__m256 *a2;
	__m256 *a3;
	__m256 *b2;
#endif
};

inline c_biquad8::c_biquad8(const float *g, const float *b2, const float *a2, const float *a3)
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

#ifdef AVX
	z1 = (__m256*)_aligned_malloc(sizeof(__m256), 32);
	z2 = (__m256*)_aligned_malloc(sizeof(__m256), 32);
	d = (__m256*)_aligned_malloc(sizeof(__m256), 32);
	this->g = (__m256*)_aligned_malloc(sizeof(__m256), 32);
	this->a2 = (__m256*)_aligned_malloc(sizeof(__m256), 32);
	this->a3 = (__m256*)_aligned_malloc(sizeof(__m256), 32);
	this->b2 = (__m256*)_aligned_malloc(sizeof(__m256), 32);

	*z1 = _mm256_setzero_ps();
	*z2 = _mm256_setzero_ps();
	*d = _mm256_setzero_ps();

	*this->g = _mm256_setr_ps(g[7], g[6], g[5], g[4], g[3], g[2], g[1], g[0]);
	*this->a2 = _mm256_setr_ps(a2[7], a2[6], a2[5], a2[4], a2[3], a2[2], a2[1], a2[0]);
	*this->a3 = _mm256_setr_ps(a3[7], a3[6], a3[5], a3[4], a3[3], a3[2], a3[1], a3[0]);
	*this->b2 = _mm256_setr_ps(b2[7], b2[6], b2[5], b2[4], b2[3], b2[2], b2[1], b2[0]);
#endif

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

#ifdef AVX
__forceinline float c_biquad8::process_df2t_avx(float input)
{
	_mm256_zeroall();
	d->m256_f32[7] = input;
	__m256 post_gain = _mm256_mul_ps(*d, *g);
	__m256 out = _mm256_add_ps(post_gain, *z1);
	__m256 temp1 = _mm256_mul_ps(post_gain, *b2);
	__m256 temp2 = _mm256_mul_ps(out, *a2);
	temp1 = _mm256_add_ps(temp1, *z2);
	*z1 = _mm256_sub_ps(temp1, temp2);
	__m256 temp3 = _mm256_mul_ps(*a3, out);
	*z2 = _mm256_sub_ps(post_gain, temp3);

	//Step 1:
	//__m256 p0 = _mm256_permute_ps(a, 0x39) //00111001=0x39
	//	//p0 = a4, a7, a6, a5, a0, a3, a2, a1
	//	Step 2 :

	//	__m256 p1 = _mm256_permute2f128_ps(p0, p0, 0x81) //10000001=0x81
	//	//p1 = 0, 0, 0, 0, a4, a7, a6, a5
	//	Step 3 :
	//	p2 = _mm256_blend_ps(p0, p1, 0x88) //10001000=0x88
	//	//p2 = 0, a7, a6, a5, a4, a3, a2, a1

	__m256 p0 = _mm256_permute_ps(out, 0x39);
	__m256 p1 = _mm256_permute2f128_ps(p0, p0, 0x81);
	*d = _mm256_blend_ps(p0, p1, 0x88);
	float ret = out.m256_f32[0];
	_mm256_zeroall();
	return ret;
}
#endif
