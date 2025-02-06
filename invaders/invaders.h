#pragma once
#include "..\console.h"
#include <memory>
#include <array>
#include <vector>

class c_z80;

class c_invaders : public c_console
{
  public:
    c_invaders();
    virtual int load();
    int is_loaded();
    int emulate_frame();
    virtual int reset();
    int get_crc();
    int get_sound_bufs(const short **buf_l, const short **buf_r);
    void set_audio_freq(double freq);
    void set_input(int input);
    int *get_video();
    virtual ~c_invaders();
    
    void enable_mixer();
    void disable_mixer();

  private:
    struct s_roms
    {
        std::string filename;
        uint32_t crc32;
        uint32_t length;
        uint32_t offset;
        uint8_t *loc;
    };
    virtual uint8_t read_byte(uint16_t address);
    virtual void write_byte(uint16_t address, uint8_t data);
    uint8_t read_port(uint8_t port);

    void write_port(uint8_t port, uint8_t data);
    int load_romset(std::vector<s_roms> &romset);

    void int_ack();

    std::unique_ptr<c_z80> z80;

    int nmi;
    int irq;
    uint8_t data_bus;

    int loaded;
    uint16_t shift_register;
    uint8_t shift_amount;
    union {
        struct
        {
            uint8_t credit : 1;
            uint8_t p2_start : 1;
            uint8_t p1_start : 1;
            uint8_t : 1; //always 1
            uint8_t p1_shoot : 1;
            uint8_t p1_left : 1;
            uint8_t p1_right : 1;
            uint8_t : 1; //not connected
        };
        uint8_t value;
    } INP1;

    union {
        struct
        {
            uint8_t ships0 : 1; //ships0 << 1 | ships1
            uint8_t ships1 : 1; //00 = 3, 10 = 5, 01 = 4, 11 = 6
            uint8_t tilt : 1; // tilt?
            uint8_t extra_ship : 1; //0 = 1500 pts, 1 = 1000 pts
            uint8_t p2_shot : 1;
            uint8_t p2_left : 1;
            uint8_t p2_right : 1;
            uint8_t display_coin : 1; //display coin info in demo 0 = on
        };
        uint8_t value;
    } INP2;
    static const int FB_WIDTH = 256;
    static const int FB_HEIGHT = 224;
    uint8_t rom[8 * 1024];
    uint8_t ram[1 * 1024];
    uint8_t vram[7 * 1024];
    uint32_t fb[FB_WIDTH * FB_HEIGHT];
};