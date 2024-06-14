#pragma once
#include "..\console.h"
#include "..\z80\z80.h"
#include <memory>

class c_pacman_vid;
class c_pacman_psg;

enum class PACMAN_MODEL
{
    PACMAN,
    MSPACMAB
};

class c_pacman : public c_console
{
  public:
    c_pacman(PACMAN_MODEL model = PACMAN_MODEL::PACMAN);
    virtual int load();
    int is_loaded();
    int emulate_frame();
    virtual int reset();
    int get_crc();
    int get_sound_bufs(const short **buf_l, const short **buf_r);
    void set_audio_freq(double freq);
    void set_input(int input);
    int *get_video();
    virtual ~c_pacman();
    
    void set_irq(int irq);
    void enable_mixer();
    void disable_mixer();

  protected:

    virtual uint8_t read_byte(uint16_t address);
    virtual void write_byte(uint16_t address, uint8_t data);
    uint8_t read_port(uint8_t port);
    void write_port(uint8_t port, uint8_t data);

    std::unique_ptr<c_z80> z80;
    std::unique_ptr<c_pacman_vid> pacman_vid;
    std::unique_ptr<c_pacman_psg> pacman_psg;
    std::unique_ptr<uint8_t[]> prg_rom;
    std::unique_ptr<uint8_t[]> work_ram;

    int nmi;
    int irq;
    uint8_t data_bus;

    
    uint8_t IN0;
    uint8_t IN1;
    uint64_t frame_counter;
    int irq_enabled;
    int irq_input;
    int loaded;

    PACMAN_MODEL model;
    int prg_mask;
};