#pragma once
#include "..\mapper.h"

class c_mapper85 :
	public c_mapper
{
public:
	c_mapper85();
	~c_mapper85() {};
	void WriteByte(unsigned short address, unsigned char value);
	void clock(int cycles);
	void reset();
	float mix_audio(float sample);
private:
	int ticks;
	int audio_ticks;
	int irq_counter;
	int irq_asserted;
	int irq_control;
	int irq_reload;
	int irq_scaler;
	int audio_register_select;

	unsigned char channel_regs[8][4];


	void audio_register_write(int audio_register, int value);
	void clock_audio();

	static const int instruments[16][8];
	int custom_instrument[8];
	int phase[12];
	int phase_secondary[12];
	int output[12];
	int egc[12];

	int sin_table[256];

	float audio_output;
	float last_audio_output;

	float linear2db(float in);
	float db2linear(float in);

	int clock_envelope(int slot, const int *inst);
	int envelope_state[12];

	enum ENVELOPE_STATES { ENV_IDLE = 0, ENV_ATTACK, ENV_DECAY, ENV_SUSTAIN, ENV_RELEASE };
	void key_on(int slot);
	void key_off(int slot);

	int db(float db);
	float ln(float a);

	int am_counter;
	int fm_counter;

	static const float pi;
};