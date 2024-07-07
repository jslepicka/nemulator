#pragma once
#include "../console.h"
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <string>

class c_sm83;
class c_gbmapper;
class c_gbppu;
class c_gbapu;

enum GB_MODEL
{
    DMG,
    CGB
};

class c_gb : public c_console
{
  public:
    c_gb(GB_MODEL model);
    ~c_gb();
    int load();
    void write_byte(uint16_t address, uint8_t data);
    void write_word(uint16_t address, uint16_t data);
    uint8_t read_byte(uint16_t address);
    uint16_t read_word(uint16_t address);
    int reset();
    int emulate_frame();
    int IE; //interrput enable register
    int IF; //interrupt flag register
    std::unique_ptr<c_sm83> cpu;
    std::unique_ptr<c_gbapu> apu;
    std::unique_ptr<c_gbppu> ppu;
    uint32_t *get_fb();
    void clock_timer();
    void set_vblank_irq(int status);
    void set_input(int input);
    void set_stat_irq(int status);

    int is_loaded()
    {
        return loaded;
    }
    int get_crc()
    {
        return 0;
    }
    int *get_video();

    int get_sound_bufs(const short **buf_l, const short **buf_r);
    void set_audio_freq(double freq);

    void enable_mixer();
    void disable_mixer();

    GB_MODEL get_model() const
    {
        return model;
    }

  private:
    std::unique_ptr<c_gbmapper> mapper;
    std::unique_ptr<uint8_t[]> ram;
    std::unique_ptr<uint8_t[]> hram;
    std::unique_ptr<uint8_t[]> rom;
    int SB; //serial transfer data
    int SC; //serial transfer control

    uint8_t DIV;  //divider register
    uint8_t TIMA; //timer counter
    uint8_t TMA; //timer modulo
    uint8_t TAC; //timer control
    uint8_t JOY;
    int divider;
    int last_TAC_out;
    int input;
    int next_input;

    uint8_t cart_type;
    uint8_t rom_size;
    uint8_t header_ram_size;
    int ram_size;
    int wram_bank;
    char title[17] = {0};

    struct s_mapper
    {
        std::function<std::unique_ptr<c_gbmapper>()> mapper;
        int has_ram;
        int has_battery;
        int has_rumble;
    };

    int load_sram();
    int save_sram();

    const static std::map<int, s_mapper> mapper_factory;
    s_mapper *m;
    int loaded;
    char sramPath[MAX_PATH];

    int serial_transfer_count;
    int last_serial_clock;

    uint8_t read_wram(uint16_t address);
    void write_wram(uint16_t address, uint8_t data);
    uint8_t read_io(uint16_t address);
    void write_io(uint16_t address, uint8_t data);
    uint8_t read_joy();
    void write_joy(uint8_t data);

    GB_MODEL model;
    static const int RAM_SIZE = 32768;
};
