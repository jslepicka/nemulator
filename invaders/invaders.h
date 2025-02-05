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
    
    void set_irq(int irq);
    void enable_mixer();
    void disable_mixer();

  protected:
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

    std::unique_ptr<c_z80> z80;
    std::unique_ptr<uint8_t[]> rom;
    std::unique_ptr<uint8_t[]> ram;
    std::unique_ptr<uint8_t[]> vram;

    int nmi;
    int irq;
    uint8_t data_bus;

    uint64_t frame_counter;
    int loaded;
    uint16_t shift_register;
    uint8_t shift_amount;
    uint8_t INP1;
    uint32_t fb[256 * 224];
};