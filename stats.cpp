module;
#include "d3d11.h"
#include <cstdio>
module nemulator:stats;

extern int clientHeight;
extern int clientWidth;
extern ID3D11Device *d3dDev;
extern ID3D11DeviceContext *d3dContext;

c_stats::c_stats()
{
    font = NULL;
    reported = false;
}

c_stats::~c_stats()
{
    delete font;
}

void c_stats::load_fonts()
{
    delete font;
    font = new c_font((int)(clientHeight * .05), "Calibri");

}

void c_stats::init(void *params)
{
    load_fonts();
}

void c_stats::resize()
{
    load_fonts();
}

int c_stats::update(double dr, int child_result, void *params)
{
    return child_result;
}

void c_stats::report_stat(std::string stat_name, int stat_value)
{
    char temp[32];
    sprintf_s(temp, 32, "%d", stat_value);
    report_stat(stat_name, temp);
}

void c_stats::report_stat(std::string stat_name, double stat_value)
{
    char temp[32];
    sprintf_s(temp, 32, "%.2f", stat_value);
    report_stat(stat_name, temp);
}

void c_stats::report_stat(std::string stat_name, uint64_t stat_value)
{
    char temp[32];
    sprintf_s(temp, 32, "%llu", stat_value);
    report_stat(stat_name, temp);
}

void c_stats::report_stat(std::string stat_name, std::string stat_value)
{
    stats.erase(stat_name);
    stats.insert(std::pair<std::string, std::string>(stat_name, stat_value));
}

void c_stats::clear()
{
    stats.clear();
}

void c_stats::draw()
{
    double y = .025;
    //for (std::map<std::string, std::string>::iterator i = stats.begin();  i != stats.end(); ++i)
    //{
    //    std::pair<std::string, std::string> p = *i;
    //    std::string s(p.first + ": " + p.second);
    //    draw_text((char*)s.c_str(), .02, y, s_color(1.0f, 1.0f, 1.0f, 1.0f));
    //    y += .05;
    //}

    for (auto &stat : stats)
    {
        std::pair<std::string, std::string> p = stat;
        std::string s(p.first + ": " + p.second);
        draw_text((char*)s.c_str(), .02, y, s_color(1.0f, 1.0f, 1.0f, 1.0f));
        y += .05;
    }

    //draw_text("test", .1, .1, s_color(0.0f, 1.0f, 0.0f, 1.0f));
}

void c_stats::draw_text(char *text, double x, double y, s_color color)
{
    RECT r = {(long)(clientWidth * x), (long)(clientHeight * y), clientWidth, clientHeight};
    ID3D11DepthStencilState *state;
    int oldref;
    d3dContext->OMGetDepthStencilState(&state, (UINT *)&oldref);
    font->draw(text, r, 0, color);
    d3dContext->OMSetDepthStencilState(state, oldref);
}
    