module;
#include "d3d10.h"
#include "D3DX10.h"
#include "shape.fxo.h"

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
//heights are a fraction of the client height.  the panel holds one row plus an arrow pointing at the other row
const double c_qam::row_height = .1;
const double c_qam::arrow_height = .04;
//time constant (ms) for easing the system row; it covers ~95% of the distance in 3x this time
const double c_qam::system_scroll_time = 40.0;
//time constant (ms) for sliding between the letters and the systems
const double c_qam::row_blend_time = 50.0;
//font heights, as a fraction of the client height
static constexpr double letter_font_height = .075;
static constexpr double system_font_height = .045;
//half the width of the arrow's square, as a fraction of the client height
static constexpr double arrow_size = .0125;
//how much of the space between an arrow and its row to close.  the arrow and the row are each centered
//in their own box, so the natural gap is the padding of both boxes
static constexpr double arrow_gap_closed = .5;

c_qam::c_qam()
{
    selected = 0;
    scroll_timer = 0.0;
    state = STATE_IDLE;
    scroll_pos = 0.0;
    result = RESULT_CANCEL;
    font = 0;
    system_font = 0;
    shape_effect = 0;
    triangle_technique = 0;
    shape_color = 0;
    shape_angle = 0;
    shape_layout = 0;
    shape_vertices = 0;
    row = ROW_CHAR;
    row_blend = 0.0;
    selected_system = 0;
    active_system = 0;
    system_row_left = 0;
    system_row_right = 0;
    system_scroll = 0.0;
    system_scroll_target = 0.0;
}

c_qam::~c_qam()
{
    ReleaseCOM(font);
    ReleaseCOM(system_font);
    ReleaseCOM(shape_vertices);
    ReleaseCOM(shape_layout);
    ReleaseCOM(shape_effect);
}

void c_qam::activate()
{
    state = STATE_ACTIVATED;
    row = ROW_CHAR;
    row_blend = 0.0;
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
    layout_systems();
}

//measures the system names and the edges of the letter row, then scrolls straight to the selected system
void c_qam::layout_systems()
{
    auto text_width = [](ID3DX10Font *f, const char *s) {
        RECT calc = {0, 0, 0, 0};
        f->DrawText(NULL, s, -1, &calc, DT_CALCRECT | DT_SINGLELINE, D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f));
        return (int)(calc.right - calc.left);
    };

    //letters are centered in their cells, so align with the outer edges of the first and last letters
    double cell_width = clientWidth / 29.0;
    system_row_left = (int)(cell_width + (cell_width - text_width(font, "#")) / 2.0);
    system_row_right = (int)(cell_width * 28 - (cell_width - text_width(font, "Z")) / 2.0);

    int spacing = (int)(clientHeight * .04);
    int x = 0;
    system_lefts.clear();
    system_widths.clear();
    for (auto &s : systems)
    {
        system_lefts.push_back(x);
        system_widths.push_back(text_width(system_font, s.c_str()));
        x += system_widths.back() + spacing;
    }

    system_scroll_target = 0.0;
    update_system_scroll_target();
    system_scroll = system_scroll_target;
}

//scrolls only as far as needed to keep the selected system within the row
void c_qam::update_system_scroll_target()
{
    if (systems.empty())
        return;
    int visible_width = system_row_right - system_row_left;
    int left = system_lefts[selected_system];
    int right = left + system_widths[selected_system];
    if (left < system_scroll_target)
        system_scroll_target = left;
    else if (right > system_scroll_target + visible_width)
        system_scroll_target = right - visible_width;
}

void c_qam::init(void *params)
{
    init_arrow();
    resize();
}

void c_qam::init_arrow()
{
    HRESULT hr = D3DX10CreateEffectFromMemory((LPCVOID)g_shape_effect, sizeof(g_shape_effect), "shape", NULL, NULL,
                                              "fx_4_0", 0, 0, d3dDev, NULL, NULL, &shape_effect, NULL, NULL);
    if (FAILED(hr))
        return;
    triangle_technique = shape_effect->GetTechniqueByName("Triangle");
    shape_color = shape_effect->GetVariableByName("shape_color")->AsVector();
    shape_angle = shape_effect->GetVariableByName("shape_angle")->AsScalar();

    D3D10_INPUT_ELEMENT_DESC layout[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D10_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D10_INPUT_PER_VERTEX_DATA, 0},
    };
    D3D10_PASS_DESC pass;
    triangle_technique->GetPassByIndex(0)->GetDesc(&pass);
    d3dDev->CreateInputLayout(layout, 2, pass.pIAInputSignature, pass.IAInputSignatureSize, &shape_layout);

    //rewritten each frame as the arrow moves and turns
    D3D10_BUFFER_DESC desc = {};
    desc.Usage = D3D10_USAGE_DYNAMIC;
    desc.ByteWidth = sizeof(s_shape_vertex) * 4;
    desc.BindFlags = D3D10_BIND_VERTEX_BUFFER;
    desc.CPUAccessFlags = D3D10_CPU_ACCESS_WRITE;
    d3dDev->CreateBuffer(&desc, NULL, &shape_vertices);
}

//draws an arrow pointing up, turned counterclockwise by angle (in radians) about its center.  the center
//and size (half the width of its square) are in pixels
void c_qam::draw_arrow(double center_x, double center_y, double size, double angle, D3DXCOLOR color)
{
    if (!shape_effect || !shape_layout || !shape_vertices)
        return;

    static const float corners[4][2] = {{-1.0f, -1.0f}, {-1.0f, 1.0f}, {1.0f, -1.0f}, {1.0f, 1.0f}};
    double sine = std::sin(angle);
    double cosine = std::cos(angle);
    s_shape_vertex *vertices;
    if (FAILED(shape_vertices->Map(D3D10_MAP_WRITE_DISCARD, 0, (void **)&vertices)))
        return;
    for (int i = 0; i < 4; i++)
    {
        double lx = corners[i][0];
        double ly = corners[i][1];
        //rotate in the shape's own space, where y points up, then flip to the screen, where it points down
        double px = center_x + (lx * cosine - ly * sine) * size;
        double py = center_y - (lx * sine + ly * cosine) * size;
        vertices[i] = {(float)(px / clientWidth * 2.0 - 1.0), (float)(1.0 - py / clientHeight * 2.0), 0.5f,
                       (float)lx, (float)ly};
    }
    shape_vertices->Unmap();

    //the effect sets blend, depth, and rasterizer state; put back whatever was there, since the game
    //panels don't set their own rasterizer state
    ID3D10BlendState *blend = NULL;
    FLOAT blend_factor[4];
    UINT sample_mask;
    d3dDev->OMGetBlendState(&blend, blend_factor, &sample_mask);
    ID3D10DepthStencilState *depth = NULL;
    UINT stencil_ref;
    d3dDev->OMGetDepthStencilState(&depth, &stencil_ref);
    ID3D10RasterizerState *raster = NULL;
    d3dDev->RSGetState(&raster);

    UINT stride = sizeof(s_shape_vertex);
    UINT offset = 0;
    d3dDev->IASetInputLayout(shape_layout);
    d3dDev->IASetVertexBuffers(0, 1, &shape_vertices, &stride, &offset);
    d3dDev->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    shape_color->SetFloatVector((float *)color);
    shape_angle->SetFloat((float)angle);
    triangle_technique->GetPassByIndex(0)->Apply(0);
    d3dDev->Draw(4, 0);

    d3dDev->OMSetBlendState(blend, blend_factor, sample_mask);
    d3dDev->OMSetDepthStencilState(depth, stencil_ref);
    d3dDev->RSSetState(raster);
    ReleaseCOM(blend);
    ReleaseCOM(depth);
    ReleaseCOM(raster);
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
        const char *face;
    };

    s_fonts fonts[] = {
        {&font, letter_font_height, "Calibri"},
        {&system_font, system_font_height, "Calibri"},
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
        strcpy_s(fontDesc.FaceName, f.face);
        HRESULT hr = D3DX10CreateFontIndirect(d3dDev, &fontDesc, f.font);
    }
}

void c_qam::resize()
{
    load_fonts();
    layout_systems();
    scroll_target = (int)(clientHeight * (row_height + arrow_height));
    if (state == STATE_READY)
        scroll_pos = scroll_target;
}

int c_qam::update(double dt, int child_result, void *params)
{
    //easing (rather than a fixed-duration interpolation) keeps the scroll smooth when the
    //target changes mid-scroll, e.g., while left/right is repeating
    update_system_scroll_target();
    system_scroll += (system_scroll_target - system_scroll) * (1.0 - std::exp(-dt / system_scroll_time));
    if (std::abs(system_scroll_target - system_scroll) < .5)
        system_scroll = system_scroll_target;

    double row_target = row == ROW_SYSTEM ? 1.0 : 0.0;
    row_blend += (row_target - row_blend) * (1.0 - std::exp(-dt / row_blend_time));
    if (std::abs(row_target - row_blend) < .01)
        row_blend = row_target;

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
    const D3DXCOLOR invalid(.13f, .13f, .13f, 1.0f);
    /*const D3DXCOLOR arrow(.46f, .46f, .46f, 1.0f);*/
    const D3DXCOLOR arrow(1.0f, 1.0f, 1.0f, 1.0f);
    auto faded = [](D3DXCOLOR color, double alpha) {
        color.a *= (float)alpha;
        return color;
    };
    const UINT format = DT_NOCLIP | DT_SINGLELINE | DT_VCENTER;

    //only one row is shown.  switching slides the content down, bringing the systems in from above
    //while the letters fade out below, and back again
    long top = (long)scroll_pos - scroll_target;
    long row_h = (long)(clientHeight * row_height);
    long arrow_h = (long)(clientHeight * arrow_height);
    long shift = (long)(row_blend * row_h);

    //the letters, with an arrow above them pointing up to the systems
    if (row_blend < 1.0)
    {
        double alpha = 1.0 - row_blend;

        RECT r = {0, top + arrow_h + shift, clientWidth, top + arrow_h + shift + row_h};
        char j[2] = {0, 0};
        for (int i = 0; i < 27; i++)
        {
            j[0] = c[i];
            r.left = (LONG)((clientWidth / 29.0) * (i + 1));
            r.right = (LONG)((clientWidth / 29.0) * (i + 2));
            D3DXCOLOR color = i == selected ? highlight : valid_chars[i] ? normal : invalid;
            font->DrawText(NULL, j, -1, &r, format | DT_CENTER, faded(color, alpha));
        }
    }

    //the systems, left aligned with the letters, with an arrow beneath them pointing back down
    if (row_blend > 0.0)
    {
        long systems_top = top - row_h + shift;
        for (int i = 0; i < (int)systems.size(); i++)
        {
            int x = system_row_left + system_lefts[i] - (int)system_scroll;
            RECT r = {x, systems_top, x + system_widths[i], systems_top + row_h};
            system_font->DrawText(NULL, systems[i].c_str(), -1, &r, format | DT_LEFT,
                                  faded(i == selected_system ? highlight : normal, row_blend));
        }
    }
    //one arrow, pointing at the other row.  it travels with the content and turns over as it goes, from
    //above the letters pointing up to beneath the systems pointing down
    double arrow_visible = arrow_size * 2.0 * .866; //the triangle's height within its square
    double up_nudge = arrow_gap_closed * ((arrow_height - arrow_visible) / 2 + (row_height - letter_font_height) / 2);
    double down_nudge = arrow_gap_closed * ((row_height - system_font_height) / 2 + (arrow_height - arrow_visible) / 2);
    double up_y = top + clientHeight * (arrow_height / 2 + up_nudge);
    double down_y = top + clientHeight * (row_height + arrow_height / 2 - down_nudge);
    draw_arrow(clientWidth / 2.0, up_y + (down_y - up_y) * row_blend, clientHeight * arrow_size,
               row_blend * std::numbers::pi, arrow);

    d3dDev->OMSetDepthStencilState(state, oldref);
}
