module;
#include "d3d10.h"
#include "D3DX10.h"

#define ReleaseCOM(x) { if(x) {x->Release(); x = 0; } }


module nemulator:qam;

import nemulator.buttons;
import input_handler;

extern ID3D10Device *d3dDev;
extern int clientHeight;
extern int clientWidth;
extern std::unique_ptr<c_input_handler> g_ih;

//const char *c_qam::c = "#abcdefghijklmnopqrstuvwxyz";

const char *c_qam::c = "#ABCDEFGHIJKLMNOPQRSTUVWXYZ";
const double c_qam::scroll_delay = 100.0;
//row heights are a fraction of the client height
const double c_qam::system_row_height = .08;
const double c_qam::char_row_height = .1;
//time constant (ms) for easing the system row; it covers ~95% of the distance in 3x this time
const double c_qam::system_scroll_time = 40.0;

c_qam::c_qam()
{
    selected = 0;
    scroll_timer = 0.0;
    state = STATE_IDLE;
    scroll_pos = 0.0;
    result = RESULT_CANCEL;
    font = 0;
    system_font = 0;
    row = ROW_CHAR;
    selected_system = 0;
    active_system = 0;
    system_scroll = 0.0;
}

c_qam::~c_qam()
{
    ReleaseCOM(font);
    ReleaseCOM(system_font);
}

void c_qam::activate()
{
    state = STATE_ACTIVATED;
    row = ROW_CHAR;
    result = RESULT_CANCEL;
}
void c_qam::set_valid_chars(int *v)
{
    valid_chars = v;
}

void c_qam::set_systems(const std::vector<std::string> &system_names, int active)
{
    systems = system_names;
    selected_system = active;
    active_system = active;
    system_scroll = active;
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
    struct s_fonts {
        ID3DX10Font **font;
        double scale;
    };

    s_fonts fonts[] = {
        {&font, .075},
        {&system_font, .045}
    };

    D3DX10_FONT_DESC fontDesc;
    for (auto f : fonts) {
        ReleaseCOM((*f.font));
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
        HRESULT hr = D3DX10CreateFontIndirect(d3dDev, &fontDesc, f.font);
    }
}

void c_qam::resize()
{
    load_fonts();
    scroll_target = (int)(clientHeight * (system_row_height + char_row_height));
    if (state == STATE_READY)
        scroll_pos = scroll_target;
}

int c_qam::update(double dt, int child_result, void *params)
{
    //easing (rather than a fixed-duration interpolation) keeps the scroll smooth when the
    //target changes mid-scroll, e.g., while left/right is repeating
    system_scroll += (selected_system - system_scroll) * (1.0 - std::exp(-dt / system_scroll_time));
    if (std::abs(selected_system - system_scroll) < .001)
        system_scroll = selected_system;

    if (state == STATE_ACTIVATED)
    {
        scroll_timer = 0.0;
        state = STATE_SCROLL_IN;
        g_ih->disable_extrafast();
    }
    else if (state == STATE_READY)
    {
        int result_mask = c_input_handler::RESULT_DOWN | c_input_handler::RESULT_REPEAT_SLOW | c_input_handler::RESULT_REPEAT_FAST /*| c_input_handler::RESULT_REPEAT_EXTRAFAST*/;
        int num_systems = (int)systems.size();

        if (g_ih->get_result(BUTTON_RIGHT, true) & result_mask)
        {
            if (row == ROW_SYSTEM)
            {
                selected_system = (selected_system + 1) % num_systems;
            }
            else
            {
                do
                {
                    selected = ++selected % 27;
                } while (valid_chars[selected] == 0);
                //selected = ++selected % 27;
            }
        }
        else if (g_ih->get_result(BUTTON_LEFT, true) & result_mask)
        {
            if (row == ROW_SYSTEM)
            {
                selected_system = (selected_system + num_systems - 1) % num_systems;
            }
            else
            {
                do
                {
                selected--;
                if (selected < 0)
                    selected = 26;
                } while (valid_chars[selected] == 0);
                //selected = --selected % 27;
            }
        }
        else if (row == ROW_CHAR && (g_ih->get_result(BUTTON_UP, true) & c_input_handler::RESULT_DOWN))
        {
            row = ROW_SYSTEM;
        }
        else if (row == ROW_SYSTEM && (g_ih->get_result(BUTTON_DOWN, true) & c_input_handler::RESULT_DOWN))
        {
            //leaving the system row without choosing a system keeps the current filter
            selected_system = active_system;
            row = ROW_CHAR;
        }
        else if ((g_ih->get_result(BUTTON_CANCEL, true) & c_input_handler::RESULT_DOWN) ||
                 (g_ih->get_result(BUTTON_DOWN, true) & c_input_handler::RESULT_DOWN) ||
                 (g_ih->get_result(BUTTON_1SELECT, true) & c_input_handler::RESULT_DOWN))
        {
            state = STATE_SCROLL_OUT;
            scroll_timer = 0.0;
            result = RESULT_CANCEL;
            //g_ih->ack();
            //return c_task::TASK_RESULT_CANCEL;
        }
        else if ((g_ih->get_result(BUTTON_OK, true) & c_input_handler::RESULT_DOWN))
        {
            //*(char *)params = c[selected];
            result = row == ROW_SYSTEM ? RESULT_SYSTEM : RESULT_CHAR;
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
                if (result == RESULT_CANCEL)
                    return c_task::TASK_RESULT_CANCEL;
                if (result == RESULT_CHAR)
                    *(char*)params = c[selected];
                return c_task::TASK_RESULT_RETURN;
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
    ID3D10DepthStencilState *state;
    int oldref;
    d3dDev->OMGetDepthStencilState(&state, (UINT *)&oldref);

    const D3DXCOLOR highlight(1.0f, 0.0f, 0.0f, 1.0f);
    const D3DXCOLOR normal(1.0f, 1.0f, 1.0f, 1.0f);
    const D3DXCOLOR unfocused_highlight(.46f, 0.0f, 0.0f, 1.0f);
    const D3DXCOLOR unfocused_normal(.46f, .46f, .46f, 1.0f);
    const D3DXCOLOR invalid(.13f, .13f, .13f, 1.0f);

    long top = (long)scroll_pos - scroll_target;
    long system_row_bottom = top + (long)(clientHeight * system_row_height);

    //the system row keeps the selected system centered, scrolling as the selection changes
    if (!systems.empty())
    {
        int spacing = (int)(clientHeight * .04);
        std::vector<int> lefts; //left edge of each system, relative to the start of the row
        std::vector<int> widths;
        int row_width = 0;
        for (auto &s : systems)
        {
            RECT calc = {0, 0, 0, 0};
            system_font->DrawText(NULL, s.c_str(), -1, &calc, DT_CALCRECT | DT_SINGLELINE, normal);
            lefts.push_back(row_width);
            widths.push_back(calc.right - calc.left);
            row_width += widths.back() + spacing;
        }

        //while scrolling, system_scroll is between two systems, so center on a point between them
        int last = (int)systems.size() - 1;
        int from = std::clamp((int)std::floor(system_scroll), 0, last);
        int to = from < last ? from + 1 : last;
        double mu = std::clamp(system_scroll - from, 0.0, 1.0);
        double from_center = lefts[from] + widths[from] / 2.0;
        double to_center = lefts[to] + widths[to] / 2.0;
        int x = (int)(clientWidth / 2.0 - (from_center + (to_center - from_center) * mu));

        for (int i = 0; i <= last; i++)
        {
            RECT r = {x + lefts[i], top, x + lefts[i] + widths[i], system_row_bottom};
            D3DXCOLOR color = row == ROW_SYSTEM ? (i == selected_system ? highlight : normal)
                                                : (i == selected_system ? unfocused_highlight : unfocused_normal);
            system_font->DrawText(NULL, systems[i].c_str(), -1, &r, DT_NOCLIP | DT_LEFT | DT_SINGLELINE | DT_VCENTER, color);
        }
    }

    //RECT r = {(LONG)(clientWidth * .1), (LONG)(clientHeight * .1), 0, 0};
    RECT r = {0, system_row_bottom, clientWidth, (long)scroll_pos};

    //char *c = "0ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    char j[2];

    j[1] = 0;

    for (int i = 0; i < 27; i++)
    {
        j[0] = c[i];
        r.left = (LONG)((clientWidth / 29.0) * (i + 1));
        r.right = (LONG)((clientWidth / 29.0) * (i + 2));
        D3DXCOLOR color;
        if (i == selected)
            color = row == ROW_CHAR ? highlight : unfocused_highlight;
        else if (!valid_chars[i])
            color = invalid;
        else
            color = row == ROW_CHAR ? normal : unfocused_normal;
        font->DrawText(NULL, j, -1, &r, DT_NOCLIP | DT_CENTER | DT_SINGLELINE | DT_VCENTER, color);
    }
    d3dDev->OMSetDepthStencilState(state, oldref);
}
