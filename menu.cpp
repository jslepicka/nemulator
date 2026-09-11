module;

#include "d3d10.h"
#include "D3DX10.h"

#define ReleaseCOM(x) { if(x) {x->Release(); x = 0; } }

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
    if (g_ih->get_result(BUTTON_DOWN, true) & c_input_handler::RESULT_DOWN ||
        g_ih->get_result(BUTTON_1SELECT, true) & c_input_handler::RESULT_DOWN)
    {
        selected_item = ++selected_item % menu_items->num_items;
    }
    else if (g_ih->get_result(BUTTON_UP, true) & c_input_handler::RESULT_DOWN)
    {
        selected_item--;
        if (selected_item < 0)
            selected_item = menu_items->num_items - 1;
    }
    else if (g_ih->get_result(BUTTON_OK, true) & c_input_handler::RESULT_DOWN)
    {
        *(int *)params = selected_item;
        dead = true;
        g_ih->ack();
        return c_task::TASK_RESULT_RETURN;
    }
    else if ((g_ih->get_result(BUTTON_CANCEL, true) & c_input_handler::RESULT_DOWN))
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

c_options_menu::~c_options_menu()
{
    ReleaseCOM(title_font);
    ReleaseCOM(font);
    ReleaseCOM(hint_font);
}

void c_options_menu::init(void *params)
{
    auto p = (s_params *)params;
    title = p->title;
    items = p->items;
    shadow = p->shadow;
    load_fonts();
}

void c_options_menu::resize()
{
    load_fonts();
}

int c_options_menu::update(double dt, int child_result, void *params)
{
    auto &item = items[selected_item];
    int num_items = (int)items.size();
    int adjust_mask = c_input_handler::RESULT_DOWN | (item.repeat ? c_input_handler::RESULT_REPEAT : 0);

    if (g_ih->get_result(BUTTON_DOWN, true) & c_input_handler::RESULT_DOWN)
    {
        selected_item = (selected_item + 1) % num_items;
        confirm = false;
    }
    else if (g_ih->get_result(BUTTON_UP, true) & c_input_handler::RESULT_DOWN)
    {
        selected_item = (selected_item + num_items - 1) % num_items;
        confirm = false;
    }
    else if (g_ih->get_result(BUTTON_LEFT, true) & adjust_mask)
    {
        if (item.get_value)
            item.change(-1);
    }
    else if (g_ih->get_result(BUTTON_RIGHT, true) & adjust_mask)
    {
        if (item.get_value)
            item.change(1);
    }
    else if (g_ih->get_result(BUTTON_OK, true) & c_input_handler::RESULT_DOWN)
    {
        if (item.confirm_label.empty() || confirm)
        {
            item.change(0);
            confirm = false;
        }
        else
        {
            confirm = true;
        }
    }
    else if (g_ih->get_result(BUTTON_CANCEL, true) & c_input_handler::RESULT_DOWN)
    {
        dead = true;
        g_ih->ack();
        return c_task::TASK_RESULT_CANCEL;
    }
    g_ih->ack();
    return c_task::TASK_RESULT_NONE;
}

void c_options_menu::load_fonts()
{
    struct s_fonts
    {
        ID3DX10Font **font;
        double scale;
    };

    s_fonts fonts[] = {
        {&title_font, .08},
        {&font, .05},
        {&hint_font, .035},
    };

    for (auto f : fonts)
    {
        ReleaseCOM((*f.font));
        D3DX10_FONT_DESC fontDesc = {0};
        fontDesc.Height = (int)(clientHeight * f.scale);
        fontDesc.Width = 0;
        fontDesc.Weight = 0;
        fontDesc.MipLevels = 1;
        fontDesc.Italic = false;
        fontDesc.CharSet = DEFAULT_CHARSET;
        fontDesc.OutputPrecision = OUT_DEFAULT_PRECIS;
        fontDesc.Quality = DEFAULT_QUALITY;
        fontDesc.PitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
        strcpy_s(fontDesc.FaceName, "Calibri");
        D3DX10CreateFontIndirect(d3dDev, &fontDesc, f.font);
    }
}

void c_options_menu::draw_text(ID3DX10Font *f, const std::string &text, double left, double right, double y,
                               UINT format, D3DXCOLOR color)
{
    RECT r = {(LONG)(clientWidth * left), (LONG)(clientHeight * y), (LONG)(clientWidth * right), clientHeight};
    if (shadow)
    {
        LONG offset = clientHeight / 400 + 1;
        RECT shadow_rect = {r.left + offset, r.top + offset, r.right + offset, r.bottom + offset};
        f->DrawText(NULL, text.c_str(), -1, &shadow_rect, DT_NOCLIP | format, D3DXCOLOR(0.0f, 0.0f, 0.0f, color.a));
    }
    f->DrawText(NULL, text.c_str(), -1, &r, DT_NOCLIP | format, color);
}

void c_options_menu::draw()
{
    const D3DXCOLOR normal(1.0f, 1.0f, 1.0f, 1.0f);
    const D3DXCOLOR highlight(1.0f, 0.0f, 0.0f, 1.0f);
    const D3DXCOLOR hint(.6f, .6f, .6f, 1.0f);

    ID3D10DepthStencilState *depth_state;
    UINT oldref;
    d3dDev->OMGetDepthStencilState(&depth_state, &oldref);

    draw_text(title_font, title, 0.0, 1.0, TITLE_Y, DT_CENTER, normal);
    for (int i = 0; i < (int)items.size(); i++)
    {
        auto &item = items[i];
        double y = LIST_Y + ROW_HEIGHT * i;
        D3DXCOLOR color = i == selected_item ? highlight : normal;
        if (item.get_value)
        {
            //labels end just left of center and values start just right of it
            draw_text(font, item.label, 0.0, .47, y, DT_RIGHT, color);
            draw_text(font, item.get_value(), .53, 1.0, y, DT_LEFT, color);
        }
        else
        {
            bool confirming = confirm && i == selected_item;
            draw_text(font, confirming ? item.confirm_label : item.label, 0.0, 1.0, y, DT_CENTER, color);
        }
    }

    bool is_setting = (bool)items[selected_item].get_value;
    draw_text(hint_font, is_setting ? "Left/Right: change    Esc: back" : "Enter: select    Esc: back", 0.0, 1.0,
              HINT_Y, DT_CENTER, hint);

    d3dDev->OMSetDepthStencilState(depth_state, oldref);
    ReleaseCOM(depth_state);
}