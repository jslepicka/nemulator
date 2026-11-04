module;
#include "d3d11.h"

module nemulator:status;

extern int clientHeight;
extern int clientWidth;
extern ID3D11Device *d3dDev;
extern ID3D11DeviceContext *d3dContext;

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


    delete font;
}

void c_status::load_fonts()
{
    delete font;
    font = new c_font((int)(clientHeight * .05), "Calibri");

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
        draw_text(m->message.c_str(), .81, .90 - (message_count * .05), s_color(1.0f, 0.0f, 0.0f, (float)alpha));
        message_count++;
    }
    //draw_text("status", .1, .1, s_color(0.0f, 1.0f, 0.0f, 1.0f));
}

void c_status::draw_text(const char *text, double x, double y, s_color color)
{
    RECT r = {(long)(clientWidth * x), (long)(clientHeight * y), (long)(clientWidth * .99), clientHeight};
    ID3D11DepthStencilState *state;
    int oldref;
    d3dContext->OMGetDepthStencilState(&state, (UINT *)&oldref);
    font->draw(text, r, DT_RIGHT, color);
    d3dContext->OMSetDepthStencilState(state, oldref);
}
    