#include "psg.h"
#include <math.h>
#include <stdio.h>

#include <crtdbg.h>
#if defined(DEBUG) | defined(_DEBUG)
#define DEBUG_NEW new(_CLIENT_BLOCK, __FILE__, __LINE__)
#define new DEBUG_NEW
#endif

const float c_psg::g[8] = {
	0.361560523509979f,
	0.222417041659355f,
	0.0714186578989029f,
	0.00309817981906235f,
};
const float c_psg::b2[8] = {
	-1.95306515693665f,
	-1.90443754196167f,
	-1.37055253982544f,
	-1.96420431137085f,
};
const float c_psg::a2[8] = {
	-1.95503556728363f,
	-1.93484497070313f,
	-1.92062354087830f,
	-1.97571694850922f,
};
const float c_psg::a3[8] = {
	0.965465605258942f,
	0.940786004066467f,
	0.922847032546997f,
	0.988919317722321f,
};


c_psg::c_psg()
{
	float db2 = powf(10.0f, -(2.0f / 20.f));
	for (int i = 0; i < 15; i++)
	{
		vol_table[i] = 1.0f * powf(db2, i);
	}
	vol_table[15] = 0.0f;
	int jdghkdjf = 1;
	sample_accumulator = 0.0f;
	resampler_count = 0;
	resampler = new c_resampler(((228.0*262.0*60.0) / 4.0) / 48000.0, g, b2, a2, a3);
}


c_psg::~c_psg()
{
	if (resampler)
		delete resampler;
}

int c_psg::get_buffer(const short **buffer)
{
	int num_samples = resampler->get_output_buf((const short**)buffer);
	//printf("-- %d\n", num_samples);
	return num_samples; //number of buffered samples
}

void c_psg::reset()
{
	sample_count = 0;
	tick = 0;
	for (int i = 0; i < 4; i++)
	{
		vol[i] = 0xF;
		tone[i] = 0;
		counter[i] = 0;
		output[i] = 0;
	}
	noise_shift = 0;
	noise_mode = 0;
	noise_register = 0;
	lfsr = 0x8000;
	lfsr_out = 0;
	channel = 0;
	type = 0;
}

void c_psg::set_audio_rate(double freq)
{
	double x = ((228.0*262.0*60.0) / 4.0) / freq;
	resampler->set_m(x);
	//printf("%f %f\n", freq, x);
}

void c_psg::clock(int cycles)
{
	int x = 0;
	while ((x = 16 - tick - cycles) <= 0)
	{
		if (tick > 0)
		{
			cycles -= tick;
			tick = 0;
		}
		else
			cycles -= 16;
		
		float out = 0.0f;
		//process 3 square channels
		for (int s = 0; s < 3; s++)
		{
			counter[s] = (counter[s] - 1) & 0x3FF;
			if (counter[s] == 0)
			{
				output[s] ^= 1;
				counter[s] = tone[s];
				//counter[s] <<= 2;
			}
			if (output[s] || tone[s] < 2)
				out += vol_table[vol[s]];

		}
		//noise
		counter[3] = (counter[3] - 1) & 0x3FF;
		if (counter[3] == 0)
		{
			output[3] ^= 1;
			switch (tone[3] & 0x3)
			{
			case 0:
				counter[3] = 0x10;
				break;
			case 1:
				counter[3] = 0x20;
				break;
			case 2:
				counter[3] = 0x40;
				break;
			case 3:
				counter[3] = tone[2];
				break;
			}
			//counter[3] <<= 2;
			if (output[3] == 1) //clock lfsr
			{
				if (tone[3] & 0x4)
				{
					int in = ((lfsr & 0x1) << 15) ^ ((lfsr & 0x8) << 12);
					lfsr_out = lfsr & 0x1;
					lfsr >>= 1;
					lfsr |= in;
				}
				else
				{
					lfsr_out = lfsr & 0x1;
					int in = lfsr_out << 15;
					lfsr >>= 1;
					lfsr |= in;
				}
			}
			//printf("%2X\n", lfsr);
		}
		if (lfsr & 0x1)
			out += vol_table[vol[3]];

		//resample(out / 4.0f);
		if (mixer_enabled)
		{
			for (int i = 0; i < 4; i++)
				resampler->process(out / 4.0f);
		}
	}
	tick = x;
}

void c_psg::write(int data)
{
	if (data & 0x80) // latch/data
	{
		channel = (data & 0x60) >> 5;
		type = data & 0x10;
		switch (type)
		{
		case TYPE_TONE:
			if (channel == 3)
			{
				tone[channel] = data & 0x7;
				lfsr = 0x8000;
			}
			else
				tone[channel] = (tone[channel] & 0xF0) | (data & 0x0F);
			break;
		case TYPE_VOLUME:
			vol[channel] = data & 0xF;
			break;
		}
	}
	else //data
	{
		switch (type)
		{
		case TYPE_TONE:
			if (channel == 3)
			{
				tone[channel] = data & 0x7;
				lfsr = 0x8000;
			}
			else
				tone[channel] = (tone[channel] & 0x0F) | ((data & 0x3F) << 4);
			break;
		case TYPE_VOLUME:
			vol[channel] = data & 0xF;
			break;
		}
	}
}
