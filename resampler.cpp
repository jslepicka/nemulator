#include "resampler.h"
#include <memory>

//c_resampler::c_resampler(float m, const float *g, const float *b2, const float *a2, const float *a3) :
//	blockdc(0.99804037809371948f, -1.0f, -0.99608069658279419f),
//	lowpass(0.56582623720169067f, 1.0f, 0.13165250420570374f)
c_resampler::c_resampler(float m, i_audio_filter* pre_filter, i_audio_filter* post_filter)
{
	this->m = m;
	mf = 0.0f;
	output_buf_index = 0;
	filtered_buf_index = FILTERED_BUF_LEN - 1;

	mf = m - (int)m;
	samples_required = (int)m + 2;
	//lpf = new c_biquad4(g, b2, a2, a3);

	output_buf = new short[OUTPUT_BUF_LEN];
	filtered_buf = new float[FILTERED_BUF_LEN * 2];

	memset(output_buf, 0, sizeof(output_buf));

	for (int i = 0; i < FILTERED_BUF_LEN * 2; i++)
		filtered_buf[i] = 0.0f;

	this->pre_filter = pre_filter;
	this->post_filter = post_filter;
}


c_resampler::~c_resampler()
{
	//delete lpf;
	delete[] output_buf;
	delete[] filtered_buf;
}

void c_resampler::set_m(float m)
{
	this->m = m;
}

int c_resampler::get_output_buf(const short **sample_buf)
{
	*sample_buf = this->output_buf;
	return output_buf_index;
}

void c_resampler::clear_buf()
{
	output_buf_index = 0;
}