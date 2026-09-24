module;
#include "d3d10.h"
#include "D3DX10.h"
#include "shape.fxo.h"

#define ReleaseCOM(x) { if(x) {x->Release(); x = 0; } }

module nemulator:disk_indicator;

extern ID3D10Device *d3dDev;
extern int clientHeight;
extern int clientWidth;

//sizes are a fraction of the client height.  the panel sits in the lower left corner, clear of the status
//messages in the lower right
static constexpr double panel_height = .07;
static constexpr double panel_aspect = 2.0;
static constexpr double margin = .03;
//the pencil shown while writing rides the end of the bar, and reaches past the panel's right edge
static constexpr double pencil_overhang = .3;
//the side label, as a fraction of the panel's height.  its left edge lines up with the bar in shape.fx
static constexpr double label_left = .88;
static constexpr double label_bottom = .58;
static constexpr double label_font_height = .36;
//games stop the motor between files, so keep the indicator up briefly after the disk stops (ms)
static constexpr double hold_time = 500.0;
static constexpr double fade_in_time = 100.0;
static constexpr double fade_out_time = 250.0;
static constexpr double spin_period = 1200.0; //ms per turn of the hub
static constexpr double blink_period = 300.0; //the label blinks while the disk is being switched
static const D3DXCOLOR switch_color(.988f, .988f, .988f, 1.0f);
static const D3DXCOLOR label_color(.988f, .988f, .988f, 1.0f);

c_disk_indicator::c_disk_indicator()
{
    effect = 0;
    technique = 0;
    var_size = 0;
    var_progress = 0;
    var_writing = 0;
    var_spin = 0;
    var_alpha = 0;
    layout = 0;
    vertices = 0;
    font = 0;
    switching = false;
    hold_timer = 0.0;
    alpha = 0.0;
    spin = 0.0;
    blink_timer = 0.0;
}

c_disk_indicator::~c_disk_indicator()
{
    ReleaseCOM(effect);
    ReleaseCOM(layout);
    ReleaseCOM(vertices);
    ReleaseCOM(font);
}

void c_disk_indicator::init(void *params)
{
    load_font();

    HRESULT hr = D3DX10CreateEffectFromMemory((LPCVOID)g_shape_effect, sizeof(g_shape_effect), "shape", NULL, NULL,
                                              "fx_4_0", 0, 0, d3dDev, NULL, NULL, &effect, NULL, NULL);
    if (FAILED(hr))
        return;
    technique = effect->GetTechniqueByName("Disk");
    var_size = effect->GetVariableByName("disk_size")->AsVector();
    var_progress = effect->GetVariableByName("disk_progress")->AsScalar();
    var_writing = effect->GetVariableByName("disk_writing")->AsScalar();
    var_spin = effect->GetVariableByName("disk_spin")->AsScalar();
    var_alpha = effect->GetVariableByName("disk_alpha")->AsScalar();

    D3D10_INPUT_ELEMENT_DESC desc[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D10_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D10_INPUT_PER_VERTEX_DATA, 0},
    };
    D3D10_PASS_DESC pass;
    technique->GetPassByIndex(0)->GetDesc(&pass);
    d3dDev->CreateInputLayout(desc, 2, pass.pIAInputSignature, pass.IAInputSignatureSize, &layout);

    //rewritten each frame, since the panel follows the client size
    D3D10_BUFFER_DESC bd = {};
    bd.Usage = D3D10_USAGE_DYNAMIC;
    bd.ByteWidth = sizeof(s_vertex) * 4;
    bd.BindFlags = D3D10_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = D3D10_CPU_ACCESS_WRITE;
    d3dDev->CreateBuffer(&bd, NULL, &vertices);
}

//the indicator is updated by report(), but it sits between the menus and c_nemulator in the task list, so
//it has to pass their results along
int c_disk_indicator::update(double dt, int child_result, void *params)
{
    return child_result;
}

void c_disk_indicator::resize()
{
    load_font();
}

void c_disk_indicator::load_font()
{
    ReleaseCOM(font);
    D3DX10_FONT_DESC fontDesc;
    fontDesc.Height = (int)(clientHeight * panel_height * label_font_height);
    fontDesc.Width = 0;
    fontDesc.Weight = 0;
    fontDesc.MipLevels = 1;
    fontDesc.Italic = false;
    fontDesc.CharSet = DEFAULT_CHARSET;
    fontDesc.OutputPrecision = OUT_DEFAULT_PRECIS;
    fontDesc.Quality = DEFAULT_QUALITY;
    fontDesc.PitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
    strcpy_s(fontDesc.FaceName, "Calibri");
    D3DX10CreateFontIndirect(d3dDev, &fontDesc, &font);
}

void c_disk_indicator::report(double dt, bool visible, const c_system::s_disk_activity &activity)
{
    using STATE = c_system::s_disk_activity::STATE;
    if (!visible)
    {
        alpha = 0.0;
        hold_timer = 0.0;
        switching = false;
        return;
    }

    bool spinning = activity.state == STATE::READING || activity.state == STATE::WRITING;
    switching = activity.state == STATE::SWITCHING;
    if (activity.state != STATE::IDLE)
    {
        shown = activity;
        hold_timer = hold_time;
    }
    else
        hold_timer = std::max(hold_timer - dt, 0.0);

    if (hold_timer > 0.0)
        alpha = std::min(alpha + dt / fade_in_time, 1.0);
    else
        alpha = std::max(alpha - dt / fade_out_time, 0.0);

    if (spinning)
        spin = std::fmod(spin + dt / spin_period * 2.0 * std::numbers::pi, 2.0 * std::numbers::pi);
    blink_timer = std::fmod(blink_timer + dt, blink_period * 2.0);
}

void c_disk_indicator::draw()
{
    using STATE = c_system::s_disk_activity::STATE;
    if (alpha <= 0.0 || !technique || !layout || !vertices)
        return;

    double height = clientHeight * panel_height;
    double width = height * panel_aspect;
    double left = clientHeight * margin;
    double top = clientHeight * (1.0 - margin) - height;

    //Local is the position within the panel in pixels.  the quad covers the pencil's overhang as well
    double quad_width = width + height * pencil_overhang;
    static const float corners[4][2] = {{0.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}};
    s_vertex *v;
    if (FAILED(vertices->Map(D3D10_MAP_WRITE_DISCARD, 0, (void **)&v)))
        return;
    for (int i = 0; i < 4; i++)
    {
        double px = left + corners[i][0] * quad_width;
        double py = top + corners[i][1] * height;
        v[i] = {(float)(px / clientWidth * 2.0 - 1.0), (float)(1.0 - py / clientHeight * 2.0), 0.5f,
                (float)(corners[i][0] * quad_width), (float)(corners[i][1] * height)};
    }
    vertices->Unmap();

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

    float size[4] = {(float)width, (float)height, 0.0f, 0.0f};
    UINT stride = sizeof(s_vertex);
    UINT offset = 0;
    d3dDev->IASetInputLayout(layout);
    d3dDev->IASetVertexBuffers(0, 1, &vertices, &stride, &offset);
    d3dDev->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    var_size->SetFloatVector(size);
    var_progress->SetFloat(shown.state == STATE::SWITCHING ? 0.0f : (float)shown.position);
    var_writing->SetFloat(shown.state == STATE::WRITING ? 1.0f : 0.0f);
    var_spin->SetFloat((float)spin);
    var_alpha->SetFloat((float)alpha);
    technique->GetPassByIndex(0)->Apply(0);
    d3dDev->Draw(4, 0);

    d3dDev->OMSetBlendState(blend, blend_factor, sample_mask);
    d3dDev->OMSetDepthStencilState(depth, stencil_ref);
    d3dDev->RSSetState(raster);
    ReleaseCOM(blend);
    ReleaseCOM(raster);

    if (font)
    {
        //the side being put in blinks while the disk is out
        D3DXCOLOR color = switching ? switch_color : label_color;
        color.a = (float)(alpha * (switching && blink_timer >= blink_period ? .25 : 1.0));
        std::string label = shown.get_side_name();
        RECT r = {(long)(left + height * label_left), (long)top, (long)(left + width),
                  (long)(top + height * label_bottom)};
        font->DrawText(NULL, label.c_str(), -1, &r, DT_NOCLIP | DT_LEFT | DT_BOTTOM | DT_SINGLELINE, color);
        d3dDev->OMSetDepthStencilState(depth, stencil_ref);
    }
    ReleaseCOM(depth);
}
