module;

export module genesis;
import nemulator.std;
import nemulator.buttons;
import system;
import class_registry;
import z80;
import :vdp;

class c_m68k;
//class c_vdp;

namespace genesis
{

export class c_genesis : public c_system, register_class<system_registry, c_genesis>
{
  public:
    static std::vector<s_system_info> get_registry_info()
    {
        // clang-format off
        static const std::vector<s_button_map> button_map = {
            {BUTTON_1UP,             0x01},
            {BUTTON_1DOWN,           0x02},
            {BUTTON_1LEFT,           0x04},
            {BUTTON_1RIGHT,          0x08},
            {BUTTON_1A,              0x10}, //B button 1
            {BUTTON_1C,              0x20}, //C button 3
            {BUTTON_1B,            0x1000}, //A button 2
            {BUTTON_1START,        0x2000}
        };
        // clang-format on

        return {
            {
                .name = "Genesis",
                .identifier = "genesis",
                .extension = "bin",
                .display_info =
                    {
                        .fb_width = 320,
                        .fb_height = 224,
                        .crop_right = -1,
                    },
                .button_map = button_map,
                .constructor = []() { return std::make_unique<c_genesis>(); },
            }
        };
    }
    c_genesis();
    ~c_genesis();
    int emulate_frame();
    uint8_t read_byte(uint32_t address);
    uint16_t read_word(uint32_t address);
    void write_byte(uint32_t address, uint8_t value);
    void write_word(uint32_t address, uint16_t value);
    int reset();
    int *get_video();
    int get_sound_bufs(const short **buf_l, const short **buf_r);
    int irq;
    int nmi;
    void set_audio_freq(double freq);
    int load();
    int is_loaded()
    {
        return loaded;
    }
    void enable_mixer();
    void disable_mixer();
    void set_input(int input);

  private:
    int loaded = 0;
    std::unique_ptr<c_m68k> m68k;
    //std::unique_ptr<uint8_t[]> ram;
    std::unique_ptr<uint8_t[]> rom;
    std::unique_ptr<uint8_t[]> cart_ram;
    std::unique_ptr<c_vdp> vdp;
    std::unique_ptr<c_z80> z80;
    int file_length;
    uint8_t ipl;
    int last_bus_request;
    int z80_reset;
    int z80_busreq;
    int z80_irq;
    int z80_nmi;
    int z80_running;
    int z80_has_bus;
    uint32_t stalled;
    uint32_t th1;
    uint32_t th2;
    uint32_t joy1;
    uint32_t joy2;
    uint8_t ram[64 * 1024];
    uint8_t z80_ram[8 * 1024];
    uint32_t rom_size;
    uint32_t rom_mask;
    uint32_t rom_end;
    uint32_t cart_ram_start;
    uint32_t cart_ram_end;
    uint32_t cart_ram_size;
    uint32_t has_sram;

    void open_sram();
    void close_sram();
    

    void on_mode_switch(int x_res);
    uint8_t z80_read_byte(uint16_t address);
    void z80_write_byte(uint16_t address, uint8_t value);
    uint8_t z80_read_port(uint8_t port);
    void z80_write_port(uint8_t port, uint8_t value);
};

} //namespace genesis