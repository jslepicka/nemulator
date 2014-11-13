#include "nes_input_handler.h"

c_nes_input_handler::c_nes_input_handler(int buttons)
	: c_input_handler(buttons)
{
	for (int i = 0; i < 2; i++)
	{
		prev_temp[i] = 0;
		mask[i] = ~0x50;
	}
	turbo_state = new int[buttons];
	turbo_rate = new int[buttons];
	memset(turbo_state, 0, sizeof(int) * buttons);
	for (int i = 0; i < buttons; i++)
		turbo_rate[i] = 10;
}

c_nes_input_handler::~c_nes_input_handler()
{
	delete[] turbo_state;
}

void c_nes_input_handler::set_turbo_state(int button, int turbo_enabled)
{
	turbo_state[button % num_buttons] = turbo_enabled ? 1 : 0;
}

int c_nes_input_handler::get_turbo_state(int button)
{
	return turbo_state[button % num_buttons];
}

void c_nes_input_handler::set_turbo_rate(int button, int rate)
{
	if (rate > turbo_rate_max)
		rate = turbo_rate_max;
	if (rate < turbo_rate_min)
		rate = turbo_rate_min;
	turbo_rate[button % num_buttons] = rate;
}

int c_nes_input_handler::get_turbo_rate(int button)
{
	return turbo_rate[button % num_buttons];
}

unsigned char c_nes_input_handler::get_nes_byte(int controller)
{
	controller = controller % 2;

	int offset = controller * 8;
	unsigned char temp = 0;
	s_state *s = state + offset;
	for (int i = offset; i < (offset + 8); i++)
	{
		temp |= (s->ack ? 0 : (s->state_cur & (!(turbo_state[i] && s->depressed_count % turbo_rate[i] >= (turbo_rate[i]/2)))) << (i - offset));
		s++;
	}

	int changed = temp ^ prev_temp[controller];
	prev_temp[controller] = temp;
	int hidden = temp & ~mask[controller];
	
	if ((changed & 0xC0) && (hidden & 0xC0))
		mask[controller] ^= 0xC0;
	if ((changed & 0x30) && (hidden & 0x30))
		mask[controller] ^= 0x30;

	//ensure that left+right or up+down can't be pressed together
	//if ((temp & 0xC0) == 0xC0)
	//	temp = (temp & ~0xC0) | (prev_temp[controller] & 0xC0);
	//if ((temp & 0x30) == 0x30)
	//	temp = (temp & ~0x30) | (prev_temp[controller] & 0x30);

	
	return temp & mask[controller];
}