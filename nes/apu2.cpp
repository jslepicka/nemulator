#include "apu2.h"
#include <memory>
#include <assert.h>
#include "nes.h"
#include "mapper.h"
#include "cpu.h"
#include "apu_luts.h"
//#include <xmmintrin.h>

//#define AUDIO_LOG 1
#ifdef AUDIO_LOG
std::ofstream file;
#endif

const float c_apu2::NES_AUDIO_RATE = 341.0f / 3.0f * 262.0f * 60.0f/* / 3.0f*/;

c_apu2::c_apu2(void)
{
	squares[0] = &square1;
	squares[1] = &square2;
	mixer_enabled = 1;
#ifdef AUDIO_LOG
	file.open("c:\\log\\audio.out", std::ios_base::trunc | std::ios_base::binary);
#endif
	sound_buffer = new int32_t[1024];
}

c_apu2::~c_apu2(void)
{
#ifdef AUDIO_LOG
	file.close();
#endif
	if (sound_buffer)
		delete[] sound_buffer;
	if (resampler)
		delete resampler;
	if (lpf)
		delete lpf;
	if (post_filter)
		delete post_filter;
}

void c_apu2::enable_mixer()
{
	mixer_enabled = 1;
}

void c_apu2::disable_mixer()
{
	mixer_enabled = 0;
}

void c_apu2::set_nes(c_nes* nes)
{
	this->nes = nes;
	dmc.set_nes(nes);
};

void c_apu2::reset()
{
	ticks = 0;
	frame_seq_counter = CLOCKS_PER_FRAME_SEQ;
	int x = sizeof(reg);
	for (int i = 0; i < sizeof(reg); i++)
		reg[i] = 0;
	frame_seq_step = 0;
	frame_seq_steps = 4;
	square1.set_sweep_mode(1);
	frame_irq_enable = 0;
	frame_irq_flag = 0;
	frame_irq_asserted = 0;
	square1.reset();
	square2.reset();
	triangle.reset();
	noise.reset();
	dmc.reset();
	square_clock = 0;
	memset(sound_buffer, 0, sizeof(int32_t) * 1024);
	/*
	lowpass elliptical, 20kHz
	d = fdesign.lowpass('N,Fp,Ap,Ast', 8, 20000, .1, 80, 1786840);
	Hd = design(d, 'ellip', 'FilterStructure', 'df2tsos');
	set(Hd, 'Arithmetic', 'single');
	g = regexprep(num2str(reshape(Hd.ScaleValues(1:4), [1 4]), '%.16ff '), '\s+', ',')
	b2 = regexprep(num2str(Hd.sosMatrix(5:8), '%.16ff '), '\s+', ',')
	a2 = regexprep(num2str(Hd.sosMatrix(17:20), '%.16ff '), '\s+', ',')
	a3 = regexprep(num2str(Hd.sosMatrix(21:24), '%.16ff '), '\s+', ',')
	*/
	lpf = new c_biquad4(
		{ 0.5086284279823303f,0.3313708603382111f,0.1059221103787422f,0.0055782101117074f },
		{ -1.9872593879699707f,-1.9750031232833862f,-1.8231037855148315f,-1.9900115728378296f },
		{ -1.9759204387664795f,-1.9602127075195313f,-1.9470522403717041f,-1.9888486862182617f },
		{ 0.9801648259162903f,0.9627774357795715f,0.9480593800544739f,0.9940192103385925f }
	);
	/*
	post-filter is butterworth bandpass, 30Hz - 14kHz
	d = fdesign.bandpass('N,F3dB1,F3dB2', 2, 30, 14000, 48000);
	Hd = design(d,'butter', 'FilterStructure', 'df2tsos');
	set(Hd, 'Arithmetic', 'single');
	g = num2str(Hd.ScaleValues(1), '%.16ff ')
	b = regexprep(num2str(Hd.sosMatrix(1:3), '%.16ff '), '\s+', ',')
	a = regexprep(num2str(Hd.sosMatrix(4:6), '%.16ff '), '\s+', ',')
	*/
	/*post_filter = new c_biquad(
		0.5648277401924133f,
		{ 1.0000000000000000f,0.0000000000000000f,-1.0000000000000000f },
		{ 1.0000000000000000f,-0.8659016489982605f,-0.1296554803848267f }
	);*/

	//12kHz
	post_filter = new c_biquad(
		0.4990182518959045f,
		{ 1.0000000000000000f,0.0000000000000000f,-1.0000000000000000f },
		{ 1.0000000000000000f,-0.9980365037918091f,0.0019634978380054f }
	);

	resampler = new c_resampler(NES_AUDIO_RATE / 48000.0f, lpf, post_filter);
}

int c_apu2::get_buffer(const short** buf)
{
	int num_samples = resampler->get_output_buf(buf);
	return num_samples;
}

void c_apu2::set_audio_rate(double freq)
{
	double x = NES_AUDIO_RATE / freq;
	resampler->set_m(x);
}

void c_apu2::write_byte(unsigned short address, unsigned char value)
{
	reg[address - 0x4000] = value;
	switch (address - 0x4000)
	{
	case 0x0:
	case 0x1:
	case 0x2:
	case 0x3:
		squares[(address & 0x4) >> 2]->write(address, value);
		break;
	case 0x4:
	case 0x5:
	case 0x6:
	case 0x7:
		squares[(address & 0x4) >> 2]->write(address, value);
		break;
	case 0x8:
	case 0xA:
	case 0xB:
		triangle.write(address, value);
		break;
	case 0xC:
	case 0xE:
	case 0xF:
		noise.write(address, value);
		break;
	case 0x10:
	case 0x11:
	case 0x12:
	case 0x13:
		dmc.write(address, value);
		break;
	case 0x15:
		if (value & 0x1)
			square1.enable();
		else
			square1.disable();
		if (value & 0x2)
			square2.enable();
		else
			square2.disable();
		if (value & 0x4)
			triangle.enable();
		else
			triangle.disable();
		if (value & 0x8)
			noise.enable();
		else
			noise.disable();
		if (value & 0x10)
			dmc.enable();
		else
			dmc.disable();
		dmc.ack_irq();
		break;
	case 0x17:
	{
		frame_irq_enable = !(value & 0x40);
		if (!frame_irq_enable && frame_irq_flag)
		{
			if (frame_irq_asserted)
			{
				nes->cpu->clear_irq();
				frame_irq_asserted = 0;
			}
		}
		else if (frame_irq_enable && frame_irq_flag)
		{
			if (!frame_irq_asserted)
			{
				nes->cpu->execute_irq();
				frame_irq_asserted = 1;
			}
		}
		frame_seq_step = 0;
		frame_seq_counter = CLOCKS_PER_FRAME_SEQ;
		if (value & 0x80)
		{
			frame_seq_steps = 5;
			clock_frame_seq();
		}
		else
		{
			frame_seq_steps = 4;
		}
	}
	break;
	default:
		break;
	}
}

unsigned char c_apu2::read_byte(unsigned short address)
{
	//can only read 0x4015
	if (address == 0x4015)
	{
		int retval = 0;
		retval |= square1.get_status();
		retval |= (square2.get_status() << 1);
		retval |= (triangle.get_status() << 2);
		retval |= (noise.get_status() << 3);
		retval |= (dmc.get_status() << 4);
		retval |= (dmc.get_irq_flag() << 7);
		retval |= ((frame_irq_flag ? 1 : 0) << 6);

		if (frame_irq_flag)
		{
			frame_irq_flag = 0;
			if (frame_irq_asserted)
			{
				nes->cpu->clear_irq();
				frame_irq_asserted = 0;
			}
		}

		return retval;

	}
	return reg[address - 0x4000];
}
void c_apu2::clock_once()
{
	frame_seq_counter -= 12;
	if (frame_seq_counter < 0)
	{
		frame_seq_counter += CLOCKS_PER_FRAME_SEQ;
		clock_frame_seq();
	}
	clock_timers();
	if (mixer_enabled)
	{
		mix();
	}
}

void c_apu2::clock(int cycles)
{
	ticks += cycles;

	while (ticks > 2) //1 cpu cycle
	{
		clock_once();
		ticks -= 3;
	}
}

void c_apu2::mix()
{
	int square_vol = square1.get_output() + square2.get_output();
	float square_out = square_lut[square_vol];

	int triangle_vol = triangle.get_output();
	int noise_vol = noise.get_output();
	int dmc_vol = dmc.get_output();
	float tnd_out = tnd_lut[3 * triangle_vol + 2 * noise_vol + dmc_vol];

	float sample = square_out + tnd_out;
	if (nes->mapper->has_expansion_audio())
		sample = nes->mapper->mix_audio(sample);
	resampler->process(sample);
}

void c_apu2::clock_timers()
{
	if (square_clock)
	{
		square1.clock_timer();
		square2.clock_timer();
	}
	square_clock ^= 1;
	triangle.clock_timer();
	noise.clock_timer();
	dmc.clock_timer();
}

void c_apu2::clock_envelopes()
{
	square1.clock_envelope();
	square2.clock_envelope();
	triangle.clock_linear();
	noise.clock_envelope();
}

void c_apu2::clock_length_sweep()
{
	square1.clock_length_sweep();
	square2.clock_length_sweep();
	triangle.clock_length();
	noise.clock_length();
}

void c_apu2::clock_frame_seq()
{
	if (frame_seq_steps == 5)
	{
		switch (frame_seq_step)
		{
		case 0:
			clock_envelopes();
			clock_length_sweep();
			frame_seq_step = 1;
			break;
		case 1:
			clock_envelopes();
			frame_seq_step = 2;
			break;
		case 2:
			clock_envelopes();
			clock_length_sweep();
			frame_seq_step = 3;
			break;
		case 3:
			clock_envelopes();
			frame_seq_step = 4;
			break;
		case 4:
			frame_seq_step = 0;
			break;
		}
		//frame_seq_step = ++frame_seq_step % 5;
	}
	else
	{
		switch (frame_seq_step)
		{
		case 0:
			clock_envelopes();
			frame_seq_step = 1;
			break;
		case 1:
			clock_envelopes();
			clock_length_sweep();
			frame_seq_step = 2;
			break;
		case 2:
			clock_envelopes();
			frame_seq_step = 3;
			break;
		case 3:
			clock_envelopes();
			clock_length_sweep();
			//if irq flag, and !irq disable, execute interrupt
			frame_irq_flag = 1;
			if (frame_irq_enable && !frame_irq_asserted)
			{
				frame_irq_asserted = 1;
				nes->cpu->execute_irq();
			}
			frame_seq_step = 0;
			break;
		}
		//frame_seq_step = ++frame_seq_step & 0x3;
	}

}

c_apu2::c_envelope::c_envelope()
{
	reset();
}

c_apu2::c_envelope::~c_envelope()
{
}

void c_apu2::c_envelope::reset()
{
	reset_flag = 0;
	counter = 1;
	output = 0;
	period = 1;
	enabled = 0;
	loop = 0;
	env_vol = 0;
}

void c_apu2::c_envelope::clock()
{
	if (reset_flag)
	{
		reset_flag = 0;
		env_vol = 15;
		counter = period;
	}
	else if (--counter == 0)
	{
		if (env_vol || loop)
			env_vol = (env_vol - 1) & 0xF;
		counter = period;
	}

	if (enabled)
		output = env_vol;
}

void c_apu2::c_envelope::write(unsigned char value)
{
	period = (value & 0xF) + 1;
	enabled = !(value & 0x10);
	loop = value & 0x20;
	output = enabled ? env_vol : value & 0xF;
}

void c_apu2::c_envelope::reset_counter()
{
	reset_flag = 1;
}

int c_apu2::c_envelope::get_output()
{
	return output;
}

c_apu2::c_timer::c_timer()
{
	reset();
}

c_apu2::c_timer::~c_timer()
{
}

void c_apu2::c_timer::reset()
{
	period = 1;
	counter = 1;
	//output = 0;
}

int c_apu2::c_timer::clock()
{
	if (--counter == 0)
	{
		counter = (period + 1);
		return 1;
	}
	else
	{
		return 0;
	}
}

//int c_apu2::c_timer::clock2x()
//{
//	if (--counter == 0)
//	{
//		counter = (period + 1) << 1;
//		return 1;
//	}
//	else
//	{
//		return 0;
//	}
//}

void c_apu2::c_timer::set_period_lo(int period_lo)
{
	period = (period & 0x0700) | (period_lo & 0xFF);
}

void c_apu2::c_timer::set_period_hi(int period_hi)
{
	period = (period & 0xFF) | ((period_hi & 0x7) << 8);
}

int c_apu2::c_timer::get_period()
{
	return period;
}


void c_apu2::c_timer::set_period(int value)
{
	period = value;
}

c_apu2::c_sequencer::c_sequencer()
{
	reset();
}

c_apu2::c_sequencer::~c_sequencer()
{
}

void c_apu2::c_sequencer::reset()
{
	duty_cycle = 0;
	step = 0;
	output = 0;
	ticks = 0;
}

int c_apu2::c_sequencer::get_output()
{
	return duty_cycle_table[duty_cycle][step];
}

void c_apu2::c_sequencer::reset_step()
{
	step = 0;
	//TODO: silence output?
	//output = 0;
}

void c_apu2::c_sequencer::set_duty_cycle(int duty)
{
	duty_cycle = duty & 0x3;
}

const int c_apu2::c_sequencer::duty_cycle_table[4][8] = {
	{0, 1, 0, 0, 0, 0, 0, 0},
	{0, 1, 1, 0, 0, 0, 0, 0},
	{0, 1, 1, 1, 1, 0, 0, 0},
	{1, 0, 0, 1, 1, 1, 1, 1}
};

void c_apu2::c_sequencer::clock()
{
	step = --step & 0x7;
}

c_apu2::c_length::c_length()
{
	reset();
}

c_apu2::c_length::~c_length()
{
}

void c_apu2::c_length::reset()
{
	counter = 0;
	halt = 0;
	length = 0;
	enabled = 0;
}

void c_apu2::c_length::clock()
{
	if (!halt && counter != 0)
		counter--;
}

int c_apu2::c_length::get_output()
{
	return counter;
}

int c_apu2::c_length::get_counter()
{
	return counter;
}

void c_apu2::c_length::set_halt(int halt)
{
	this->halt = halt;
}

void c_apu2::c_length::enable()
{
	enabled = 1;
}

void c_apu2::c_length::disable()
{
	enabled = 0;
	counter = 0;
}

int c_apu2::c_length::get_halt()
{
	return halt;
}

const int c_apu2::c_length::length_table[32] =
{
	0x0A, 0xFE, 0x14, 0x02, 0x28, 0x04, 0x50, 0x06,
	0xA0, 0x08, 0x3C, 0x0A, 0x0E, 0x0C, 0x1A, 0x0E,
	0x0C, 0x10, 0x18, 0x12, 0x30, 0x14, 0x60, 0x16,
	0xC0, 0x18, 0x48, 0x1A, 0x10, 0x1C, 0x20, 0x1E
};

void c_apu2::c_length::set_length(int index)
{
	if (enabled)
		counter = length_table[index & 0x1F];
}

c_apu2::c_square::c_square()
{
	reset();
}

void c_apu2::c_square::reset()
{
	sweep_silencing = 1;
	output = 0;
	sweep_reg = 0;
	sweep_period = 0;
	sweep_reset = 0;
	sweep_enable = 0;
	sweep_negate = 0;
	sweep_shift = 0;
	sweep_output = 1;
	sweep_mode = 0;
	sweep_counter = 1;
	envelope.reset();
	timer.reset();
	sequencer.reset();
	length.reset();
}

c_apu2::c_square::~c_square()
{
}

void c_apu2::c_square::clock_timer()
{
	if (timer.clock())
		sequencer.clock();
}

void c_apu2::c_square::clock_envelope()
{
	envelope.clock();
}

void c_apu2::c_square::clock_length()
{
	length.clock();
}

void c_apu2::c_square::clock_length_sweep()
{
	//length.clock();
	clock_length();
	int period = timer.get_period();
	if (sweep_period != 0 && --sweep_counter == 0)
	{
		sweep_counter = sweep_period;
		if (period >= 8)
		{
			sweep_output = 1;
			int offset = period >> sweep_shift;
			if (sweep_negate)
			{
				if (sweep_shift && sweep_enable)
				{
					timer.set_period(period + ((-1 * sweep_mode) - offset));
					update_sweep_silencing();
				}
			}
			else
			{
				if (period + offset < 0x800)
				{
					if (sweep_shift && sweep_enable)
					{
						timer.set_period(period + offset);
						update_sweep_silencing();
					}
				}
			}
		}
	}
	if (sweep_reset)
	{
		sweep_reset = 0;
		sweep_counter = sweep_period;
	}
}

int c_apu2::c_square::get_output()
{
	if (!sweep_silencing && sequencer.get_output() && length.get_output())
		return envelope.get_output();
	return 0;
}

int c_apu2::c_square::get_output_mmc5()
{
	//MMC5 squares don't have the sweep unit.  This is faster than turning get_output
	//into a virtual function call.
	if (sequencer.get_output() && length.get_output())
		return envelope.get_output();
	return 0;
}

void c_apu2::c_square::write(unsigned short address, unsigned char value)
{
	int period;
	switch (address & 0x3)
	{
	case 0x0:
		length.set_halt((value & 0x20) >> 5);
		envelope.write(value);
		sequencer.set_duty_cycle(value >> 6);
		break;
	case 0x1:
		sweep_enable = value & 0x80;
		sweep_negate = value & 0x8;
		sweep_shift = value & 0x7;
		sweep_period = 0;
		sweep_period = ((value & 0x70) >> 4) + 1;
		sweep_reset = 1;
		break;
	case 0x2:
		timer.set_period_lo(value);
		period = timer.get_period();
		update_sweep_silencing();
		break;
	case 0x3:
		sequencer.reset_step();
		timer.set_period_hi(value);
		length.set_length((value >> 3) & 0x1F);
		envelope.reset_counter();
		update_sweep_silencing();
		break;
	}
}

void c_apu2::c_square::update_sweep_silencing()
{
	int period = timer.get_period();
	int offset = period >> sweep_shift;
	sweep_silencing = ((period < 8) || (!sweep_negate && ((period + offset) > 0x7FF)));
}

void c_apu2::c_square::enable()
{
	length.enable();
}

void c_apu2::c_square::disable()
{
	length.disable();
}

int c_apu2::c_square::get_status()
{
	return length.get_counter() ? 1 : 0;
}

void c_apu2::c_square::set_sweep_mode(int mode)
{
	sweep_mode = mode & 0x1;
}

const int c_apu2::c_triangle::sequence[32] = {
	0xF, 0xE, 0xD, 0xC, 0xB, 0xA, 0x9, 0x8, 0x7, 0x6, 0x5, 0x4, 0x3, 0x2, 0x1, 0x0,
	0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF
};

c_apu2::c_triangle::c_triangle()
{
	reset();
}

c_apu2::c_triangle::~c_triangle()
{
}

void c_apu2::c_triangle::reset()
{
	output = 0;
	sequence_pos = 0;
	linear_counter = 0;
	linear_reload = 0;
	length_enabled = 0;
	linear_control = 0;
	linear_halt = 0;
	timer.reset();
	length.reset();
}

void c_apu2::c_triangle::clock_timer()
{
	//               +---------+    +---------+
	//               |LinearCtr|    | Length  |
	//               +---------+    +---------+
	//                    |              |
	//                    v              v
	//+---------+        |\             |\         +---------+    +---------+ 
	//|  Timer  |------->| >----------->| >------->|Sequencer|--->|   DAC   |
	//+---------+        |/             |/         +---------+    +---------+ 

	if (timer.clock())
	{
		if (length.get_output() && linear_counter)
		{
			output = sequence[sequence_pos];
			sequence_pos = ++sequence_pos & 0x1F;
			//need this for crappy output filter otherwise freq is too high
			//and causes aliasing
			//if (timer.get_period() < 2)
			//	output = 0x7;
		}
	}
}

void c_apu2::c_triangle::enable()
{
	length.enable();
}

void c_apu2::c_triangle::disable()
{
	length.disable();
}

void c_apu2::c_triangle::write(unsigned short address, unsigned char value)
{
	switch (address - 0x4000)
	{
	case 0x8:
		length_enabled = !(value & 0x80);
		linear_control = (value & 0x80);
		linear_reload = (value & 0x7F);
		length.set_halt((value & 0x80) >> 7);
		break;
	case 0xA:
		timer.set_period_lo(value);
		break;
	case 0xB:
		linear_halt = 1;
		timer.set_period_hi(value & 0x7);
		length.set_length(value >> 3);
		length.set_halt(1);
		break;
	default:
		break;
	}
}

int c_apu2::c_triangle::get_output()
{
	return output;
}

int c_apu2::c_triangle::get_status()
{
	return length.get_counter() ? 1 : 0;
}

void c_apu2::c_triangle::clock_length()
{
	length.clock();
}

void c_apu2::c_triangle::clock_linear()
{
	if (linear_halt)
	{
		linear_counter = linear_reload;
	}
	else if (linear_counter != 0)
	{
		linear_counter--;
	}
	if (!linear_control)
	{
		linear_halt = 0;
	}
}

c_apu2::c_noise::c_noise()
{
	reset();
}

c_apu2::c_noise::~c_noise()
{
}

void c_apu2::c_noise::reset()
{
	output = 0;
	lfsr_shift = 1;
	random_period = 0;
	random_counter = 0;
	length_enabled = 0;
	lfsr = 0x1;
	length.reset();
	envelope.reset();
	timer.reset();
}

const int c_apu2::c_noise::random_period_table[16] = {
	0x004, 0x008, 0x010, 0x020, 0x040, 0x060, 0x080, 0x0A0,
	0x0CA, 0x0FE, 0x17C, 0x1FC, 0x2FA, 0x3F8, 0x7F2, 0xFE4
};

void c_apu2::c_noise::write(unsigned short address, unsigned char value)
{
	//$400C   --le nnnn   loop env/disable length, env disable, vol/env period
	//$400E   s--- pppp   short mode, period index
	//$400F   llll l---   length index
	switch (address - 0x4000)
	{
	case 0xC:
		length.set_halt((value & 0x20) >> 5);
		envelope.write(value);
		break;
	case 0xE:
		lfsr_shift = (value & 0x80) ? 6 : 1;
		random_period = random_period_table[value & 0xF];
		timer.set_period(random_period);
		break;
	case 0xF:
		length.set_length(value >> 3);
		envelope.reset_counter();
		break;
	}
}

void c_apu2::c_noise::clock_timer()
{
	//timer.clock(0);
	if (timer.clock())
	{
		int	feedback = (lfsr & 0x1) ^ ((lfsr >> lfsr_shift) & 0x1);
		feedback <<= 14;
		lfsr = (lfsr >> 1) | feedback;
	}
}

void c_apu2::c_noise::clock_envelope()
{
	envelope.clock();
}

void c_apu2::c_noise::clock_length()
{
	length.clock();
}

int c_apu2::c_noise::get_output()
{
	if (length.get_output() && !(lfsr & 0x1))
	{
		return envelope.get_output();
	}
	else
	{
		return 0;
	}
}

int c_apu2::c_noise::get_status()
{
	return length.get_counter() ? 1 : 0;
}

void c_apu2::c_noise::enable()
{
	length.enable();
}

void c_apu2::c_noise::disable()
{
	length.disable();
}

c_apu2::c_dmc::c_dmc()
{
	reset();
}

c_apu2::c_dmc::~c_dmc()
{
}

void c_apu2::c_dmc::reset()
{
	cycle = 1;
	silence = 1;
	output_shift = 0;
	output_counter = 0;
	sample_buffer_empty = 1;
	bits_remain = 1;
	duration = 0;
	enabled = 0;
	loop = 0;
	sample_address = 0;
	sample_length = 1;
	irq_enable = 0;
	irq_asserted = 0;
	irq_flag = 0;
	timer.reset();
}

int c_apu2::c_dmc::get_output()
{
	return output_counter;
}

const int c_apu2::c_dmc::freq_table[16] = {
	0x1AC, 0x17C, 0x154, 0x140, 0x11E, 0x0FE, 0x0E2, 0x0D6,
	0x0BE, 0x0A0, 0x08E, 0x080, 0x06A, 0x054, 0x048, 0x036
};

void c_apu2::c_dmc::write(unsigned short address, unsigned char value)
{
	//$4010   il-- ffff   IRQ enable, loop, frequency index
	//$4011   -ddd dddd   DAC
	//$4012   aaaa aaaa   sample address
	//$4013   llll llll   sample length
	switch (address - 0x4000)
	{
	case 0x10:
		loop = value & 0x40;
		irq_enable = value & 0x80;
		freq_index = value & 0xF;
		timer.set_period(freq_table[freq_index] - 1);
		if (!irq_asserted && irq_enable && irq_flag)
		{
			irq_asserted = 1;
			nes->cpu->execute_irq();
		}
		else if (!irq_enable)
		{
			if (irq_asserted)
			{
				irq_asserted = 0;
				nes->cpu->clear_irq();
			}
			irq_flag = 0;
		}
		break;
	case 0x11:
		output_counter = (value & 0x7F);
		break;
	case 0x12:
		sample_address = 0x4000 + value * 0x40;
		break;
	case 0x13:
		sample_length = value * 0x10 + 1;
		break;
	}
}

int c_apu2::c_dmc::get_status()
{
	return duration ? 1 : 0;
}

void c_apu2::c_dmc::clock_timer()
{
	if (timer.clock())
	{
		if (!silence)
		{
			if (output_shift & 0x1)
			{
				if (output_counter < 126)
					output_counter += 2;
			}
			else
			{
				if (output_counter > 1)
					output_counter -= 2;
			}
		}
		output_shift >>= 1;
		if (--bits_remain == 0)
		{
			bits_remain = 8;
			if (sample_buffer_empty)
			{
				silence = 1;
			}
			else
			{
				output_shift = sample_buffer;
				sample_buffer_empty = 1;
				fill_sample_buffer();
				silence = 0;
			}
		}
	}
}

void c_apu2::c_dmc::set_nes(c_nes* nes)
{
	this->nes = nes;
}

void c_apu2::c_dmc::enable()
{
	if (duration == 0)
	{
		address = sample_address;
		duration = sample_length;
		fill_sample_buffer();
		enabled = 1;
	}
}

void c_apu2::c_dmc::disable()
{
	duration = 0;
	enabled = 0;
	//disable irq?
	//ack_irq();
}

void c_apu2::c_dmc::ack_irq()
{
	irq_flag = 0;
	if (irq_asserted)
	{
		nes->cpu->clear_irq();
		irq_asserted = 0;
	}
}

void c_apu2::c_dmc::fill_sample_buffer()
{
	if (sample_buffer_empty && duration)
	{
		output_shift = nes->DmcRead(0x8000 + address);
		address = (address + 1) & 0x7FFF;
		sample_buffer_empty = 0;
		if (--duration == 0)
		{
			if (loop)
			{
				address = sample_address;
				duration = sample_length;
			}
			else
			{
				if (irq_enable)
				{
					irq_flag = 1;
					if (irq_flag && !irq_asserted)
					{
						nes->cpu->execute_irq();
						irq_asserted = 1;
					}
				}
				else
				{
					irq_flag = 0;
				}
			}
		}
	}
}

int c_apu2::c_dmc::get_irq_flag()
{
	return irq_flag ? 1 : 0;
}