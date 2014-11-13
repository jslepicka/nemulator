#pragma once
#include "biquad8.hpp"
#include <cmath>
#include <stdlib.h>

class c_resampler
{
public:
	c_resampler(float m);
	virtual ~c_resampler();
	void set_m(float m);
	int get_sample_buf(const short **sample_buf);
	void process(float sample);
private:
	float input_rate;
	float output_rate;
	float m, mf;
	int samples_required;
	c_biquad8 *lpf;
	static float g[8];
	static float b2[8];
	static float a2[8];
	static float a3[8];

	float blockdc(float sample);
	float last_sample = 0.0f;
	float last_out = 0.0f;

	static const int SAMPLE_BUF_LEN = 1024;
	static const int FILTERED_BUF_LEN = 4;
	int sample_buf_index;
	int filtered_buf_index;
	short *sample_buf;
	float *filtered_buf;
};

//1st order IIR highpass
//a1 = exp(-2*pi*(Fc/Fs))
__forceinline float c_resampler::blockdc(float sample)
{
	static const float a = 0.99608f; //Fc=~30Hz
	float output = sample - last_sample + a * last_out;
	last_sample = sample;
	last_out = output;
	return output;
}

__forceinline void c_resampler::process(float sample)
{
	filtered_buf[filtered_buf_index] = filtered_buf[filtered_buf_index + FILTERED_BUF_LEN] = lpf->process_df2t(sample);
	if (--samples_required == 0)
	{
		float y2 = filtered_buf[filtered_buf_index];
		float y1 = filtered_buf[filtered_buf_index + 1];
		float y0 = filtered_buf[filtered_buf_index + 2];
		float ym = filtered_buf[filtered_buf_index + 3];

		//4-point, 3rd-order B-spline (x-form)
		//see deip.pdf
		float ym1py1 = ym + y1;
		float c0 = 1.0f / 6.0f*ym1py1 + 2.0f / 3.0f*y0;
		float c1 = 1.0f / 2.0f*(y1 - ym);
		float c2 = 1.0f / 2.0f*ym1py1 - y0;
		float c3 = 1.0f / 2.0f*(y0 - y1) + 1.0f / 6.0f*(y2 - ym);
		float j = ((c3*mf + c2)*mf + c1)*mf + c0;

		float extra = 2.0f - mf;
		float n = m - extra;
		mf = n - (int)n;
		samples_required = (int)n + 2;
		static const float max_out = 32767.0f;
		//int s = (int)(round(blockdc(j) * max_out));
#if 1
		int s = (int)round(blockdc(j) * max_out);
		
#else
		float dither = (rand() / ((float)RAND_MAX * .5f) - 1.0f) / (1 << 16);
		int s = (int)round(blockdc(j) * max_out + dither);
#endif
		
		if (s > 32767)
			s = 32767;
		else if (s < -32768)
			s = -32768;
		sample_buf[sample_buf_index++] = (short)s;
	}
	filtered_buf_index = (filtered_buf_index - 1) & 0x3;
}
