module;

#include "d3d10.h"
#include "D3DX10.h"
module nemulator.menu;
import nemulator.buttons;
import input_handler;

extern int clientHeight;
extern int clientWidth;
extern ID3D10Device *d3dDev;
extern std::unique_ptr<c_input_handler> g_ih;

c_menu::c_menu()
{
    font = NULL;
    menu_items = NULL;
    selected_item = 0;
    y_offset = 0.0;
    text_height = .10;
    text_spacing = text_height + .05;
}

c_menu::~c_menu()
{
    for (int i = 0; i < menu_items->num_items; i++)
        delete[] menu_items->items[i];
    if (menu_items->items)
        delete[] menu_items->items;
    if (menu_items)
        delete menu_items;
    if (font)
    {
        font->Release();
        font = NULL;
    }
}

void c_menu::init(void *params)
{
    s_menu_items *passed_menu_items = (s_menu_items*)params;
    menu_items = new s_menu_items();
    menu_items->num_items = passed_menu_items->num_items;
    menu_items->items = new char*[menu_items->num_items];

    for (int i = 0; i < menu_items->num_items; i++)
    {
        int len = (int)strlen(passed_menu_items->items[i]);
        menu_items->items[i] = new char[len+1];
        strcpy(menu_items->items[i], passed_menu_items->items[i]);
    }
    load_fonts();
    y_offset = (1.0 - ((menu_items->num_items-.5) * text_spacing)) / 2;
}

void c_menu::draw()
{
    for (int i = 0; i < menu_items->num_items; i++)
    {
        draw_text(menu_items->items[i], 0.0, y_offset + text_spacing * i, i == selected_item ? D3DXCOLOR(1.0f, 0.0f, 0.0f, .8f) : D3DXCOLOR(1.0f, 1.0f, 1.0f, .8f));
    }
}

void c_menu::resize()
{
    load_fonts();
}

int c_menu::update(double dt, int child_result, void *params)
{
    if (g_ih->get_result(BUTTON_1DOWN, true) & c_input_handler::RESULT_DOWN ||
        g_ih->get_result(BUTTON_1SELECT, true) & c_input_handler::RESULT_DOWN)
    {
        selected_item = ++selected_item % menu_items->num_items;
    }
    else if (g_ih->get_result(BUTTON_1UP, true) & c_input_handler::RESULT_DOWN)
    {
        selected_item--;
        if (selected_item < 0)
            selected_item = menu_items->num_items - 1;
    }
    else if (g_ih->get_result(BUTTON_1A, true) & c_input_handler::RESULT_DOWN ||
        g_ih->get_result(BUTTON_1C, true) & c_input_handler::RESULT_DOWN ||
        g_ih->get_result(BUTTON_1START, true) & c_input_handler::RESULT_DOWN ||
        g_ih->get_result(BUTTON_RETURN, true) & c_input_handler::RESULT_DOWN)
    {
        *(int *)params = selected_item;
        dead = true;
        g_ih->ack();
        return c_task::TASK_RESULT_RETURN;
    }
    else if ((g_ih->get_result(BUTTON_1B, true) & c_input_handler::RESULT_DOWN) ||
        (g_ih->get_result(BUTTON_ESCAPE) & c_input_handler::RESULT_DOWN))
    {
        dead = true;
        g_ih->ack();
        return c_task::TASK_RESULT_CANCEL;
    }
    g_ih->ack();
    return c_task::TASK_RESULT_NONE;
}

void c_menu::draw_text(char *text, double x, double y, D3DXCOLOR color)
{
    RECT r = {0, (long)(clientHeight * y), clientWidth, clientHeight};
    ID3D10DepthStencilState *state;
    int oldref;
    d3dDev->OMGetDepthStencilState(&state, (UINT *)&oldref);
    font->DrawText(NULL, text, -1, &r, DT_NOCLIP | DT_CENTER, color);
    d3dDev->OMSetDepthStencilState(state, oldref);
}

void c_menu::load_fonts()
{
    if (font)
    {
        font->Release();
        font = NULL;
    }
    D3DX10_FONT_DESC fontDesc;
    fontDesc.Height = (int)(clientHeight * text_height);
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