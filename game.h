#pragma once
#include "console.h"
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

enum GAME_TYPE
{
    GAME_NES,
    GAME_SMS,
    GAME_GG,
    GAME_GB,
    GAME_GBC,
    GAME_PACMAN,
    GAME_MSPACMAN,
    GAME_MSPACMNF, //ms pac man fast
    GAME_MSPACMAB, //ms pac man pre-decrypted roms
    GAME_INVADERS,
    GAME_NONE
};

class c_game : public TexturePanelItem
{
public:
    c_game(GAME_TYPE type, std::string path, std::string filename, std::string sram_path);
    HANDLE GetEventStart() { return eventStart; } //Retrieve eventStart handle
    HANDLE GetEventDone() { return eventDone; } //Retrieve eventDone handle
    ~c_game();
    HANDLE eventStart;
    HANDLE eventDone;
    c_console* console;
    void DrawToTexture(ID3D10Texture2D *tex);
    ID3D10Buffer *get_vertex_buffer(int stretched);
    bool Selectable();
    bool mask_sides;
    bool limit_sprites;
    int emulation_mode;
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
    ID3D10Buffer* default_vertex_buffer = NULL;
    ID3D10Buffer *unloaded_vertex_buffer = NULL;

    double width;
    double height;

    static const int tex_width = 512;
    static const int tex_height = 512;
    static const int static_width = 256;
    static const int static_height = 256;
    c_console::display_info_t display_info;
};
