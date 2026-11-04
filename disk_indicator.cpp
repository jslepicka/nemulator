module;
#include "d3d11.h"
#include "shape_vs.fxo.h"
#include "shape_disk_ps.fxo.h"

#define ReleaseCOM(x) { if(x) {x->Release(); x = 0; } }

module nemulator:disk_indicator;

extern ID3D11Device *d3dDev;
extern ID3D11DeviceContext *d3dContext;
extern int clientHeight;
extern int clientWidth;

//sizes are a fraction of the client height.  the panel sits in the lower left corner, clear of the status
//messages in the lower right
static constexpr double panel_height = .07;
static constexpr double panel_aspect = 2.0;
static constexpr double margin = .03;
//the pencil shown while writing rides the end of the bar, and reaches past the panel's right edge
static constexpr double pencil_overhang = .3;
//the side label, as a fraction of the panel's height.  its left edge lines up with the bar in shape_disk_ps.hlsl
static constexpr double label_left = .88;
static constexpr double label_bottom = .58;
static constexpr double label_font_height = .36;
//games stop the motor between files, so keep the indicator up briefly after the disk stops (ms)
static constexpr double hold_time = 500.0;
static constexpr double fade_in_time = 100.0;
static constexpr double fade_out_time = 250.0;
static constexpr double spin_period = 1200.0; //ms per turn of the hub
static constexpr double blink_period = 300.0; //the label blinks while the disk is being switched
static const s_color switch_color(.988f, .988f, .988f, 1.0f);
static const s_color label_color(.988f, .988f, .988f, 1.0f);

c_disk_indicator::c_disk_indicator()
{
    constants_buffer = 0;
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
    ReleaseCOM(constants_buffer);
    ReleaseCOM(layout);
    ReleaseCOM(vertices);
    delete font;
}

void c_disk_indicator::init(void *params)
{
    load_font();

    D3D11_BLEND_DESC blend = alpha_blend_desc();
    D3D11_DEPTH_STENCIL_DESC depth = {};
    depth.DepthEnable = FALSE;
    depth.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    depth.DepthFunc = D3D11_COMPARISON_LESS;
    //a rotated shape can face either way
    D3D11_RASTERIZER_DESC raster = {};
    raster.FillMode = D3D11_FILL_SOLID;
    raster.CullMode = D3D11_CULL_NONE;
    raster.DepthClipEnable = TRUE;
    if (!pass.create({g_shape_vs, sizeof(g_shape_vs), g_shape_disk_ps, sizeof(g_shape_disk_ps), &blend, &depth, &raster}))
        return;
    constants_buffer = create_constant_buffer<s_constants>();

    D3D11_INPUT_ELEMENT_DESC desc[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    d3dDev->CreateInputLayout(desc, 2, g_shape_vs, sizeof(g_shape_vs), &layout);

    //rewritten each frame, since the panel follows the client size
    D3D11_BUFFER_DESC bd = {};
    bd.Usage = D3D11_USAGE_DYNAMIC;
    bd.ByteWidth = sizeof(s_vertex) * 4;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
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
    delete font;
    font = new c_font((int)(clientHeight * panel_height * label_font_height), "Calibri");
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
    if (alpha <= 0.0 || !pass.is_created() || !constants_buffer || !layout || !vertices)
        return;

    double height = clientHeight * panel_height;
    double width = height * panel_aspect;
    double left = clientHeight * margin;
    double top = clientHeight * (1.0 - margin) - height;

    //Local is the position within the panel in pixels.  the quad covers the pencil's overhang as well
    double quad_width = width + height * pencil_overhang;
    static const float corners[4][2] = {{0.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}};
    D3D11_MAPPED_SUBRESOURCE mapped;
    if (FAILED(d3dContext->Map(vertices, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
        return;
    s_vertex *v = (s_vertex *)mapped.pData;
    for (int i = 0; i < 4; i++)
    {
        double px = left + corners[i][0] * quad_width;
        double py = top + corners[i][1] * height;
        v[i] = {(float)(px / clientWidth * 2.0 - 1.0), (float)(1.0 - py / clientHeight * 2.0), 0.5f,
                (float)(corners[i][0] * quad_width), (float)(corners[i][1] * height)};
    }
    d3dContext->Unmap(vertices, 0);

    //the pass sets blend, depth, and rasterizer state; put back whatever was there, since the game
    //panels don't set their own rasterizer state
    ID3D11BlendState *blend = NULL;
    FLOAT blend_factor[4];
    UINT sample_mask;
    d3dContext->OMGetBlendState(&blend, blend_factor, &sample_mask);
    ID3D11DepthStencilState *depth = NULL;
    UINT stencil_ref;
    d3dContext->OMGetDepthStencilState(&depth, &stencil_ref);
    ID3D11RasterizerState *raster = NULL;
    d3dContext->RSGetState(&raster);

    UINT stride = sizeof(s_vertex);
    UINT offset = 0;
    d3dContext->IASetInputLayout(layout);
    d3dContext->IASetVertexBuffers(0, 1, &vertices, &stride, &offset);
    d3dContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    s_constants constants = {{(float)width, (float)height},
                             shown.state == STATE::SWITCHING ? 0.0f : (float)shown.position,
                             shown.state == STATE::WRITING ? 1.0f : 0.0f,
                             (float)spin,
                             (float)alpha};
    d3dContext->UpdateSubresource(constants_buffer, 0, NULL, &constants, 0, 0);
    d3dContext->PSSetConstantBuffers(0, 1, &constants_buffer);
    pass.apply();
    d3dContext->Draw(4, 0);

    d3dContext->OMSetBlendState(blend, blend_factor, sample_mask);
    d3dContext->OMSetDepthStencilState(depth, stencil_ref);
    d3dContext->RSSetState(raster);
    ReleaseCOM(blend);
    ReleaseCOM(raster);

    if (font)
    {
        //the side being put in blinks while the disk is out
        s_color color = switching ? switch_color : label_color;
        color.a = (float)(alpha * (switching && blink_timer >= blink_period ? .25 : 1.0));
        std::string label = shown.get_side_name();
        RECT r = {(long)(left + height * label_left), (long)top, (long)(left + width),
                  (long)(top + height * label_bottom)};
        font->draw(label.c_str(), r, DT_LEFT | DT_BOTTOM | DT_SINGLELINE, color);
        d3dContext->OMSetDepthStencilState(depth, stencil_ref);
    }
    ReleaseCOM(depth);
}
