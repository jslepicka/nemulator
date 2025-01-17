#pragma once
#include "..\console.h"
#include <memory>

class c_z80;
class c_vdp;
class c_psg;

enum class SMS_MODEL
{
    SMS,
    GAMEGEAR
};

class c_sms : public c_console
{
public:
    c_sms(SMS_MODEL model);
    ~c_sms();
    int emulate_frame();
    unsigned char read_byte(unsigned short address);
    unsigned short read_word(unsigned short address);
    void write_byte(unsigned short address, unsigned char value);
    void write_word(unsigned short address, unsigned short value);
    void write_port(int port, unsigned char value);
    unsigned char read_port(int port);
    int reset();
    int *get_video();
    int get_sound_bufs(const short **buf_l, const short **buf_r);
    int irq;
    int nmi;
    void set_audio_freq(double freq);
    int load();
    int is_loaded() { return loaded; }
    int get_crc() { return crc; }
    void enable_mixer();
    void disable_mixer();
    void set_input(int input);
    SMS_MODEL get_model() const { return model; }

  private:
    SMS_MODEL model;
    int psg_cycles;
    int has_sram = 0;
    unsigned int crc = 0;
    int joy = 0xFF;
    int loaded = 0;
    int ram_select;
    int nationalism;
    std::unique_ptr<c_z80> z80;
    std::unique_ptr<c_vdp> vdp;
    std::unique_ptr<c_psg> psg;
    std::unique_ptr<unsigned char[]> ram;
    std::unique_ptr<unsigned char[]> rom;
    int file_length;
    unsigned char *page[3];
    unsigned char cart_ram[16384];
    int load_sram();
    int save_sram();
    void get_sram_path(char *path);
    char sram_file_name[MAX_PATH];
    void catchup_psg();
    uint64_t last_psg_run;
};

