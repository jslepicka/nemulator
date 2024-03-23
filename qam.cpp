#include "qam.h"
#include "Nemulator.h"

extern ID3D10Device *d3dDev;
extern int clientHeight;
extern int clientWidth;
extern std::unique_ptr<c_input_handler> g_ih;

//const char *c_qam::c = "#abcdefghijklmnopqrstuvwxyz";

const char *c_qam::c = "#ABCDEFGHIJKLMNOPQRSTUVWXYZ";
const double c_qam::scroll_delay = 100.0;

c_qam::c_qam()
{
	selected = 0;
	scroll_timer = 0.0;
	state = STATE_IDLE;
	scroll_pos = 0.0;
	result = 0;
	font = 0;
}

c_qam::~c_qam()
{
}

void c_qam::activate()
{
	state = STATE_ACTIVATED;
}
void c_qam::set_valid_chars(int *v)
{
	valid_chars = v;
}

void c_qam::init(void *params)
{
	resize();
}

void c_qam::set_char(char c)
{
	c = toupper(c);
	if (c < 'A')
		c = '0';
	else if (c > 'Z')
		c = 'Z';
	selected = c == '0' ? 0 : c - 64;
}

void c_qam::load_fonts()
{
	ReleaseCOM(font);
	D3DX10_FONT_DESC fontDesc;
	fontDesc.Height = (int)(clientHeight * .075);
	fontDesc.Width = 0;
	fontDesc.Weight = 0;
	fontDesc.MipLevels = 1;
	fontDesc.Italic = false;
	fontDesc.CharSet = DEFAULT_CHARSET;
	fontDesc.OutputPrecision = OUT_DEFAULT_PRECIS;
	fontDesc.Quality = DEFAULT_QUALITY;
	fontDesc.PitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
	strcpy_s(fontDesc.FaceName, "Calibri");
	HRESULT hr = D3DX10CreateFontIndirect(d3dDev, &fontDesc, &font);
}

void c_qam::resize()
{
	load_fonts();
	scroll_target = (int)(clientHeight * .1);
	if (state == STATE_READY)
		scroll_pos = scroll_target;
}

int c_qam::update(double dt, int child_result, void *params)
{
	if (state == STATE_ACTIVATED)
	{
		scroll_timer = 0.0;
		state = STATE_SCROLL_IN;
		g_ih->disable_extrafast();
	}
	else if (state == STATE_READY)
	{
		int result_mask = c_input_handler::RESULT_DOWN | c_input_handler::RESULT_REPEAT_SLOW | c_input_handler::RESULT_REPEAT_FAST /*| c_input_handler::RESULT_REPEAT_EXTRAFAST*/;

		if (g_ih->get_result(BUTTON_1RIGHT, true) & result_mask)
		{
			do
			{
				selected = ++selected % 27;
			} while (valid_chars[selected] == 0);
			//selected = ++selected % 27;
		}
		else if (g_ih->get_result(BUTTON_1LEFT, true) & result_mask)
		{
			do
			{
			selected--;
			if (selected < 0)
				selected = 26;
			} while (valid_chars[selected] == 0);
			//selected = --selected % 27;
		}
		else if ((g_ih->get_result(BUTTON_1SELECT, true) & c_input_handler::RESULT_DOWN) ||
			(g_ih->get_result(BUTTON_1B, true) & c_input_handler::RESULT_DOWN) ||
			(g_ih->get_result(BUTTON_1DOWN, true) & c_input_handler::RESULT_DOWN) ||
			(g_ih->get_result(BUTTON_ESCAPE, true) & c_input_handler::RESULT_DOWN))
		{
			state = STATE_SCROLL_OUT;
			scroll_timer = 0.0;
			result = -1;
			//g_ih->ack();
			//return c_task::TASK_RESULT_CANCEL;
		}
		else if ((g_ih->get_result(BUTTON_1START, true) & c_input_handler::RESULT_DOWN) ||
			(g_ih->get_result(BUTTON_1A, true) & c_input_handler::RESULT_DOWN) ||
			(g_ih->get_result(BUTTON_RETURN, true) & c_input_handler::RESULT_DOWN))
		{
			//*(char *)params = c[selected];
			result = c[selected];
			state = STATE_SCROLL_OUT;
			scroll_timer = 0.0;
			//g_ih->ack();
			//return c_task::TASK_RESULT_RETURN;
		}
	}
	else if (state == STATE_IDLE)
	{
		g_ih->enable_extrafast();
		return child_result;
	}
	else
	{
		scroll_timer += dt;
		if (state == STATE_SCROLL_IN)
		{
			if (scroll_timer >= scroll_delay)
			{
				state = STATE_READY;
				scroll_timer = scroll_delay;
				scroll_pos = scroll_target;
			}
			scroll_pos = (scroll_timer/scroll_delay) * scroll_target;
		}
		else if (state == STATE_SCROLL_OUT)
		{
			if (scroll_timer >= scroll_delay)
			{
				//dead = true;
				state = STATE_IDLE;
				scroll_timer = scroll_delay;
				g_ih->ack();
				if (result > 0)
				{
					*(char*)params = result;
					return c_task::TASK_RESULT_RETURN;
				}
				else
					return c_task::TASK_RESULT_CANCEL;
			}
			scroll_pos = scroll_target - ((scroll_timer / scroll_delay) * scroll_target);
		}
	}
	g_ih->ack();
	return child_result;
}

void c_qam::draw()
{
	if (state == STATE_IDLE)
		return;
	//RECT r = {(LONG)(clientWidth * .1), (LONG)(clientHeight * .1), 0, 0};
	RECT r = {0, (long)scroll_pos - scroll_target, clientWidth, (long)scroll_pos};
	ID3D10DepthStencilState *state;
	int oldref;
	d3dDev->OMGetDepthStencilState(&state, (UINT *)&oldref);

	//char *c = "0ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	char j[2];
	j[1] = 0;

	for (int i = 0; i < 27; i++)
	{
		j[0] = c[i];
		r.left = (LONG)((clientWidth / 29.0) * (i + 1));
		r.right = (LONG)((clientWidth / 29.0) * (i + 2));
		D3DXCOLOR color;
		font->DrawText(NULL, j, -1, &r, DT_NOCLIP | DT_CENTER | DT_SINGLELINE | DT_VCENTER, 
			i == selected ? D3DXCOLOR(1.0f, 0.0f, 0.0f, 1.0f) : valid_chars[i] ? D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f) : D3DXCOLOR(.13f, .13f, .13f, 1.0f));
	}
	d3dDev->OMSetDepthStencilState(state, oldref);
}