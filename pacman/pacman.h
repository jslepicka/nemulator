#pragma once
#include "..\console.h"
//#include "..\z80\z80.h"
#include <memory>
#include <array>
#include <vector>

#define BIT(x, n) (((x) >> (n)) & 1)
#define BITSWAP16(val, B15, B14, B13, B12, B11, B10, B9, B8, B7, B6, B5, B4, B3, B2, B1, B0)                           \
    ((BIT(val, B15) << 15) | (BIT(val, B14) << 14) | (BIT(val, B13) << 13) | (BIT(val, B12) << 12) |                   \
     (BIT(val, B11) << 11) | (BIT(val, B10) << 10) | (BIT(val, B9) << 9) | (BIT(val, B8) << 8) | (BIT(val, B7) << 7) | \
     (BIT(val, B6) << 6) | (BIT(val, B5) << 5) | (BIT(val, B4) << 4) | (BIT(val, B3) << 3) | (BIT(val, B2) << 2) |     \
     (BIT(val, B1) << 1) | (BIT(val, B0) << 0))

class c_pacman_vid;
class c_pacman_psg;
class c_z80;

enum class PACMAN_MODEL
{
    PACMAN,
    MSPACMAN,
    MSPACMNF,
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
    int get_sound_bufs(const short **buf_l, const short **buf_r);
    void set_audio_freq(double freq);
    void set_input(int input);
    int *get_video();
    virtual ~c_pacman();
    
    void set_irq(int irq);
    void enable_mixer();
    void disable_mixer();
    static const std::vector<load_info_t> load_info;

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
    //void decrypt_mspacman();
    //void decrypt_rom(int src, int dst, int len, std::array<uint8_t, 16> addr_bits);
    //uint16_t bitswap(uint16_t in, std::array<uint8_t, 16>);
    //void check_mspacman_trap(uint16_t address);
    int load_romset(std::vector<s_roms> &romset);


    std::unique_ptr<c_z80> z80;
    std::unique_ptr<c_pacman_vid> pacman_vid;
    std::unique_ptr<c_pacman_psg> pacman_psg;
    std::unique_ptr<uint8_t[]> prg_rom;
    std::unique_ptr<uint8_t[]> prg_rom_overlay; //for ms. pacman
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