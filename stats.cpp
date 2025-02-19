#include "stats.h"

extern int clientHeight;
extern int clientWidth;
extern ID3D10Device *d3dDev;

c_stats::c_stats()
{
    font = NULL;
    reported = false;
}

c_stats::~c_stats()
{
    if (font)
    {
        font->Release();
        font = NULL;
    }
}

void c_stats::load_fonts()
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
    //    draw_text((char*)s.c_str(), .02, y, D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f));
    //    y += .05;
    //}

    for (auto &stat : stats)
    {
        std::pair<std::string, std::string> p = stat;
        std::string s(p.first + ": " + p.second);
        draw_text((char*)s.c_str(), .02, y, D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f));
        y += .05;
    }

    //draw_text("test", .1, .1, D3DXCOLOR(0.0f, 1.0f, 0.0f, 1.0f));
}

void c_stats::draw_text(char *text, double x, double y, D3DXCOLOR color)
{
    RECT r = {(long)(clientWidth * x), (long)(clientHeight * y), clientWidth, clientHeight};
    ID3D10DepthStencilState *state;
    int oldref;
    d3dDev->OMGetDepthStencilState(&state, (UINT *)&oldref);
    font->DrawText(NULL, text, -1, &r, DT_NOCLIP, color);
    d3dDev->OMSetDepthStencilState(state, oldref);
}
    