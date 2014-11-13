#include "mapper85.h"
#include "..\cpu.h"
#include <math.h>

const int c_mapper85::instruments[16][8] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x03, 0x21, 0x04, 0x06, 0x8D, 0xF2, 0x42, 0x17,  // instrument 1
	0x13, 0x41, 0x05, 0x0E, 0x99, 0x96, 0x63, 0x12,  // instrument 2
	0x31, 0x11, 0x10, 0x0A, 0xF0, 0x9C, 0x32, 0x02,  // instrument 3
	0x21, 0x61, 0x1D, 0x07, 0x9F, 0x64, 0x20, 0x27,  // instrument 4
	0x22, 0x21, 0x1E, 0x06, 0xF0, 0x76, 0x08, 0x28,  // instrument 5
	0x02, 0x01, 0x06, 0x00, 0xF0, 0xF2, 0x03, 0x95,  // instrument 6
	0x21, 0x61, 0x1C, 0x07, 0x82, 0x81, 0x16, 0x07,  // instrument 7
	0x23, 0x21, 0x1A, 0x17, 0xEF, 0x82, 0x25, 0x15,  // instrument 8
	0x25, 0x11, 0x1F, 0x00, 0x86, 0x41, 0x20, 0x11,  // instrument 9
	0x85, 0x01, 0x1F, 0x0F, 0xE4, 0xA2, 0x11, 0x12,  // instrument A
	0x07, 0xC1, 0x2B, 0x45, 0xB4, 0xF1, 0x24, 0xF4,  // instrument B
	0x61, 0x23, 0x11, 0x06, 0x96, 0x96, 0x13, 0x16,  // instrument C
	0x01, 0x02, 0xD3, 0x05, 0x82, 0xA2, 0x31, 0x51,  // instrument D
	0x61, 0x22, 0x0D, 0x02, 0xC3, 0x7F, 0x24, 0x05,  // instrument E
	0x21, 0x62, 0x0E, 0x00, 0xA1, 0xA0, 0x44, 0x17,  // instrument F
};

const float c_mapper85::pi = 3.1415926535897932384626433832795f;

c_mapper85::c_mapper85()
{
	//Lagrange Point (J)
	//Tiny Toon Adventures 2 (J)
	mapperName = "VRC7";

	for (int i = 0; i < 256; i++)
	{
		
		float sinx = sin(pi*i / 256.0f);
		if (!sinx)
			sin_table[i] = (1 << 23);
		else
		{
			sinx = linear2db(sinx);
			if (sinx >(1 << 23))
				sin_table[i] = (1 << 23);
			else
				sin_table[i] = sinx;
		}
		//sin_table[i] = linear2db(sin(pi * i / 256.0f));
	}

	expansion_audio = 1;
}

void c_mapper85::reset()
{
	SetPrgBank8k(PRG_E000, prgRomPageCount8k - 1);
	irq_counter = 0;
	irq_asserted = 0;
	irq_control = 0;
	irq_reload = 0;
	irq_scaler = 0;
	audio_register_select = 0;
	ticks = 0;
	audio_ticks = 0;
	memset(phase, 0, sizeof(phase));
	memset(phase_secondary, 0, sizeof(phase_secondary));
	memset(channel_regs, 0, sizeof(channel_regs));
	memset(egc, 0, sizeof(egc));
	memset(envelope_state, 0, sizeof(envelope_state));
	audio_output = 0.0f;
	last_audio_output = 0.0f;
	am_counter = 0;
	fm_counter = 0;
}

void c_mapper85::WriteByte(unsigned short address, unsigned char value)
{
	if (address >= 0x8000)
	{
		switch (address >> 12)
		{
		case 0x8:
			if (address & 0x18)
				SetPrgBank8k(PRG_A000, value);
			else
				SetPrgBank8k(PRG_8000, value);
			break;
		case 0x9:
			if (address & 0x18)
			{
				if (address == 0x9010) // audio register select
				{
					audio_register_select = value;
				}
				else if (address == 0x9030) //audio register write
				{
					audio_register_write(audio_register_select, value);
				}
			}
			else
				SetPrgBank8k(PRG_C000, value);
			break;
		case 0xA:
		case 0xB:
		case 0xC:
		case 0xD:
		{
			int bank = ((address >> 12) - 0xA) * 2;
			if (address & 0x18)
				bank++;
			SetChrBank1k(bank, value);
		}
			break;
		case 0xE:
			if (address & 0x18)
			{
				irq_reload = value;
			}
			else
			{
				switch (value & 0x3)
				{
				case 0:
					set_mirroring(MIRRORING_VERTICAL);
					break;
				case 1:
					set_mirroring(MIRRORING_HORIZONTAL);
					break;
				case 2:
					set_mirroring(MIRRORING_ONESCREEN_LOW);
					break;
				case 3:
					set_mirroring(MIRRORING_ONESCREEN_HIGH);
					break;
				}
			}
			break;
		case 0xF:
			if (address & 0x18)
			{
				if (irq_asserted)
				{
					cpu->clear_irq();
					irq_asserted = 0;
				}
				if (irq_control & 0x1)
					irq_control |= 0x2;
				else
					irq_control &= ~0x2;
			}
			else
			{
				irq_control = value & 0x7;
				if (irq_control & 0x2)
				{
					irq_scaler = 0;
					irq_counter = irq_reload;
				}
				if (irq_asserted)
				{
					cpu->clear_irq();
					irq_asserted = 0;
				}
			}
			break;
		}
	}
	else
	{
		c_mapper::WriteByte(address, value);
	}
}

void c_mapper85::clock(int cycles)
{
	ticks += cycles;
	while (ticks > 2)
	{
		if ((irq_control & 0x02) && ((irq_control & 0x04) || ((irq_scaler += 3) >= 341)))
		{
			if (!(irq_control & 0x04))
			{
				irq_scaler -= 341;
			}
			if (irq_counter == 0xFF)
			{
				irq_counter = irq_reload;
				if (!irq_asserted)
				{
					cpu->execute_irq();
					irq_asserted = 1;
				}
			}
			else
				irq_counter++;
		}
		ticks -= 3;
		if (++audio_ticks == 36)
		{
			clock_audio();
			audio_ticks = 0;
		}
	}

	//ticks += cycles;
	//while (ticks >= 108)
	//{
	//	clock_audio();
	//	ticks -= 108;
	//}
}

void c_mapper85::audio_register_write(int audio_register, int value)
{
	//get key_state
	int channel = audio_register & 0x7;
	int prev_key_state = channel_regs[channel][2] & 0x10;

	if (audio_register & 0x30)
	{
		channel_regs[audio_register & 0x7][((audio_register & 0x30) >> 4)] = value;
	}
	else
	{
		custom_instrument[audio_register & 0x7] = value;
	}

	if (audio_register & 0x30)
	{
		int new_key_state = channel_regs[channel][2] & 0x10;
		if (prev_key_state ^ new_key_state)
		{
			if (new_key_state)
			{
				for (int slot = channel * 2; slot <= channel * 2 + 1; slot++)
				{
					key_on(slot);
				}
			}
			else
			{
				for (int slot = channel * 2; slot <= channel * 2 + 1; slot++)
				{
					key_off(slot);
				}
			}
		}
	}

}

void c_mapper85::clock_audio()
{
	static int multiplier[] = { 1, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 20, 24, 24, 30, 30 };
	audio_output = 0.0f;
	const int *inst;
	am_counter = (am_counter + 78) & 0xFFFFF;
	fm_counter = (fm_counter + 105) & 0xFFFFF;

	float am_output = (1.0f + sin(2.0f*pi*am_counter / (1 << 20))) * db(.6f);
	float fm_output = pow(2.0f, 13.75f / 1200.0f * sin(2.0f*pi*fm_counter / (1 << 20)));

	for (int channel = 0; channel < 6; channel++)
	{
		int f = channel_regs[channel][1] | ((channel_regs[channel][2] & 0x1) << 8);
		int b = (channel_regs[channel][2] & 0xE) >> 1;
		int instrument = channel_regs[channel][3] >> 4;

		if (channel == 0)
			inst = custom_instrument;
		else
			inst = instruments[instrument];


		for (int slot = channel*2; slot <= (channel*2)+1; slot++)
		{
			int is_carrier = slot & 1;
			int m = 0;
			m = multiplier[inst[is_carrier] & 0xF];
			int vibrato = inst[is_carrier] & 0x40 ? fm_output : 1;
			phase[slot] += ((f * (1 << b) * m * vibrato / 2) & 0x3FFFF); // * m * v / 2
			if (is_carrier)
			{
				phase_secondary[slot] = (phase[slot] + output[slot - 1]) & 0x3FFFF;

			}
			else
			{
				int feedback = 0;
				if (inst[3] & 0x7)
				{
					feedback = output[slot] >> (8 - (inst[3] & 0x7));
				}
				phase_secondary[slot] = (phase[slot] + feedback) & 0x3FFFF;

			}
			int s = sin_table[(phase_secondary[slot] >> 9) & 0xFF];
			s += clock_envelope(slot, inst);
			float base = 0.0f;
			if (is_carrier)
			{
				base = (db(3) * (channel_regs[channel][3] & 0xF));
			}
			else
			{
				base = (db(.75f) * (inst[2] & 0x3F));
			}
			s += base;
			s += inst[is_carrier] & 0x80 ? am_output : 0;
			//TOTAL = half_sine_table[I] + base + key_scale + envelope + AM
			if (s < (1 << 23))
			{
				float o = db2linear(s);
				if (o > 1.0f || o < 0.0f)
					int a = 1;
				if (phase_secondary[slot] & 0x20000)
				{
					if ((is_carrier && (inst[3] & 0x10)) ||
						(inst[3] & 0x8))
						o = 0.0f;
					else
						o = -o;
				}
				int adj_out = (int)(o * ((1 << 20)-1));
				output[slot] = adj_out;
				if (is_carrier)
					audio_output += o;
			}
			else
			{
				output[slot] = 0;
			}
		}

	}
	audio_output /= 6.0f;
	//audio_output += last_audio_output;
	//audio_output /= 2.0f;

}

float c_mapper85::mix_audio(float sample)
{
	return (sample *.5f) + (audio_output * .5f);
}

float c_mapper85::linear2db(float in)
{
	return -20.0f * log10(in) * ((1 << 23) / 48.0f);
}

float c_mapper85::db2linear(float in)
{
	return pow(10.0f, (in / -20.0f / ((1 << 23) / 48.0f)));
}

int c_mapper85::clock_envelope(int slot, const int *inst)
{
	int is_carrier = slot & 0x1;
	//slot attack rate
	//int r = is_carrier ? inst[5] >> 4 : inst[4] >> 4;
	int r = 0;
	int rks = 0;
	int rh = 0;
	int rl = 0;
	int bf = channel_regs[slot >> 1][2] & 0xF;
	int kb = inst[is_carrier] & 0x10 ? bf : bf >> 2;
	int sustain_level = 0;
	float out = 0.0f;

	switch (envelope_state[slot])
	{
	case ENV_IDLE:
		return (1 << 23);
		break;
	case ENV_ATTACK:
		r = is_carrier ? inst[5] >> 4 : inst[4] >> 4;
		if (r != 0)
		{
			rks = r * 4 + kb;//fix: add kb +
			rh = rks >> 2;
			if (rh > 15)
				rh = 15;
			rl = rks & 3;
			egc[slot] += ((12 * (rl + 4)) << rh);
			if (egc[slot] >= (1 << 23))
			{
				egc[slot] = 0;
				envelope_state[slot] = ENV_DECAY;
			}
		}
		//out = db(48.0f) - (db(48.0f) * log(float(egc[slot] == 0 ? 1 : egc[slot])) / log((float)(1 << 23)));
		//return (int)out;
		return egc[slot];
		break;
	case ENV_DECAY:
		r = is_carrier ? inst[5] & 0xF : inst[4] & 0xF;
		rks = r * 4 + kb;
		rh = rks >> 2;
		if (rh > 15)
			rh = 15;
		rl = rks & 3;
		egc[slot] += ((rl + 4) << (rh - 1));
		sustain_level = is_carrier ? inst[7] >> 4 : inst[6] >> 4;
		sustain_level = (db(3) * sustain_level/* * (1 << 23) / 48*/);
		if (egc[slot] >= sustain_level)
		{
			egc[slot] = sustain_level;
			envelope_state[slot] = ENV_SUSTAIN;
		}
		return egc[slot];
		break;
	case ENV_SUSTAIN:
		//fix: percussive
		r = 0;

		if (is_carrier)
		{
			if (!(inst[1] & 0x20))
				r = inst[7] >> 4;
		}
		else if (!(inst[0] & 0x20))
		{
			r = inst[6] >> 4;
		}
		rks = r * 4 + kb;
		rh = rks >> 2;
		if (rh > 15)
			rh = 15;
		rl = rks & 3;
		egc[slot] += (rl + 4) << (rh - 1);
		if (egc[slot] >= (1 << 23))
		{
			egc[slot] = (1 << 23);
			envelope_state[slot] = ENV_IDLE;
		}
		return egc[slot];
		break;
	case ENV_RELEASE:
		if (channel_regs[slot >> 1][2] & 0x20)
		{
			r = 5;
		}
		else
		{
			r = 7;
		}
		rks = r * 4 + kb;
		rh = rks >> 2;
		if (rh > 15)
			rh = 15;
		rl = rks & 3;
		egc[slot] += (rl + 4) << (rh - 1);
		if (egc[slot] >= (1 << 23))
		{
			egc[slot] = (1 << 23);
			envelope_state[slot] = ENV_IDLE;
		}
		return (egc[slot]);
		break;
	default:
		return (1 << 23);
	}

	return (1 << 23);
}

void c_mapper85::key_on(int slot)
{
	egc[slot] = 0;
	phase[slot] = 0;
	envelope_state[slot] = ENV_ATTACK;
}

void c_mapper85::key_off(int slot)
{
	//if (envelope_state[slot] = ENV_ATTACK)
	//	egc[slot] = output[slot];
	envelope_state[slot] = ENV_RELEASE;
}

int c_mapper85::db(float db)
{
	return (int)(db * (float)(1 << 23) / 48.0f);
}

