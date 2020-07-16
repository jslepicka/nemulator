#include "psg.h"
#include <math.h>
#include <stdio.h>

#include <crtdbg.h>
//#if defined(DEBUG) | defined(_DEBUG)
//#define DEBUG_NEW new(_CLIENT_BLOCK, __FILE__, __LINE__)
//#define new DEBUG_NEW
//#endif

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
	/*
	lowpass 20kHz
	d = fdesign.lowpass('N,Fp,Ap,Ast', 8, 20000, .1, 80, 896040);
	Hd = design(d, 'ellip', 'FilterStructure', 'df2tsos');
	set(Hd, 'Arithmetic', 'single');
	g = regexprep(num2str(reshape(Hd.ScaleValues(1:4), [1 4]), '%.16ff '), '\s+', ',')
	b2 = regexprep(num2str(Hd.sosMatrix(5:8), '%.16ff '), '\s+', ',')
	a2 = regexprep(num2str(Hd.sosMatrix(17:20), '%.16ff '), '\s+', ',')
	a3 = regexprep(num2str(Hd.sosMatrix(21:24), '%.16ff '), '\s+', ',')
	*/
	lpf = new c_biquad4(
		{ 0.5068508386611939f,0.3307863473892212f,0.1168005615472794f,0.0055816280655563f },
		{ -1.9496889114379883f,-1.9021773338317871f,-1.3770858049392700f,-1.9604763984680176f },
		{ -1.9442052841186523f,-1.9171522855758667f,-1.8950747251510620f,-1.9676681756973267f },
		{ 0.9609073400497437f,0.9271715879440308f,0.8989855647087097f,0.9881398081779480f }
	);
	post_filter = new c_biquad(
		0.5648277401924133f,
		{ 1.0000000000000000f,0.0000000000000000f,-1.0000000000000000f },
		{ 1.0000000000000000f,-0.8659016489982605f,-0.1296554803848267f }
	);
	resampler = new c_resampler(((228.0 * 262.0 * 60.0) / 4.0) / 48000.0, lpf, post_filter);
	sound_buffer = new int32_t[1024];
}


c_psg::~c_psg()
{
	if (resampler)
		delete resampler;
	if (lpf)
		delete lpf;
	if (post_filter)
		delete post_filter;
	if (sound_buffer)
		delete[] sound_buffer;
}

int c_psg::get_buffer(const int32_t **buffer)
{
	const short* source;
	short* stereo_dest = (short*)sound_buffer;
	int num_samples = resampler->get_output_buf(&source);
	for (int i = 0; i < num_samples; i++) {
		*stereo_dest++ = *source;
		*stereo_dest++ = *source++;
	}
	*buffer = sound_buffer;
	return num_samples;
}

void c_psg::reset()
{
	available_cycles = 0;
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
	memset(sound_buffer, 0, sizeof(int32_t) * 1024);
}

void c_psg::set_audio_rate(double freq)
{
	double x = ((228.0*262.0*60.0) / 4.0) / freq;
	resampler->set_m(x);
	//printf("%f %f\n", freq, x);
}

void c_psg::clock(int cycles)
{
	available_cycles += cycles;
	while (available_cycles >= 16)
	{
		available_cycles -= 16;
		
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
