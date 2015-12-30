#pragma once
#include "..\resampler.h"
class c_psg
{
public:
	c_psg();
	~c_psg();
	int get_buffer(const short **buffer);
	void clock(int cycles);
	void write(int data);
	void reset();
	void set_audio_rate(double freq);
	void enable_mixer() { mixer_enabled = 1; }
	void disable_mixer() { mixer_enabled = 0; };
private:
	int mixer_enabled = 0;
	static const float g[8];
	static const float b2[8];
	static const float a2[8];
	static const float a3[8];
	c_resampler *resampler;
	int tick;
	int sample_count;
	short *sample_buffer;
	float vol_table[16];
	int vol[4];
	int tone[4];
	int counter[4];
	int output[4];
	int channel;
	int type;
	float sample_accumulator;
	int resampler_count;
	int noise_shift;
	int noise_mode;
	int noise_register;
	unsigned short lfsr;
	int lfsr_out;
	enum
	{
		TYPE_TONE = 0,
		TYPE_VOLUME = 0x10
	};
};

