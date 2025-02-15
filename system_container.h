#pragma once
#include "system.h"
#include "nes\nes.h"
#include "sms\sms.h"
#include "gb\gb.h"
#include "pacman\pacman.h"
#include "pacman\mspacman.h"
#include "invaders\invaders.h"
#include <string>
#include "TexturePanelItem.h"
#include "d3dx10.h"
#include <memory>
#include <functional>
#include "game_types.h"

// A container that decouples nemulator-specific code from emulation code
class c_system_container : public TexturePanelItem
{
public:
    c_system_container(GAME_TYPE type, std::string path, std::string filename, std::string sram_path, std::function<c_system*()> constructor);
    ~c_system_container();
    c_system* system;
    void DrawToTexture(ID3D10Texture2D *tex);
    ID3D10Buffer *get_vertex_buffer(int stretched);
    bool Selectable();
    bool mask_sides;
    bool limit_sprites;
    std::string get_filename();
    bool favorite;
    int submapper;
    char title[MAX_PATH];
    std::string filename;
    GAME_TYPE type;
    int played = 0;
    double get_height();
    double get_width();
    struct SimpleVertex
    {
        D3DXVECTOR3 pos;
        D3DXVECTOR2 tex;
    };

  private:
    std::function<c_system *()> constructor;
    void OnActivate(bool load);
    void OnDeactivate();
    void OnLoad();
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
};
