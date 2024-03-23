#include "task1.h"
#include "Nemulator.h"

extern ID3D10Device *d3dDev;
extern int clientHeight;
extern int clientWidth;
extern std::unique_ptr<c_input_handler> g_ih;

c_task1::c_task1()
{
}

c_task1::~c_task1()
{
}

void c_task1::init(void *params)
{
	D3DX10_FONT_DESC fontDesc;
	fontDesc.Height = (int)(clientHeight * .25);
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
	elapsed = 0.0;
	counter = 0;
}

int c_task1::update(double dt, int child_result, void *params)
{
	char x[256];
	sprintf_s(x, "%d", counter++);
	s = x;
	if (counter == 100)
	{
		add_task((c_task*)n, NULL);
		dead = true;
		return 1;
	}
	//g_ih->ack();
	return 0;
}

void c_task1::draw()
{
	RECT r = {(LONG)(clientWidth * (.1 * xx)), (LONG)(clientHeight * .1), 0, 0};
	ID3D10DepthStencilState *state;
	int oldref;
	d3dDev->OMGetDepthStencilState(&state, (UINT *)&oldref);
	font->DrawText(NULL, s.c_str(), -1, &r, DT_NOCLIP, D3DXCOLOR(1.0f, 0.0f, 0.0f, 1.0f));
	d3dDev->OMSetDepthStencilState(state, oldref);
}