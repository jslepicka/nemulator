#include "nes_input_handler.h"

#include <crtdbg.h>
#if defined(DEBUG) | defined(_DEBUG)
#define DEBUG_NEW new(_CLIENT_BLOCK, __FILE__, __LINE__)
#define new DEBUG_NEW
#endif

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
	delete[] turbo_rate;
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

int c_nes_input_handler::get_sms_input()
{
	struct s_keymap
	{
		int button;
		int shift;
	};
	
	s_keymap keymap[] =
	{
		{ BUTTON_1B,     4  }, //button 1
		{ BUTTON_1A,     5  }, //button 2
		{ BUTTON_1UP,    0  },
		{ BUTTON_1DOWN,  1  },
		{ BUTTON_1LEFT,  2  },
		{ BUTTON_1RIGHT, 3  },
		{ BUTTON_2B,     10 }, //button 1
		{ BUTTON_2A,     11 }, //button 2
		{ BUTTON_2UP,    6  },
		{ BUTTON_2DOWN,  7  },
		{ BUTTON_2LEFT,  8  },
		{ BUTTON_2RIGHT, 9  },
		{ BUTTON_SMS_PAUSE, 31}
	};
		
	uint32_t ret = 0;
	for (int i = 0; i < sizeof(keymap) / sizeof(s_keymap); i++)
	{
		s_keymap k = keymap[i];
		ret |= state[k.button].ack ? 0 : (state[k.button].state_cur << k.shift);
	}
	return ret;
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