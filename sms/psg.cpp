#include "psg.h"
#include <math.h>
#include <stdio.h>

#include <crtdbg.h>
#if defined(DEBUG) | defined(_DEBUG)
#define DEBUG_NEW new(_CLIENT_BLOCK, __FILE__, __LINE__)
#define new DEBUG_NEW
#endif

const float c_psg::g[8] = {
	0.699921727180481,
	0.610556006431580,
	0.428973615169525,
	0.212722122669220,
	0.0619322322309017,
	0.00240012421272695,
	0.00301445997320116,
	0.00284892972558737,

};
const float c_psg::b2[8] = {
	-1.96975886821747,
	-1.96356105804443,
	-1.94670689105988,
	-1.88419520854950,
	-1.23711526393890,
	-1.97199189662933,
	2.0,
	2.0

};
const float c_psg::a2[8] = {
	-1.96834802627563,
	-1.95860993862152,
	-1.94580507278442,
	-1.93146324157715,
	-1.92113804817200,
	-1.97612035274506,
	-1.90577268600464,
	-1.80112266540527,

};
const float c_psg::a3[8] = {
	0.986887753009796,
	0.974294543266296,
	0.957059919834137,
	0.937478542327881,
	0.923312544822693,
	0.996024370193481,
	0.917830467224121,
	0.812518358230591,
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
	int num_samples = resampler->get_sample_buf((const short**)buffer);
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
	for (int i = 0; i < cycles; i++)
	{
		if (++tick == 16)
		{
			tick = 0;
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
	}
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
