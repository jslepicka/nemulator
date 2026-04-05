module;

export module sms;
import nemulator.std;

import :vdp;
import :common;
export import :psg;
import :crc;
import nemulator.buttons;
import system;
import class_registry;
import bus;
import z80;

namespace sms
{

export class c_sms : public c_system, register_class<system_registry, c_sms>, public i_z80_callbacks<c_sms>
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
            {BUTTON_1B,              0x10}, //button 1
            {BUTTON_1A,              0x20}, //button 2
            {BUTTON_2UP,             0x40},
            {BUTTON_2DOWN,           0x80},
            {BUTTON_2LEFT,          0x100},
            {BUTTON_2RIGHT,         0x200},
            {BUTTON_2B,             0x400}, //button 1
            {BUTTON_2A,             0x800}, //button 2
            {BUTTON_SMS_PAUSE, 0x80000000},
        };
        // clang-format on

        return {
            {
                .name = "Sega Master System",
                .identifier = "sms",
                .display_info =
                    {
                        .fb_width = 256,
                        .fb_height = 192,
                        .crop_top = -14,
                        .crop_bottom = -14,
                    },
                .button_map = button_map,
                .volume = pow(10.0f, -1.0f / 20.0f),
                .constructor = []() { return std::make_unique<c_sms>(SMS_MODEL::SMS); },
            },
            {
                .name = "Sega Game Gear",
                .identifier = "gg",
                .display_info =
                    {
                        .fb_width = 256,
                        .fb_height = 192,
                        .crop_left = 48,
                        .crop_right = 48,
                        .crop_top = 24,
                        .crop_bottom = 24,
                    },
                .button_map = button_map,
                .volume = pow(10.0f, -1.0f / 20.0f),
                .constructor = []() { return std::make_unique<c_sms>(SMS_MODEL::GAMEGEAR); },
            },
        };
    }
    c_sms(SMS_MODEL model);
    ~c_sms();
    int emulate_frame();

    s_bus<uint16_t> bus;
    s_bus<uint8_t> io_bus;

    uint8_t read_byte(uint16_t address);
    void write_byte(uint16_t address, uint8_t value);
    uint16_t read_word(uint16_t address);
    void write_word(uint16_t address, uint16_t value);
    void write_port(uint8_t port, uint8_t value);
    uint8_t read_port(uint8_t port);

    uint8_t _z80_read_byte(uint16_t address)
    {
        return read_byte(address);
    }
    void _z80_write_byte(uint16_t address, uint8_t data)
    {
        write_byte(address, data);
    }
    uint8_t _z80_read_port(uint8_t port)
    {
        return read_port(port);
    }
    void _z80_write_port(uint8_t port, uint8_t data)
    {
        write_port(port, data);
    }
    void _z80_int_ack()
    {
    }

    int reset();
    int *get_video();
    int get_sound_buf(const float **buf);
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
    SMS_MODEL get_model() const
    {
        return model;
    }

    //static std::vector<s_system_info> get_registry_info();


  private:
    SMS_MODEL model;
    int psg_cycles;
    int has_sram = 0;
    int joy = 0xFF;
    int loaded = 0;
    int ram_select;
    int nationalism;
    uint8_t data_bus = 0xFF;
    std::unique_ptr<c_z80<c_sms>> z80;
    std::unique_ptr<i_vdp> vdp;
    std::unique_ptr<c_psg> psg;
    std::unique_ptr<unsigned char[]> ram;
    std::unique_ptr<unsigned char[]> rom;
    int file_length;
    unsigned char *page[3];
    unsigned char cart_ram[16384];
    int load_sram();
    int save_sram();
    void catchup_psg();
    uint64_t last_psg_run;
    int codemasters;
};

} //namespace sms