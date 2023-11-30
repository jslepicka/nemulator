#pragma once
#include "..\console.h"
#include "..\z80\z80.h"

class c_pacman_vid;
class c_pacman_psg;

class c_pacman : public c_console
{
  public:
    c_pacman();
    virtual int load();
    int is_loaded();
    int emulate_frame();
    virtual int reset();
    int get_crc();
    int get_sound_bufs(const short **buf_l, const short **buf_r);
    void set_audio_freq(double freq);
    void set_input(int input);
    int get_fb_height() { return 512; }      
    int get_fb_width() { return 512; }
    int *get_video();
    virtual ~c_pacman();
    c_z80 *z80;
    void set_irq(int irq);
    void enable_mixer();
    void disable_mixer();
  protected:

    virtual uint8_t read_byte(uint16_t address);
    virtual void write_byte(uint16_t address, uint8_t data);
    uint8_t read_port(uint8_t port);
    void write_port(uint8_t port, uint8_t data);

    uint8_t *prg_rom;
    uint8_t *work_ram;

    int nmi;
    int irq;
    uint8_t data_bus;

    c_pacman_vid *pacman_vid;
    c_pacman_psg *pacman_psg;
    uint8_t IN0;
    uint8_t IN1;
    uint64_t frame_counter;
    int irq_enabled;
    int irq_input;
    int loaded;

};