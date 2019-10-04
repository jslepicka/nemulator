#pragma once

#include <fstream>
//#include "..\biquad8.h"
#include "..\resampler.h"

class c_nes;

class c_apu2
{
public:
	c_apu2(void);
	virtual ~c_apu2(void);
	void reset(void);
	void write_byte(unsigned short address, unsigned char value);
	unsigned char read_byte(unsigned short address);
	void clock(int cycles);
	void clock_once();
	void set_nes(c_nes *nes);
	void enable_mixer();
	void disable_mixer();
	void set_resampler(c_resampler *resampler);
	int get_sample_buf(const float **sample_buf);

	class c_envelope
	{
	public:
		c_envelope();
		~c_envelope();
		void clock();
		void write(unsigned char value);
		void reset_counter();
		void reset();
		int get_output();
	private:
		int period;
		int reset_flag;
		int output;
		int counter;
		int enabled;
		int loop;
		int env_vol;
		int reg_vol;
	};

	class c_timer
	{
	public:
		c_timer();
		~c_timer();
		//void clock(int shift);
		int clock();
		//int clock2x();
		void set_period_lo(int period_lo);
		void set_period_hi(int period_hi);
		void set_period(int value);
		int get_output();
		int get_period();
		void reset();
	private:
		int shift;
		int counter;
		int period;
		//int output;
	};

	class c_sequencer
	{
	public:
		c_sequencer();
		~c_sequencer();
		void clock();
		void set_duty_cycle(int cycle);
		int get_output();
		void reset_step();
		void reset();
	private:
		static const int duty_cycle_table[4][8];
		int duty_cycle;
		int step;
		int output;
		int ticks;
	};

	class c_length
	{
	public:
		c_length();
		~c_length();
		void clock();
		void set_length(int index);
		void set_halt(int halt);
		int get_counter();
		int get_output();
		void reset();
		int get_halt();
		void enable();
		void disable();
	private:
		int enabled;
		int counter;
		int halt;
		int length;
		static const int length_table[32];
	};

	class c_square
	{
	public:
		c_square();
		~c_square();
		void clock_envelope();
		void clock_length();
		void clock_length_sweep();
		void clock_timer();
		int get_output();
		int get_output_mmc5();
		int get_status();
		void enable();
		void disable();
		void write(unsigned short address, unsigned char value);
		void set_sweep_mode(int mode);
		void reset();
	private:
		int sweep_silencing;
		int sweep_mode;
		int sweep_reg;
		int sweep_reset;
		int sweep_period;
		int sweep_enable;
		int sweep_negate;
		int sweep_shift;
		int sweep_output;
		int sweep_counter;
		int output;
		void update_sweep_silencing();
		c_envelope envelope;
		c_timer timer;
		c_sequencer sequencer;
		c_length length;
	};

	class c_triangle
	{
	public:
		c_triangle();
		~c_triangle();
		void clock_timer();
		void clock_length();
		void clock_linear();
		void enable();
		void disable();
		int get_status();
		int get_output();
		void write(unsigned short address, unsigned char value);
		void reset();
	private:
		static const int sequence[32];
		int sequence_pos;
		int linear_control;
		int linear_halt;
		int output;
		int linear_reload;
		int linear_counter;
		int length_enabled;
		c_timer timer;
		c_length length;
	};

	class c_noise
	{
	public:
		c_noise();
		~c_noise();
		void clock_timer();
		void clock_length();
		void clock_envelope();
		void write(unsigned short address, unsigned char value);
		int get_output();
		void enable();
		void disable();
		int get_status();
		void reset();
	private:
		int lfsr;
		static const int random_period_table[16];
		int output;
		c_length length;
		c_envelope envelope;
		c_timer timer;
		int lfsr_shift;
		int random_period;
		int random_counter;
		int length_enabled;
	};

	class c_dmc
	{
	public:
		c_dmc();
		~c_dmc();
		int get_output();
		void enable();
		void disable();
		int get_status();
		void clock_timer();
		void write(unsigned short address, unsigned char value);
		void set_nes(c_nes *nes);
		void ack_irq();
		int get_irq_flag();
		void reset();
	private:
		c_nes *nes;
		static const int freq_table[16];
		void fill_sample_buffer();
		int irq_enable;
		int loop;
		int freq_index;
		int dac;
		int sample_address;
		int sample_length;

		int dma_counter;
		int dma_remain;

		int sample_buffer;
		int sample_buffer_empty;

		int cycle;

		int silence;

		int output_shift;
		int output_counter;
		int bits_remain;

		int duration;
		int address;

		int enabled;

		int irq_flag;
		int irq_asserted;

		c_timer timer;
	};
	static const float square_lut[31];
private:
	c_resampler *resampler;
	static const int CLOCKS_PER_FRAME_SEQ = 89489;
	static const int PRE_DECIMATE_M = 3;
	static const float tnd_lut[203];
	static const float fir511_cheb[512];
	//static float fir[4][512];
	//float fir3[3];
	int mixer_enabled;
	c_nes *nes;
	int frame_irq_enable;
	int frame_irq_flag;
	int frame_irq_asserted;
	int mix_external_audio;
	float *external_audio;

	

	int ticks;
	int frame_seq_counter;
	void clock_frame_seq();
	void clock_timers();

	void clock_envelopes();
	void clock_length_sweep();

	void mix();

	unsigned char reg[24];
	int frame_seq_step;
	int frame_seq_steps;

	c_square square1;
	c_square square2;
	c_square *squares[2];
	c_triangle triangle;
	c_noise noise;
	c_dmc dmc;

	int pre_decimate;
	int square_clock;
};

