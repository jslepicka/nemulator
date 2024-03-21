#include "input_handler.h"
#include "windows.h"

c_input_handler::c_input_handler(int buttons)
{
	num_buttons = buttons;
	state = std::make_unique<s_state[]>(num_buttons);
	s_state *s = state.get();
	for (int i = 0; i < num_buttons; i++)
	{
		s->type = 0;
		s->state_cur = 0;
		s->state_prev = 0;
		s->repeat_count = 0;
		s->state_result = 0;
		s->repeat_enable = 0;
		s->button_key = 0;
		s->hold_time = 0.0;
		s->repeat_time = repeat_start;
		s->joy = -1;
		s->joy_button = 0;
		s->ack = 0;
		s->depressed_count = 0;
		s++;
	}
	repeat_start = 333.3;
	repeat_rate_slow = 333.3;
	repeat_rate_fast = 75.0;
	repeat_rate_extrafast = 33.3;
	for (int i = 0; i < 8; i++)
	{
		memset(&joyInfoEx[i], 0, sizeof(joyInfoEx[i]));
		joyInfoEx[i].dwSize = sizeof(joyInfoEx[i]);
		joyInfoEx[i].dwFlags = JOY_RETURNBUTTONS | JOY_RETURNPOVCTS | JOY_RETURNCENTERED | JOY_RETURNX | JOY_RETURNY | JOY_RETURNZ;
		joy_poll_result[i] = -1;
		joy_suppressed[i] = 0;
	}
	joymask = 0;
	ackd = false;
	extrafast_enabled = 1;
}

c_input_handler::~c_input_handler()
{
}

double c_input_handler::get_hold_time(int button)
{
	 if (ackd) return 0.0; return state[button].ack ? 0.0 : state[button].hold_time; 
}

void c_input_handler::set_repeat_mode(int button, int mode)
{
	state[button].repeat_enable = mode;
}

void c_input_handler::set_button_keymap(int button, int key)
{
	state[button].button_key = key;
}

void c_input_handler::ack_button(int button)
{
	state[button].ack = 1;
}

int c_input_handler::get_key_state(int key)
{
	if (key == 0)
		return 0;
	else
		return GetAsyncKeyState(key) & 0x8000 ? 1 : 0;
}

int c_input_handler::get_joy_state(int joy, int joy_button)
{
	if (joy_poll_result[joy] == JOYERR_NOERROR)
		return joyInfoEx[joy].dwButtons & (1 << joy_button) ? 1: 0;
	else
		return 0;
}

int c_input_handler::get_joy_pov(int joy, int low, int high)
{					
	int pov = joyInfoEx[joy].dwPOV;
	if (joy_poll_result[joy] != JOYERR_NOERROR || (pov & 0xFFFF) == 0xFFFF)
		return 0;
	if (low < high)
	{
		if (pov >= low && pov <= high)
			return 1;
	}
	else
	{
		if (pov >= low || pov <= high)
			return 1;
	}
	return 0;
}

int c_input_handler::get_joy_axis(int joy, int axis)
{
	if (joy_poll_result[joy] != JOYERR_NOERROR)
		return 0;
	signed char threshold = (axis >> 8) & 0xFF;
	int temp = 0;
	int axis_value = 0;
	switch (axis & 0xFF)
	{
	case AXIS_X:
		axis_value = joyInfoEx[joy].dwXpos;
		break;
	case AXIS_Y:
		axis_value = joyInfoEx[joy].dwYpos;
		break;
	case AXIS_Z:
		axis_value = joyInfoEx[joy].dwZpos;
		break;
	default:
		return 0;
	}
	int center_point = axis_value > 32767 ? 32768 : 32767;
	double axis_percent = ((double)(axis_value - center_point) / 32767.0) * 100.0;
	if (threshold > 0 && axis_percent >= threshold)
		return 1;
	else if (threshold < 0 && axis_percent <= threshold)
		return 1;
	else
		return 0;
}

void c_input_handler::ack()
{
	ackd = true;
}

int c_input_handler::get_result(int button, bool ack)
{
	if (ackd)
		return 0;
	int result = state[button].ack ? 0 : state[button].state_result;
	if (result && ack)
		state[button].ack = 1;
	return result;
}

void c_input_handler::set_button_joymap(int button, int joy, int joy_button)
{
	if (joy == -1 || joy_button == -1) return;
	int result = joyGetPosEx(joy, &joyInfoEx[joy]);
	if (result != JOYERR_NOERROR)
		joy_suppressed[joy] = JOY_SUPPRESSED_TIME;
	joymask |= (1 << joy);
	state[button].joy = joy;
	state[button].joy_button = joy_button; 
}

void c_input_handler::set_button_type(int button, int type)
{
	state[button].type = type;
}

void c_input_handler::poll(double dt, int ignore_input)
{
	//poll joysticks
	ackd = false;
	if (joymask)
	{
		for (int i = 0; i < 8; i++) {
			if (joy_suppressed[i] > 0.0) {
				joy_suppressed[i] -= dt;
			}
			else if (joymask & (1 << i)) {
				joy_poll_result[i] = joyGetPosEx(i, &joyInfoEx[i]);
				if (joy_poll_result[i] != JOYERR_NOERROR) {
					joy_suppressed[i] = JOY_SUPPRESSED_TIME;
				}
			}
		}
	}

	s_state *s = state.get();
	unsigned char result = 0;
	for (int i = 0; i < num_buttons; i++)
	{
		s->state_prev = s->state_cur;
		if (ignore_input) {
			s->state_cur = 0;
		}
		else {
			s->state_cur = get_key_state(s->button_key);
			if (s->joy >= 0)
			{
				switch (s->type)
				{
				case TYPE_BUTTON:
					s->state_cur |= get_joy_state(s->joy, s->joy_button);
					break;
				case TYPE_AXIS:
					s->state_cur |= get_joy_axis(s->joy, s->joy_button);
					break;
				case TYPE_POV:
					s->state_cur |= get_joy_pov(s->joy, s->joy_button & 0xFFFF, (s->joy_button >> 16) & 0xFFFF);
					break;
				}
			}
		}

		s->state_result = RESULT_NONE;
		if (s->state_cur != s->state_prev) //state has changed
		{
			s->ack = 0;
			if (s->state_cur) //key depressed
				s->state_result |= RESULT_DOWN;
			else
			{
				s->state_result |= RESULT_UP;
			}

			s->hold_time = 0.0;
			s->repeat_count = 0;
			s->repeat_time = repeat_start;
			s->depressed_count = 0;
		}
		if (s->state_cur & s->state_prev) //if key is held down
		{
			s->state_result |= RESULT_DEPRESSED;
			s->hold_time += dt;
			s->depressed_count++;
			if (s->repeat_enable)
			{
				if (s->hold_time >= s->repeat_time)
				{
					s->state_result |= RESULT_REPEAT;
					s->ack = 0;
					s->repeat_count++;
					if (s->repeat_count > 16 && extrafast_enabled)
					{
						s->repeat_time += repeat_rate_extrafast;
						s->state_result |= RESULT_REPEAT_EXTRAFAST;
					}
					else if (s->repeat_count > 1)
					{
						s->repeat_time += repeat_rate_fast;
						s->state_result |= RESULT_REPEAT_FAST;
					}
					else
					{
						s->repeat_time += repeat_rate_slow;
						s->state_result |= RESULT_REPEAT_SLOW;
					}
				}
			}
		}
		s++;
	}
}
