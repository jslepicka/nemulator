module;

#include "d3d11.h"
#include <DirectXMath.h>
#include "transform_vs.fxo.h"
#include "overlay_ps.fxo.h"

#define ReleaseCOM(x) { if(x) { x->Release(); x = 0; } }

module nemulator:audio_info;

extern ID3D11Device *d3dDev;
extern ID3D11DeviceContext *d3dContext;
extern DirectX::XMFLOAT4X4 matrixView;
extern DirectX::XMFLOAT4X4 matrixProj;


c_audio_info::c_audio_info()
{
    count = 0;
    read_pointer = 0;
    for (int i = 0; i < tex_size; i++)
    {
        freq[i] = -1;
        buf[i] = -1;
        max_freq[i] = -1;
        min_freq[i] = -1;
    }
    stable = 0;
    x = y = z = 0.0f;
}

c_audio_info::~c_audio_info()
{
    ReleaseCOM(transform_buffer);
    ReleaseCOM(sampler);
}

void c_audio_info::report(int freq, int buf, int stable, int max_freq, int min_freq)
{
    this->stable = stable;
    this->freq[count] = freq;
    this->buf[count] = buf;
    this->max_freq[count] = max_freq;
    this->min_freq[count] = min_freq;
    read_pointer = count;
    count++;
    if (count == tex_size)
        count = 0;
}

void c_audio_info::init(void *params)
{
    HRESULT hr;
    D3D11_INPUT_ELEMENT_DESC elementDesc[] =
    {
        { TEXT("POSITION"), 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { TEXT("TEXCOORD"), 0, DXGI_FORMAT_R32G32_FLOAT,    0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    UINT numElements = sizeof(elementDesc)/sizeof(elementDesc[0]);
    D3D11_BLEND_DESC blend = alpha_blend_desc();
    bool created = pass.create({g_transform_vs, sizeof(g_transform_vs), g_overlay_ps, sizeof(g_overlay_ps), &blend});
    transform_buffer = create_constant_buffer<s_transform_constants>();
    sampler = create_linear_sampler(D3D11_TEXTURE_ADDRESS_CLAMP);
    if (!created || !transform_buffer || !sampler)
    {
        MessageBox(NULL, "Unable to create shaders", 0, 0);
        PostQuitMessage(0);
        return;
    }

    hr = d3dDev->CreateInputLayout(elementDesc, numElements, g_transform_vs, sizeof(g_transform_vs), &vertex_layout);
    if (FAILED(hr))
    {
        MessageBox(NULL, "CreateInputLayout failed", 0, 0);
        PostQuitMessage(0);
        return;
    }

    SimpleVertex vertices[] = 
    {
        { { -1.0f, -.3f, 0.0f }, { 0.0f, 1.0f } },
        { { -1.0f, .3f, 0.0f }, { 0.0f, 0.0f } },
        { { 1.0f, -.3f, 0.0f }, { 1.0f, 1.0f } },
        { { 1.0f, .3f, 0.0f }, { 1.0f, 0.0f } },
    };

    D3D11_BUFFER_DESC bd = {};
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(SimpleVertex) * 4;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = 0;
    bd.MiscFlags = 0;
    D3D11_SUBRESOURCE_DATA initData;
    initData.pSysMem = vertices;
    hr = d3dDev->CreateBuffer(&bd, &initData, &vertex_buffer);
    if (FAILED(hr))
    {
        MessageBox(NULL, "CreateBuffer failed", 0, 0);
        PostQuitMessage(0);
        return;
    }

    D3D11_TEXTURE2D_DESC texDesc;
    texDesc.Width = tex_size;
    texDesc.Height = tex_size;
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 1;
    texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texDesc.SampleDesc.Count = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Usage = D3D11_USAGE_DYNAMIC;
    //texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    texDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    //texDesc.CPUAccessFlags = 0;
    texDesc.MiscFlags = 0;

    hr = d3dDev->CreateTexture2D(&texDesc, 0, &tex);
    if (FAILED(hr))
    {
        MessageBox(NULL, "CreateTexture2D failed", 0, 0);
        PostQuitMessage(0);
        return;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC descRv;
    descRv.Format = texDesc.Format;
    descRv.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    descRv.Texture2D.MipLevels = 1;
    descRv.Texture2D.MostDetailedMip = 0;
    hr = d3dDev->CreateShaderResourceView(tex, &descRv, &tex_rv);
    if (FAILED(hr))
    {
        MessageBox(NULL, "CreateShaderResourceView failed", 0, 0);
        PostQuitMessage(0);
        return;
    }

}

void c_audio_info::resize()
{
}

int c_audio_info::update(double dt, int child_result, void *params)
{
    return child_result;
}

void c_audio_info::draw()
{
    UINT stride = sizeof(SimpleVertex);
    UINT offset = 0;
    d3dContext->IASetInputLayout(vertex_layout);
    d3dContext->IASetVertexBuffers(0, 1, &vertex_buffer, &stride, &offset);
    d3dContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    D3D11_MAPPED_SUBRESOURCE map;
    d3dContext->Map(tex, 0, D3D11_MAP_WRITE_DISCARD, 0, &map);
    int *p = (int*)map.pData;



    for (int y = 0; y < tex_size; y++)
    {
        for (int x = 0; x < tex_size; x++)
        {
            int range = MAX-0;
            int t = 3200;
            //1600 * 3 * 2;
            int min = (int)((tex_size - 1) - (((range - (MAX - (t-(1600*2))))/(double)range) * (tex_size - 1)));
            int max = (int)((tex_size - 1) - (((range - (MAX - (t+(1600*2))))/(double)range) * (tex_size - 1)));
            int target = (int)((tex_size - 1) - (((range - (MAX - t))/(double)range) * (tex_size - 1)));


            if (y <= min && y >= max)
            {
                int trans = (int)((1.0 - ((double)abs(y - target) / ((min-max)/2))) * 0xFF);
                //trans <<= 24;

                if (stable)
                    *p++ = 0x99009900 | trans;
                else
                    *p++ = 0x99000000 | trans;
            }
            else
                *p++ = 0x99000000;
        }
    }


    for (int i = 0; i < tex_size; i++)
    {
        if (freq[read_pointer] != -1)
        {
            int range = (int)((48000*1.02)-(48000*.98));
            int y = (int)(((range - ((48000*1.02) - freq[read_pointer]))/(double)range) * (tex_size - 1));
            y = ((tex_size - 1) - y);
            if (y > (tex_size - 1))
                y = (tex_size - 1);
            else if (y < 0)
                y = 0;

            int *pp = (int*)map.pData;
            *(pp + (y*tex_size) + i) = 0xFF00FF00;
        }

        int range = MAX-0;
        if (buf[read_pointer] != -1)
        {
            int y = (int)(((range - (MAX - buf[read_pointer]))/(double)range) * (tex_size - 1));
            y = ((tex_size - 1) - y);

            if (y > (tex_size - 1))
                y = (tex_size - 1);
            else if (y < 0)
                y = 0;

            int *pp = (int*)map.pData;
            *(pp + (y*tex_size) + i) = 0xFF0000FF;
        }

        read_pointer = (read_pointer + 1) % tex_size;
    }

    d3dContext->Unmap(tex, 0);

    transform.projection = matrixProj;
    transform.view = matrixView;
    DirectX::XMStoreFloat4x4(&transform.world, DirectX::XMMatrixTranslation(x, y, z));
    d3dContext->UpdateSubresource(transform_buffer, 0, NULL, &transform, 0, 0);
    d3dContext->VSSetConstantBuffers(0, 1, &transform_buffer);
    d3dContext->PSSetShaderResources(0, 1, &tex_rv);
    d3dContext->PSSetSamplers(0, 1, &sampler);
    pass.apply();
    d3dContext->Draw(4, 0);
}