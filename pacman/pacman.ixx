module;

export module pacman;
import nemulator.std;

import :vid;
import :psg;
import system;
import class_registry;
import nemulator.buttons;

class c_z80;

namespace pacman
{

export enum class PACMAN_MODEL
{
    PACMAN,
    MSPACMAN,
    MSPACMNF,
    MSPACMAB
};

export class c_pacman : public c_system, register_class<system_registry, c_pacman>
{
  public:
    c_pacman(PACMAN_MODEL model = PACMAN_MODEL::PACMAN);
    virtual int load();
    int is_loaded();
    int emulate_frame();
    virtual int reset();
    int get_sound_bufs(const float **buf_l, const float **buf_r);
    void set_audio_freq(double freq);
    void set_input(int input);
    int *get_video();
    virtual ~c_pacman();

    void set_irq(int irq);
    void enable_mixer();
    void disable_mixer();

    static const std::vector<s_button_map> &get_button_map()
    {
        // clang-format off
        static const std::vector<s_button_map> button_map = {
            {BUTTON_1UP,     0x01},
            {BUTTON_1LEFT,   0x02},
            {BUTTON_1RIGHT,  0x04},
            {BUTTON_1DOWN,   0x08},
            {BUTTON_1SELECT, 0x20},
            {BUTTON_1START,  0x80},
        };
        // clang-format on
        return button_map;
    }

    static const s_system_info::s_display_info &get_display_info()
    {
        static const s_system_info::s_display_info display_info = {
            .fb_width = 288,
            .fb_height = 224,
            .rotation = 90,
            .aspect_ratio = 3.0 / 4.0,
        };
        return display_info;
    }

    static std::vector<s_system_info> get_registry_info()
    {
        return {
            {
                .is_arcade = 1,
                .name = "Arcade",
                .identifier = "pacman",
                .title = "Pac-Man",
                .display_info = get_display_info(),
                .button_map = get_button_map(),
                .constructor = []() { return std::make_unique<c_pacman>(); },
            },
            {
                .is_arcade = 1,
                .name = "Arcade",
                .identifier = "mspacmab",
                .title = "Ms. Pac-Man (Bootleg)",
                .display_info = get_display_info(),
                .button_map = get_button_map(),
                .constructor = []() { return std::make_unique<c_pacman>(PACMAN_MODEL::MSPACMAB); },
            },
        };
    }

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
} //namespace pacman