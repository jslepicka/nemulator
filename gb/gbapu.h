#pragma once
#include <cstdint>
#include "..\resampler.h"
#include "..\biquad.hpp"
#include "..\biquad4.hpp"

class c_gb;
class c_gbapu
{
public:
	c_gbapu(c_gb* gb);
	~c_gbapu();
	void reset();
	void clock();
	void write_byte(uint16_t address, uint8_t data);
	uint8_t read_byte(uint16_t data);
	void set_resampler(c_resampler* resampler);
	void mix();
	void enable_mixer();
	void disable_mixer();
	int get_buffer(const short** buffer);
private:
	c_gb* gb;
	c_resampler* resampler;
	c_biquad4* lpf;
	c_biquad* post_filter;
	int mixer_enabled;
	int ticks;
	int frame_seq_counter;
	int frame_seq_step;
	static const int CLOCKS_PER_FRAME_SEQ = 8192/2;
	uint8_t NR50;
	uint8_t NR51;
	uint8_t NR52;
	int left_vol;
	int right_vol;

	int enable_n_l;
	int enable_n_r;
	int enable_w_l;
	int enable_w_r;
	int enable_2_l;
	int enable_2_r;
	int enable_1_l;
	int enable_1_r;


	void power_on();
	void power_off();

	class c_timer
	{
	public:
		c_timer();
		~c_timer();
		void reset();
		void set_period(int period);
		int clock();
	private:
		int counter;
		int period;
		int mask;
	};

	class c_duty
	{
	public:
		c_duty();
		~c_duty();
		int get_output();
		void set_duty_cycle(int duty);
		void clock();
		void reset();
		void reset_step();
	private:
		static const int duty_cycle_table[4][8];
		int duty_cycle;
		int step;
	};

	class c_length
	{
	public:
		c_length();
		~c_length();
		int get_output();
		void set_length(int length);
		void set_enable(int enable);
		void reset();
		int clock();
		int counter;
	private:
		int enabled;
	};

	class c_envelope
	{
	public:
		c_envelope();
		~c_envelope();
		void reset();
		int get_output();
		void set_volume(int volume);
		void set_period(int period);
		void set_mode(int mode);
		void clock();
	private:
		int volume;
		int counter;
		int period;
		int output;
		int mode;

	};

	class c_square
	{
	public:
		c_square(int has_sweep);
		~c_square();
		void clock_timer();
		void clock_length();
		void clock_sweep();
		void clock_envelope();
		int get_output();
		void write(int reg, uint8_t data);
		void reset();
		void power_on();
		void power_off();
		void trigger();
	private:
		c_timer timer;
		c_duty duty;
		c_length length;
		c_envelope envelope;
		int has_sweep;
		int sweep_period;
		int sweep_negate;
		int sweep_shift;
		int sweep_shadow;
		int sweep_counter;
		int sweep_enabled;
		int sweep_freq;
		int _duty;
		int length_load;
		int starting_volume;
		int evelope_add_mode;
		int period;
		int frequency;
		int length_enable;
		int period_hi;
		int period_lo;
		int enabled;
		int envelope_period;
		int calc_sweep();
		int dac_power;
	};

	class c_noise
	{
	public:
		c_noise();
		~c_noise();
		void reset();
		void clock();
		int get_output();
		void trigger();
		void write(uint16_t address, uint8_t data);
		void clock_timer();
		void clock_length();
		void clock_envelope();
		void power_on();
	private:
		int clock_shift;
		int width_mode;
		int divisor_code;
		int lfsr;
		int starting_volume;
		int envelope_period;
		c_timer timer;
		c_length length;
		c_envelope envelope;
		int enabled;
		static const int divisor_table[8];
		int dac_power;
	};

	class c_wave
	{
	public:
		c_wave();
		~c_wave();
		void reset();
		void clock();
		int get_output();
		void trigger();
		void write(uint16_t address, uint8_t data);
		void clock_timer();
		void clock_length();
		void clock_envelope();
		void power_on();
	private:

		int starting_volume;
		int envelope_period;
		c_timer timer;
		c_length length;
		int enabled;
		int timer_period;
		int sample_buffer;
		int wave_table[32];
		int wave_pos;
		int volume_shift;
		int dac_power;
		int period_hi;
		int period_lo;
	};

	c_square square1;
	c_square square2;
	c_noise noise;
	c_wave wave;
};

