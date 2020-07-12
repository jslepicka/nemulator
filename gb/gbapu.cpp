#include "gbapu.h"
#include "gb.h"

extern HWND hWnd;

c_gbapu::c_gbapu(c_gb* gb) :
	square1(1),
	square2(0)
{
	this->gb = gb;
	mixer_enabled = 1;
}

c_gbapu::~c_gbapu()
{
	if (resampler)
		delete resampler;
	if (post_filter)
		delete post_filter;
	if (lpf)
		delete lpf;
}

void c_gbapu::reset()
{
	frame_seq_counter = CLOCKS_PER_FRAME_SEQ;
	frame_seq_step = 0;
	NR50 = 0;
	NR51 = 0;
	NR52 = 0;

	lpf = new c_biquad4(
		{ 0.5086284279823303f,0.3313708603382111f,0.1059221103787422f,0.0055782101117074f },
		{ -1.9872593879699707f,-1.9750031232833862f,-1.8231037855148315f,-1.9900115728378296f },
		{ -1.9759204387664795f,-1.9602127075195313f,-1.9470522403717041f,-1.9888486862182617f },
		{ 0.9801648259162903f,0.9627774357795715f,0.9480593800544739f,0.9940192103385925f }
	);

	post_filter = new c_biquad(
		0.4990182518959045f,
		{ 1.0000000000000000f,0.0000000000000000f,-1.0000000000000000f },
		{ 1.0000000000000000f,-0.9980365037918091f,0.0019634978380054f }
	);

	resampler = new c_resampler((456.0 * 154.0 * 60.0) / 48000.0, lpf, post_filter);
}

void c_gbapu::enable_mixer()
{
	mixer_enabled = 1;
}

void c_gbapu::disable_mixer()
{
	mixer_enabled = 0;
}

int c_gbapu::get_buffer(const short** buffer)
{
	int num_samples = resampler->get_output_buf((const short**)buffer);
	return num_samples;
}

void c_gbapu::write_byte(uint16_t address, uint8_t data)
{
	int x = 1;
	if (!(NR52 & 0x80) && address != 0xFF26) {
		//NR52 controls power to the sound hardware. When powered off, all registers (NR10-NR51) are instantly written with zero
		//and any writes to those registers are ignored while power remains off (except on the DMG, where length counters are unaffected
		//by power and can still be written while off)
		return;
	}
	switch (address) {
	case 0xFF10:
	case 0xFF11:
	case 0xFF12:
	case 0xFF13:
	case 0xFF14:
		//square 1
		square1.write(address - 0xFF10, data);
		break;

	//case 0xFF15 - this is unused on square 2
	case 0xFF16:
	case 0xFF17:
	case 0xFF18:
	case 0xFF19:
		//square 2
		square2.write(address - 0xFF15, data);
		break;

	case 0xFF1A:
	case 0xFF1B:
	case 0xFF1C:
	case 0xFF1D:
	case 0xFF1E:
	case 0xFF30:
	case 0xFF31:
	case 0xFF32:
	case 0xFF33:
	case 0xFF34:
	case 0xFF35:
	case 0xFF36:
	case 0xFF37:
	case 0xFF38:
	case 0xFF39:
	case 0xFF3A:
	case 0xFF3B:
	case 0xFF3C:
	case 0xFF3D:
	case 0xFF3E:
	case 0xFF3F:
		//wave
		wave.write(address, data);
		break;

	case 0xFF20:
	case 0xFF21:
	case 0xFF22:
	case 0xFF23:
		//noise
		noise.write(address - 0xFF20, data);
		break;
	/*
	NR50 FF24 ALLL BRRR Vin L enable, Left vol, Vin R enable, Right vol
	NR51 FF25 NW21 NW21 Left enables, Right enables
	NR52 FF26 P--- NW21 Power control/status, Channel length statuses
	*/
	case 0xFF24:
		left_vol = (data >> 4) & 0x7;
		right_vol = data & 0x7;
		NR50 = data;
		break;
	case 0xFF25:
		enable_1_l = data & 0x1 ? 1 : 0;
		enable_2_l = data & 0x2 ? 1 : 0;
		enable_w_l = data & 0x4 ? 1 : 0;
		enable_n_l = data & 0x8 ? 1 : 0;
		
		enable_1_r = data & 0x10 ? 1 : 0;
		enable_2_r = data & 0x20 ? 1 : 0;
		enable_w_r = data & 0x40 ? 1 : 0;
		enable_n_r = data & 0x80 ? 1 : 0;
		NR51 = data;
		break;
	case 0xFF26:
		if ((NR52 ^ data) & 0x80) { //power state change
			if (data & 0x80) {
				power_on();
			}
			else {
				power_off();
			}
		}
		NR52 = data;
		break;



	}
}

uint8_t c_gbapu::read_byte(uint16_t address)
{
	switch (address) {
	case 0xFF10:
	case 0xFF11:
	case 0xFF12:
	case 0xFF13:
	case 0xFF14:
		//square 1
		break;

	case 0xFF16:
	case 0xFF17:
	case 0xFF18:
	case 0xFF19:
		//square 2
		break;

	case 0xFF1A:
	case 0xFF1B:
	case 0xFF1C:
	case 0xFF1D:
	case 0xFF1E:
	case 0xFF30:
	case 0xFF31:
	case 0xFF32:
	case 0xFF33:
	case 0xFF34:
	case 0xFF35:
	case 0xFF36:
	case 0xFF37:
	case 0xFF38:
	case 0xFF39:
	case 0xFF3A:
	case 0xFF3B:
	case 0xFF3C:
	case 0xFF3D:
	case 0xFF3E:
	case 0xFF3F:
		//wave
		break;

	case 0xFF20:
	case 0xFF21:
	case 0xFF22:
	case 0xFF23:
		//noise
		break;

	case 0xFF24:
		return NR50;
		break;
	case 0xFF25:
		return NR51;
		break;
	case 0xFF26:
		//need to return channel length statuses
		return NR52;
		break;

	}
	return 0;
}


void c_gbapu::power_on()
{
	frame_seq_step = 0;
	frame_seq_counter = CLOCKS_PER_FRAME_SEQ;
	square1.power_on();
	square2.power_on();
	noise.power_on();
	wave.power_on();
}

void c_gbapu::power_off()
{
	for (int i = 0xFF10; i <= 0xFF25; i++) {
		write_byte(i, 0);
	}
}

//this is called every 4 cycles, so ~1MHz
void c_gbapu::clock(int cycles)
{
	//this will always get called with 4 cycles, so can be optimized
	for (; cycles > 0; cycles--) {
		int x = 1;
		//clock timers
		square1.clock_timer();
		square2.clock_timer();
		noise.clock_timer();
		wave.clock_timer();
		//every 8192 cycles, clock frame sequencer
		frame_seq_counter--;
		if (frame_seq_counter <= 0) {
			frame_seq_counter = CLOCKS_PER_FRAME_SEQ;
			switch (frame_seq_step & 0x7) {
			case 2:
			case 6:
				//clock sweep
				square1.clock_sweep();
			case 0:
			case 4:
				//length
				square1.clock_length();
				square2.clock_length();
				noise.clock_length();
				wave.clock_length();
				break;
			case 1:
			case 3:
			case 5:
				//nothing
				break;
			case 7:
				//vol env
				square1.clock_envelope();
				square2.clock_envelope();
				noise.clock_envelope();
				break;
			}
			frame_seq_step++;
		}
		if (mixer_enabled) {
			mix();
		}
	}
}

void c_gbapu::mix()
{
	auto dac = [](auto a) { return (a / 15.0) * 2.0 - 1.0; };
	float left_sample = 0.0f;
	float right_sample = 0.0f;
	//float square1_out = dac(square1.get_output());
	//float square2_out = dac(square2.get_output());
	//float wave_out = dac(wave.get_output());
	//float noise_out = dac(noise.get_output());

	float square1_out = square1.get_output();
	float square2_out = square2.get_output();
	float wave_out = wave.get_output();
	float noise_out = noise.get_output();

	left_sample += square1_out * enable_1_l +
		square2_out * enable_2_l +
		wave_out * enable_w_l +
		noise_out * enable_n_l;

	right_sample += square1_out * enable_1_r +
		square2_out * enable_2_r +
		wave_out * enable_w_r +
		noise_out * enable_n_r;

	left_sample = (left_sample / 60.0) * 2.0 - 1.0;
	right_sample = (right_sample / 60.0) * 2.0 - 1.0;
	
	
	right_sample /= 16.0f;
	left_sample /= 16.0f;

	right_sample *= (right_vol + 1);
	left_sample *= (left_vol + 1);

	resampler->process((right_sample + left_sample) / 2.0f);

}


c_gbapu::c_timer::c_timer()
{
	reset();
}

c_gbapu::c_timer::~c_timer()
{

}

void c_gbapu::c_timer::reset()
{
	counter = 1;
	period = 64;
}

int c_gbapu::c_timer::clock()
{
	int prev = counter;
	counter--;
	if (prev && counter == 0)
	{
		counter = period;
		return 1;
	}
	//counter &= 0x1FFF;
	return 0;
}

void c_gbapu::c_timer::set_period(int period)
{
	this->period = period;
}

c_gbapu::c_duty::c_duty()
{
	reset();
}

c_gbapu::c_duty::~c_duty()
{

}

void c_gbapu::c_duty::reset()
{
	duty_cycle = 0;
	step = 0;
}

void c_gbapu::c_duty::set_duty_cycle(int duty)
{
	duty_cycle = duty & 0x3;
}

void c_gbapu::c_duty::clock()
{
	step = (step + 1) & 0x7;
}

const int c_gbapu::c_duty::duty_cycle_table[4][8] = {
	{0, 0, 0, 0, 0, 0, 0, 1},
	{1, 0, 0, 0, 0, 0, 0, 1},
	{1, 0, 0, 0, 0, 1, 1, 1},
	{0, 1, 1, 1, 1, 1, 1, 0}
};

int c_gbapu::c_duty::get_output()
{
	return duty_cycle_table[duty_cycle][step];
}

void c_gbapu::c_duty::reset_step()
{
	step = 0;
}


c_gbapu::c_length::c_length()
{
	reset();
}

c_gbapu::c_length::~c_length()
{

}

void c_gbapu::c_length::reset()
{
	counter = 0;
	enabled = 0;
}

int c_gbapu::c_length::clock()
{
	if (enabled && counter != 0) {
		counter--;
		if (counter == 0) {
			enabled = 0;
		}
		return counter;
	}
	return 1;
}

int c_gbapu::c_length::get_output()
{
	return counter;
}

void c_gbapu::c_length::set_enable(int enable)
{
	enabled = enable;
}

void c_gbapu::c_length::set_length(int length)
{
	counter = length;
}

c_gbapu::c_envelope::c_envelope()
{
	reset();
}

c_gbapu::c_envelope::~c_envelope()
{

}

void c_gbapu::c_envelope::reset()
{
	volume = 0;
	counter = 0;
	period = 0;
	output = 0;
	mode = 0;
}

int c_gbapu::c_envelope::get_output()
{
	return output;
}

void c_gbapu::c_envelope::set_volume(int volume)
{
	this->volume = volume;
	output = volume;
}

void c_gbapu::c_envelope::set_period(int period)
{
	this->period = period;
	counter = period;
}

void c_gbapu::c_envelope::set_mode(int mode)
{
	this->mode = mode;
}

void c_gbapu::c_envelope::clock()
{
	if (period != 0) {
		int prev = counter;
		counter--;
		if (prev && counter == 0) {
			counter = period;
			int new_vol = output + (mode ? 1 : -1);
			if (new_vol >= 0 && new_vol < 16) {
				output = new_vol;
			}
		}
		counter &= 0x7;
	}
}

c_gbapu::c_square::c_square(int has_sweep)
{
	this->has_sweep = has_sweep;
	reset();
}

c_gbapu::c_square::~c_square()
{

}

void c_gbapu::c_square::clock_timer()
{
	if (timer.clock())
		duty.clock();
}

void c_gbapu::c_square::clock_length()
{
	if (length.clock() == 0) {
		enabled = 0;
	}
}

void c_gbapu::c_square::clock_envelope()
{
	envelope.clock();
}

void c_gbapu::c_square::reset()
{
	enabled = 0;
	sweep_period = 0;
	sweep_negate = 1;
	sweep_shift = 0;
	_duty = 0;
	length_load = 0;
	starting_volume = 0;
	evelope_add_mode = 0;
	period = 0;
	frequency = 0;
	length_enable = 0;
	timer.reset();
	duty.reset();
	length.reset();
	length.counter = 64;
	period_hi = 0;
	period_lo = 0;
	timer.set_period(2048 * 4);
	envelope_period = 0;
	sweep_period = 0;
	sweep_counter = 0;
	sweep_shadow = 0;
	sweep_enabled = 0;
	dac_power = 0;
}

void c_gbapu::c_square::power_on()
{
	//square duty units are reset to the first step of the waveform
	duty.reset_step();
}

void c_gbapu::c_square::write(int reg, uint8_t data)
{
	/*
	NR10 FF10 -PPP NSSS Sweep period, negate, shift
	NR11 FF11 DDLL LLLL Duty, Length load (64-L)
	NR12 FF12 VVVV APPP Starting volume, Envelope add mode, period
	NR13 FF13 FFFF FFFF Frequency LSB
	NR14 FF14 TL-- -FFF Trigger, Length enable, Frequency MSB
	*/
	switch (reg) {
	case 0:
		sweep_period = (data >> 4) & 0x7;
		sweep_shift = data & 0x7;
		sweep_negate = data & 0x8 ? -1 : 1;
		break;
	case 1:
		duty.set_duty_cycle(data >> 6);
		length.set_length(64-(data & 0x3F));
		break;
	case 2:
		starting_volume = data >> 4;
		envelope.set_volume(starting_volume);
		envelope.set_mode(data & 0x8);
		envelope_period = data & 0x7;
		envelope.set_period(envelope_period);
		dac_power = data >> 3;
		if (!dac_power) {
			enabled = 0;
		}
		break;
	case 3:
		period_lo = data;
		timer.set_period((2048 - ((period_hi << 8) | period_lo)) * 4);
		break;
	case 4:
		period_hi = data & 0x7;
		timer.set_period((2048 - ((period_hi << 8) | period_lo)) * 4);
		length.set_enable(data & 0x40);
		if (data & 0x80) {
			trigger();
		}
		break;
	}
}

void c_gbapu::c_square::trigger()
{
	if (length.counter == 0) {
		length.counter = 64;
	}
	int p = (period_hi << 8) | period_lo;
	int freq = (2048 - p) * 4;
	timer.set_period(freq);
	sweep_shadow = p;
	sweep_counter = sweep_period;
	if (sweep_period || sweep_shift) {
		sweep_enabled = 1;
		calc_sweep();
	}
	else {
		sweep_enabled = 0;
	}
	envelope.set_period(envelope_period);
	envelope.set_volume(starting_volume);
	enabled = dac_power ? 1 : 0;
}

int c_gbapu::c_square::calc_sweep()
{
	sweep_freq = sweep_shadow + ((sweep_shadow >> sweep_shift) * sweep_negate);
	if (sweep_freq > 2047) {
		enabled = 0;
		sweep_enabled = 0;
	}
	return sweep_freq;
}

void c_gbapu::c_square::clock_sweep()
{
	if (sweep_enabled && sweep_period)
	{
		int prev = sweep_counter;
		sweep_counter = (sweep_counter - 1) & 0x7;
		if (sweep_counter == 0 && prev) {
			sweep_counter = sweep_period;
			int f = calc_sweep();
			if (f < 2048 && sweep_shift) {
				sweep_shadow = f;
				timer.set_period((2048-f) * 4);
				calc_sweep();
			}
		}
	}
}

int c_gbapu::c_square::get_output()
{
	if (enabled && dac_power && duty.get_output()/* && length.get_output()*/)
	{
		return envelope.get_output();
	}
	return 0;
}

c_gbapu::c_noise::c_noise()
{
	reset();
}

c_gbapu::c_noise::~c_noise()
{

}

void c_gbapu::c_noise::reset()
{
	lfsr = ~1;
	lfsr &= 0x7FFF;
	enabled = 0;
	clock_shift = 0;
	width_mode = 0;
	divisor_code = 0;
	starting_volume = 0;
	envelope_period = 0;
	timer.reset();
	envelope.reset();
	length.reset();
	length.counter = 64;
	timer.set_period(8);
	dac_power = 0;
}

const int c_gbapu::c_noise::divisor_table[8] = { 8, 16, 32, 48, 64, 80, 96, 112 };

void c_gbapu::c_noise::write(uint16_t address, uint8_t data)
{
	/*
	NR41 FF20 --LL LLLL Length load (64-L)
	NR42 FF21 VVVV APPP Starting volume, Envelope add mode, period
	NR43 FF22 SSSS WDDD Clock shift, Width mode of LFSR, Divisor code
	NR44 FF23 TL-- ---- Trigger, Length enable
	*/

	switch (address) {
	case 0:
		length.set_length(64 - (data & 0x3F));
		break;
	case 1:
		starting_volume = data >> 4;
		envelope.set_volume(starting_volume);
		envelope.set_mode(data & 0x8);
		envelope_period = data & 0x7;
		envelope.set_period(envelope_period);
		dac_power = data >> 3;
		if (!dac_power) {
			enabled = 0;
		}
		break;
	case 2:
		clock_shift = data >> 4;
		width_mode = data & 0x8;
		divisor_code = data & 0x7;
		timer.set_period(divisor_table[divisor_code] << clock_shift);
		break;
	case 3:
		length.set_enable(data & 0x40);
		if (data & 0x80) {
			trigger();
		}
		break;
	}
}

void c_gbapu::c_noise::trigger()
{
	lfsr = ~1;
	lfsr &= 0x7FFF;
	if (length.counter == 0) {
		length.counter = 64;
	}
	envelope.set_period(envelope_period);
	envelope.set_volume(starting_volume);
	enabled = dac_power ? 1 : 0;

}

void c_gbapu::c_noise::clock_timer()
{
	if (timer.clock()) {
		int x = (lfsr & 0x1) ^ ((lfsr & 0x2) >> 1);
		lfsr >>= 1;
		lfsr |= (x << 14);
		lfsr &= 0x7FFF;
		if (width_mode) {
			lfsr &= (~0x40);
			lfsr |= (x << 6);
		}
	}
}

void c_gbapu::c_noise::clock_length()
{
	if (length.clock() == 0) {
		enabled = 0;
	}
}

void c_gbapu::c_noise::clock_envelope()
{
	envelope.clock();
}

int c_gbapu::c_noise::get_output()
{
	if (enabled && dac_power) {
		if ((~lfsr) & 0x1) {
			return envelope.get_output();
		}
	}
	return 0;
}

void c_gbapu::c_noise::power_on()
{

}

c_gbapu::c_wave::c_wave()
{
	reset();
}

c_gbapu::c_wave::~c_wave()
{

}

void c_gbapu::c_wave::reset()
{
	enabled = 0;
	timer_period = 0;
	timer.reset();
	length.reset();
	length.counter = 256;
	period_hi = 0;
	period_lo = 0;
	timer.set_period(2048 * 2);
	memset(wave_table, 0, sizeof(wave_table));
	volume_shift = 4;
	dac_power = 0;
}

void c_gbapu::c_wave::power_on()
{
	sample_buffer = 0;
}

void c_gbapu::c_wave::clock()
{

}

int c_gbapu::c_wave::get_output()
{
	if (enabled && dac_power) {
		return sample_buffer >> volume_shift;
	}
	return 0;
}

void c_gbapu::c_wave::clock_timer()
{
	if (timer.clock()) {
		wave_pos = (wave_pos + 1) & 0x1F;
		sample_buffer = wave_table[wave_pos];
	}
}

void c_gbapu::c_wave::clock_length()
{
	if (length.clock() == 0) {
		enabled = 0;
	}
}

void c_gbapu::c_wave::write(uint16_t address, uint8_t data)
{
	/*
	NR30 FF1A E--- ---- DAC power
	NR31 FF1B LLLL LLLL Length load (256-L)
	NR32 FF1C -VV- ---- Volume code (00=0%, 01=100%, 10=50%, 11=25%)
	NR33 FF1D FFFF FFFF Frequency LSB
	NR34 FF1E TL-- -FFF Trigger, Length enable, Frequency MSB
	*/
	static const int shift_table[4] = { 4, 0, 1, 2 };
	if (address >= 0xFF30) {
		int* p = &wave_table[(address - 0xFF30) * 2];
		*p = data >> 4;
		*(p + 1) = data & 0xF;
	}
	else {
		switch (address - 0xFF1A) {
		case 0:
			dac_power = data & 0x80;
			if (!dac_power) {
				enabled = 0;
			}
			break;
		case 1:
			length.set_length(256 - data);
			break;
		case 2:
			volume_shift = shift_table[(data >> 5) & 0x3];
			break;
		case 3:
			period_lo = data;
			timer.set_period((2048 - ((period_hi << 8) | period_lo)) * 2);
			break;
		case 4:
			period_hi = data & 0x7;
			timer.set_period((2048 - ((period_hi << 8) | period_lo)) * 2);
			length.set_enable(data & 0x40);
			if (data & 0x80) {
				trigger();
			}
			break;
		}
	}
}

void c_gbapu::c_wave::trigger()
{
	if (length.counter == 0) {
		length.counter = 256;
	}
	timer.set_period((2048 - ((period_hi << 8) | period_lo)) * 2);
	wave_pos = 0;
	enabled = dac_power ? 1 : 0;
}