module;

export module gb;
import nemulator.std;
import :sm83;
import :ppu;
import :mapper;
import :apu;

import nemulator.buttons;
import system;
import class_registry;

namespace gb
{

export enum GB_MODEL
{
    DMG,
    CGB
};

export class c_gb : public c_system, register_class<system_registry, c_gb>
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

    int *get_video();

    int get_sound_bufs(const float **buf_l, const float **buf_r);
    void set_audio_freq(double freq);

    void enable_mixer();
    void disable_mixer();

    GB_MODEL get_model() const
    {
        return model;
    }

    static std::vector<s_system_info> get_registry_info()
    {
        // clang-format off
        static const std::vector<s_button_map> button_map = {
            {BUTTON_1RIGHT,  0x01},
            {BUTTON_1LEFT,   0x02},
            {BUTTON_1UP,     0x04},
            {BUTTON_1DOWN,   0x08},
            {BUTTON_1A,      0x10},
            {BUTTON_1B,      0x20},
            {BUTTON_1SELECT, 0x40},
            {BUTTON_1START,  0x80},
        };
        // clang-format on
        static const s_system_info::s_display_info display_info = {
            .fb_width = 160,
            .fb_height = 144,
            .aspect_ratio = 4.7 / 4.3,
        };
        return {
            {
                .name = "Nintendo Game Boy",
                .identifier = "gb",
                .display_info = display_info,
                .button_map = button_map,
                .volume = pow(10.0f, -7.0f / 20.0f), //reduce by 7dB
                .constructor = []() { return std::make_unique<c_gb>(GB_MODEL::DMG); },
            },
            {
                .name = "Nintendo Game Boy Color",
                .identifier = "gbc",
                .display_info = display_info,
                .button_map = button_map,
                .volume = pow(10.0f, -7.0f / 20.0f),
                .constructor = []() { return std::make_unique<c_gb>(GB_MODEL::CGB); },
            },
        };
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

    int load_sram();
    int save_sram();

    int pak_features;
    int loaded;
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

} //namespace gb