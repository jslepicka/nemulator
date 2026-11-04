module;
#include <d3d11.h>
#include <DirectXMath.h>

#define ReleaseCOM(x) { if(x) { x->Release(); x = 0; } }

module shader_pass;

extern ID3D11Device *d3dDev;
extern ID3D11DeviceContext *d3dContext;

c_shader_pass::~c_shader_pass()
{
    ReleaseCOM(vertex_shader);
    ReleaseCOM(pixel_shader);
    ReleaseCOM(blend);
    ReleaseCOM(depth);
    ReleaseCOM(raster);
}

bool c_shader_pass::create(const s_desc &desc)
{
    HRESULT hr = d3dDev->CreateVertexShader(desc.vertex_shader, desc.vertex_shader_size, NULL, &vertex_shader);
    if (SUCCEEDED(hr))
        hr = d3dDev->CreateBlendState(desc.blend, &blend);
    if (SUCCEEDED(hr) && desc.depth)
        hr = d3dDev->CreateDepthStencilState(desc.depth, &depth);
    if (SUCCEEDED(hr) && desc.raster)
        hr = d3dDev->CreateRasterizerState(desc.raster, &raster);
    //last, since is_created goes by the pixel shader
    if (SUCCEEDED(hr))
        hr = d3dDev->CreatePixelShader(desc.pixel_shader, desc.pixel_shader_size, NULL, &pixel_shader);
    return SUCCEEDED(hr);
}

void c_shader_pass::apply()
{
    d3dContext->VSSetShader(vertex_shader, NULL, 0);
    d3dContext->GSSetShader(NULL, NULL, 0);
    d3dContext->PSSetShader(pixel_shader, NULL, 0);
    float blend_factor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    d3dContext->OMSetBlendState(blend, blend_factor, 0xFFFFFFFF);
    if (depth)
        d3dContext->OMSetDepthStencilState(depth, 0);
    if (raster)
        d3dContext->RSSetState(raster);
}

D3D11_BLEND_DESC no_blend_desc()
{
    D3D11_BLEND_DESC desc = {};
    D3D11_RENDER_TARGET_BLEND_DESC &target = desc.RenderTarget[0];
    target.SrcBlend = D3D11_BLEND_ONE;
    target.DestBlend = D3D11_BLEND_ZERO;
    target.BlendOp = D3D11_BLEND_OP_ADD;
    target.SrcBlendAlpha = D3D11_BLEND_ONE;
    target.DestBlendAlpha = D3D11_BLEND_ZERO;
    target.BlendOpAlpha = D3D11_BLEND_OP_ADD;
    target.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    return desc;
}

D3D11_BLEND_DESC alpha_blend_desc()
{
    D3D11_BLEND_DESC desc = no_blend_desc();
    D3D11_RENDER_TARGET_BLEND_DESC &target = desc.RenderTarget[0];
    target.BlendEnable = TRUE;
    target.SrcBlend = D3D11_BLEND_SRC_ALPHA;
    target.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    return desc;
}

//with border addressing, outside the texture is opaque black
ID3D11SamplerState *create_linear_sampler(D3D11_TEXTURE_ADDRESS_MODE address)
{
    D3D11_SAMPLER_DESC desc = {};
    desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    desc.AddressU = address;
    desc.AddressV = address;
    desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    desc.BorderColor[3] = 1.0f;
    desc.MaxLOD = D3D11_FLOAT32_MAX;
    ID3D11SamplerState *sampler = NULL;
    d3dDev->CreateSamplerState(&desc, &sampler);
    return sampler;
}

ID3D11Buffer *create_constant_buffer(UINT size)
{
    D3D11_BUFFER_DESC desc = {};
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.ByteWidth = size;
    desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    ID3D11Buffer *buffer = NULL;
    d3dDev->CreateBuffer(&desc, NULL, &buffer);
    return buffer;
}
