#pragma once
#include <string>
#include "TexturePanelItem.h"
#include "d3dx10.h"
#include <memory>
#include <functional>
#include <vector>

import nemulator.buttons;
import system;

// A container that decouples nemulator-specific code from emulation code
class c_system_container : public TexturePanelItem
{
public:
    c_system_container(c_system::s_system_info &system_info, std::string &path, std::string &filename, std::string &sram_path);
    ~c_system_container();
    c_system* system;
    void DrawToTexture(ID3D10Texture2D *tex);
    ID3D10Buffer *get_vertex_buffer(int stretched);
    bool Selectable();
    bool mask_sides;
    bool limit_sprites;
    std::string get_filename();
    std::string title;
    std::string filename;
    double get_height();
    double get_width();
    struct SimpleVertex
    {
        D3DXVECTOR3 pos;
        D3DXVECTOR2 tex;
    };
    bool is_nes = false;

    std::string &get_system_name() { return system_info.name; }
    const std::vector<s_button_map> &get_button_map() { return system_info.button_map; }

  private:
    void OnActivate(bool load);
    void OnDeactivate();
    void create_vertex_buffer();

    int ref;

    std::string path;
    std::string sram_path;

    D3D10_BUFFER_DESC bd;
    ID3D10Buffer *vertex_buffer = NULL;
    ID3D10Buffer *stretched_vertex_buffer = NULL;
    ID3D10Buffer *default_vertex_buffer = NULL;
    ID3D10Buffer *unloaded_vertex_buffer = NULL;

    double width;
    double height;

    static const int tex_width = 512;
    static const int tex_height = 512;
    static const int static_width = 256;
    static const int static_height = 256;

    c_system::s_system_info &system_info;
};
