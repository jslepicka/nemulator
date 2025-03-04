module;
#include "d3d10.h"
#include "D3DX10.h"
#include <deque>
#include <string>
module nemulator.status;

extern int clientHeight;
extern int clientWidth;
extern ID3D10Device *d3dDev;

c_status::c_status()
{
    font = NULL;
}

c_status::~c_status()
{
    for (std::deque<s_message*>::iterator i = messages.begin(); i != messages.end(); )
    {
        s_message *m = (s_message*)(*i);
        delete m;
        i = messages.erase(i);
    }


    if (font)
    {
        font->Release();
        font = NULL;
    }
}

void c_status::load_fonts()
{
    if (font)
    {
        font->Release();
        font = 0;
    }
    D3DX10_FONT_DESC fontDesc;
    fontDesc.Height = (int)(clientHeight * .05);
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

void c_status::init(void *params)
{
    load_fonts();
}


void c_status::resize()
{
    load_fonts();
}

int c_status::update(double dt, int child_result, void *params)
{
    for (std::deque<s_message*>::iterator i = messages.begin(); i != messages.end(); )
    {
        s_message *m = (s_message*)(*i);
        m->timer -= dt;
        if (m->timer <= 0.0)
        {
            delete m;
            i = messages.erase(i);
        }
        else
            ++i;
    }
    return child_result;
}

void c_status::add_message(std::string message)
{
    s_message *m = new s_message();
    m->message = message;
    m->timer = 2000.0;
    messages.push_front(m);
}

void c_status::draw()
{
    int message_count = 0;
    for (auto &m : messages)
    {
        //s_message *m = (s_message*)(*i);
        double alpha = 1.0;
        if (m->timer <= 1000.0)
        {
            alpha = (m->timer / 1000.0) * 1.0;

        }
        draw_text(m->message.c_str(), .81, .90 - (message_count * .05), D3DXCOLOR(1.0f, 0.0f, 0.0f, (float)alpha));
        message_count++;
    }
    //draw_text("status", .1, .1, D3DXCOLOR(0.0f, 1.0f, 0.0f, 1.0f));
}

void c_status::draw_text(const char *text, double x, double y, D3DXCOLOR color)
{
    RECT r = {(long)(clientWidth * x), (long)(clientHeight * y), (long)(clientWidth * .99), clientHeight};
    ID3D10DepthStencilState *state;
    int oldref;
    d3dDev->OMGetDepthStencilState(&state, (UINT *)&oldref);
    font->DrawText(NULL, text, -1, &r, DT_NOCLIP | DT_RIGHT, color);
    d3dDev->OMSetDepthStencilState(state, oldref);
}
    