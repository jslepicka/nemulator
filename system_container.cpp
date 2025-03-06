module;
#include "d3d10.h"
#include "d3dx10.h"
#include <crtdbg.h>
#include <immintrin.h>
#include <filesystem>
#include "nes\nes.h"
module nemulator.system_container;
import random;

extern HANDLE g_start_event;

extern ID3D10Device *d3dDev;

std::string c_system_container::get_filename()
{
    return filename;
}

c_system_container::c_system_container(c_system::s_system_info &si, std::string &path, std::string &filename, std::string &sram_path) :
    system_info(si)
{
    limit_sprites = false;
    this->path = path;
    this->filename = filename;
    this->sram_path = sram_path;
    ref = 0;
    system = 0;
    mask_sides = false;
    title = filename.substr(0, filename.find_last_of("."));

    bd.Usage = D3D10_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(SimpleVertex) * 4;
    bd.BindFlags = D3D10_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = 0;
    bd.MiscFlags = 0;
}

c_system_container::~c_system_container()
{
    if (vertex_buffer)
    {
        vertex_buffer->Release();
        vertex_buffer = NULL;
    }
}

void c_system_container::OnActivate(bool load)
{
    if (ref == 0)
    {
        SimpleVertex unloaded_vertices[4] = {{D3DXVECTOR3(-4.0f / 3.0f, -1.0f, 0.0f), D3DXVECTOR2(0.0f, (FLOAT)((double)static_height/tex_height))},
                                             {D3DXVECTOR3(-4.0f / 3.0f, 1.0f, 0.0f), D3DXVECTOR2(0.0f, 0.0f)},
                                             {D3DXVECTOR3(4.0f / 3.0f, -1.0f, 0.0f), D3DXVECTOR2((FLOAT)((double)static_width/tex_width), (FLOAT)((double)static_height/tex_height))},
                                             {D3DXVECTOR3(4.0f / 3.0f, 1.0f, 0.0f), D3DXVECTOR2((FLOAT)((double)static_width/tex_width), 0.0f)}};

        D3D10_SUBRESOURCE_DATA initData;
        initData.pSysMem = unloaded_vertices;
        HRESULT hr = d3dDev->CreateBuffer(&bd, &initData, &unloaded_vertex_buffer);

        if (!system)
        {
            system = system_info.constructor();
            if (system)
            {
                system->filename = filename;
                system->path = path;
                system->path_file = path + "\\" + filename;
                system->sram_path = sram_path;
                system->sram_filename = filename.substr(0, filename.find_last_of(".")) + ".ram";
                system->sram_path_file = sram_path + "\\" + system->sram_filename;
                if (load)
                    system->load();
                if (system->is_loaded()) {
                    system->disable_mixer();
                    if (is_nes) {
                        ((nes::c_nes *)system.get())->set_sprite_limit(limit_sprites);
                    }
                }
                create_vertex_buffer();
            }
        }
        is_active = 1;
    }
    ++ref;
}

void c_system_container::OnDeactivate()
{
    if (ref == 0)
        return;
    --ref;
    if (ref == 0)
    {
        if (system)
        {
            system.reset();
        }
        if (vertex_buffer)
        {
            vertex_buffer->Release();
            vertex_buffer = NULL;
        }
        if (stretched_vertex_buffer)
        {
            stretched_vertex_buffer->Release();
            stretched_vertex_buffer = NULL;
        }
        if (default_vertex_buffer)
        {
            default_vertex_buffer->Release();
            default_vertex_buffer = NULL;
        }
        is_active = 0;
    }
}

void c_system_container::DrawToTexture(ID3D10Texture2D *tex)
{
    D3D10_MAPPED_TEXTURE2D map;
    map.pData = 0;
    tex->Map(0, D3D10_MAP_WRITE_DISCARD, NULL, &map);
    int* p;
    auto &display_info = system_info.display_info;
    if (system && system->is_loaded())
    {
        int *fb_base = system->get_video();
        int y = 0;
        for (; y < display_info.fb_height; y++) {
            int *fb = fb_base + (display_info.fb_width * y);
            p = (int*)map.pData + (y) * (map.RowPitch / 4);
            int x = 0;
            int x_end = display_info.fb_width;
            if (mask_sides) {
                for (int m = 0; m < 8; m++) {
                    *p++ = 0xFF000000;
                    fb++;
                }
                x += 8;
                x_end -= 8;
            }
            for (; x < x_end; x++) {
                *p++ = *fb++;
            }
            for (; x < tex_width; x++) {
                *p++ = 0xFF000000;
            }
        }
        for (; y < tex_height; y++) {
            p = (int*)map.pData + (y) * (map.RowPitch / 4);
            for (int x = 0; x < tex_width; x++) {
                *p++ = 0xff000000;
            }
        }

    }
    else
    {
        unsigned char c = 0;
        for (int y = 0; y < static_height; ++y)
        {
            p = (int*)map.pData + y * (map.RowPitch / 4);
            for (int x = 0; x < static_width; ++x)
            {
                c = random::get_rand() & 0xFF;
                *p++ = 0xFF << 24 | c << 16 | c << 8 | c;
            }
        }
    }
    tex->Unmap(0);
}

bool c_system_container::Selectable()
{
    if (system)
        return system->is_loaded();
    else
        return 0;
}

void c_system_container::create_vertex_buffer()
{
    if (vertex_buffer)
    {
        vertex_buffer->Release();
        vertex_buffer = NULL;
    }

    auto &display_info = system_info.display_info;

    //width and height of texture we're drawing to
    int h = display_info.fb_height;
    int w = display_info.fb_width;

    double w_adjust = 0.0;
    double h_adjust = 0.0;

    double aspect_ratio = display_info.aspect_ratio;

    if (aspect_ratio != 0.0) {
        if (aspect_ratio > 1.0) {
            w_adjust = ((4.0 / 3.0) / aspect_ratio * w - w) / 2.0;
        }
        else {
            h_adjust = ((4.0 / 3.0) / aspect_ratio * h - h) / 2.0;
        }
    }
    
    double w_start = (display_info.crop_left - w_adjust);
    double w_end = (w - display_info.crop_right + w_adjust);
    double h_start = (display_info.crop_top - h_adjust);
    double h_end = (h - display_info.crop_bottom + h_adjust);
    width = w_end - w_start;
    height = h_end - h_start;

    w_start /= tex_width;
    w_end /= tex_width;
    h_start /= tex_height;
    h_end /= tex_height;

    SimpleVertex vertices[4] = {
        {D3DXVECTOR3(-4.0f / 3.0f, -1.0f, 0.0f), D3DXVECTOR2((FLOAT)w_start, (FLOAT)h_end)},
        {D3DXVECTOR3(-4.0f / 3.0f, 1.0f, 0.0f), D3DXVECTOR2((FLOAT)w_start, (FLOAT)h_start)},
        {D3DXVECTOR3(4.0f / 3.0f, -1.0f, 0.0f), D3DXVECTOR2((FLOAT)w_end, (FLOAT)h_end)},
        {D3DXVECTOR3(4.0f / 3.0f, 1.0f, 0.0f), D3DXVECTOR2((FLOAT)w_end, (FLOAT)h_start)},
    };

    if (display_info.rotation == 90) {
        vertices[0].tex.x = vertices[2].tex.x = (FLOAT)w_end;
        vertices[1].tex.x = vertices[3].tex.x = (FLOAT)w_start;
        vertices[0].tex.y = vertices[1].tex.y = (FLOAT)h_end;
        vertices[2].tex.y = vertices[3].tex.y = (FLOAT)h_start;
    }
    else if (display_info.rotation == 270) {
        vertices[0].tex.x = vertices[2].tex.x = (FLOAT)w_start;
        vertices[1].tex.x = vertices[3].tex.x = (FLOAT)w_end;
        vertices[0].tex.y = vertices[1].tex.y = (FLOAT)h_start;
        vertices[2].tex.y = vertices[3].tex.y = (FLOAT)h_end;
    }

    D3D10_SUBRESOURCE_DATA initData;
    initData.pSysMem = vertices;
    HRESULT hr = d3dDev->CreateBuffer(&bd, &initData, &vertex_buffer);
}

//return the pixel width/height accounting for cropping and aspect ratio adjustment
//this is the number of visible pixels in the texture.  It's needed by the shader
//for scaling purposes.
double c_system_container::get_width()
{
    if (system && !system->is_loaded()) {
        return static_width;
    }
    return width;
}

double c_system_container::get_height()
{
    if (system && !system->is_loaded()) {
        return static_height;
    }
    return height;
}

ID3D10Buffer* c_system_container::get_vertex_buffer(int stretched)
{
    if (system && !system->is_loaded()) {
        return unloaded_vertex_buffer;
    }
    if (stretched)
        return stretched_vertex_buffer; 
    else
        return vertex_buffer;
};
