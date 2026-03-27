module;

export module nes;
import nemulator.std;
import :ppu;
import :apu;
import :cpu;
import :joypad;
import nemulator.buttons;
import system;
import class_registry;
import :mapper;
import :ines;
import :game_genie;
import :callbacks;

namespace nes
{

export class c_nes : public c_system, register_class<system_registry, c_nes>, public i_nes_callbacks<c_nes>
{
    friend class i_nes_callbacks<c_nes>;
  public:
    c_nes();
    ~c_nes();
    int reset();
    int emulate_frame();
    void set_audio_freq(double freq);
    int get_nwc_time();
    int is_loaded()
    {
        return loaded;
    }
    void set_input(int input);

    int *get_video();
    int get_sound_buf(const float **buf);
    bool loaded;
    unsigned char _dmc_read(unsigned short address);
    int load();
    std::string &get_mapper_name();
    int get_mapper_number();
    int get_mirroring_mode();
    void set_sprite_limit(bool limit_sprites);
    bool get_sprite_limit();
    void set_submapper(int submapper);
    std::unique_ptr<c_cpu<c_nes>> cpu;
    std::unique_ptr<c_ppu<c_nes>> ppu;
    std::unique_ptr<c_mapper> mapper;
    void _write_byte(uint16_t address, uint8_t value);
    uint8_t _read_byte(uint16_t address)
    {
        //unlimited health in holy diver
        //if (address == 0x0440) {
        //    return 6;
        //}
        //start battletoads on level 2
        //if (address == 0x8320) {
        //    return 0x2;
        //}
        //journey to silius unlimited energy
        //   if (address == 0x00B0) {
        //       return 0x0F;
        //   }
        ////journey to silius unlimited gun power
        //   if (address == 0x00B1) {
        //       return 0x3F;
        //   }
        switch (address >> 12) {
            case 0:
            case 1:
                return ram[address & 0x7FF];
                break;
            case 2:
            case 3:
            //if (address <= 0x2007)
                return ppu->read_byte(address & 0x2007);
                break;
            case 4:
                if (address >= 0x4020) {
                    return mapper->read_byte(address);
                }
                switch (address) {
                    case 0x4015:
                        return apu->read_byte(address);
                        break;
                    case 0x4016:
                    case 0x4017:
                        return joypad->read_byte(address);
                        break;
                    default:
                        break;
                }
                break;
            default:
                unsigned char val = mapper->read_byte(address);
                if (game_genie->count > 0) {
                    val = game_genie->filter_read(address, val);
                }
                return val;
                break;
        }
        return 0;
    }
    iNesHeader *header;
    void enable_mixer();
    void disable_mixer();

    static std::vector<s_system_info> get_registry_info()
    {
        // clang-format off
        static const std::vector<s_button_map> button_map = {
            {BUTTON_1A,       0x01},
            {BUTTON_1B,       0x02},
            {BUTTON_1SELECT,  0x04},
            {BUTTON_1START,   0x08},
            {BUTTON_1UP,      0x10},
            {BUTTON_1DOWN,    0x20},
            {BUTTON_1LEFT,    0x40},
            {BUTTON_1RIGHT,   0x80},
            {BUTTON_2A,      0x101},
            {BUTTON_2B,      0x102},
            {BUTTON_2SELECT, 0x104},
            {BUTTON_2START,  0x108},
            {BUTTON_2UP,     0x110},
            {BUTTON_2DOWN,   0x120},
            {BUTTON_2LEFT,   0x140},
            {BUTTON_2RIGHT,  0x180},
        };
        // clang-format on

        static const s_system_info::s_display_info display_info = {
            .fb_width = 256,
            .fb_height = 240,
            .crop_top = 8,
            .crop_bottom = 8,
        };

        return {
            {
                .name = "Nintendo NES",
                .identifier = "nes",
                .display_info = display_info,
                .button_map = button_map,
                .constructor = []() { return std::make_unique<c_nes>(); },
            },
            {
                .name = "Nintendo NES",
                .identifier = "nsf",
                .display_info = display_info,
                .button_map = button_map,
                .constructor = []() { return std::make_unique<c_nes>(); },
            },
            {
                .name = "Nintendo FDS",
                .identifier = "fds",
                .display_info = display_info,
                .button_map = button_map,
                .constructor = []() { return std::make_unique<c_nes>(); },
            },
        };
    }

  private:
    int LoadImage(std::string &pathFile);
    std::unique_ptr<c_apu<c_nes>> apu;
    std::unique_ptr<c_joypad> joypad;
    std::unique_ptr<c_game_genie> game_genie;
    std::unique_ptr<unsigned char[]> image;
    std::unique_ptr<unsigned char[]> ram;
    std::unique_ptr<unsigned char[]> sram;
    int mapperNumber;
    c_mapper::s_mapper_info *mapper_info;
    int file_length;
    bool limit_sprites;
    
    void _mapper_cpu_clock()
    {
        mapper->cpu_clock();
    }
    void _mapper_ppu_clock()
    {
        mapper->ppu_clock();
    }
    void _cpu_clock()
    {
        cpu->execute();
        cpu->odd_cycle ^= 1;
        apu->clock();
    }
    void _sprite_eval(bool in_eval)
    {
        mapper->in_sprite_eval = in_eval;
    }
    void _nmi(bool nmi)
    {
        if (nmi) {
            cpu->execute_nmi();
        }
        else {
            cpu->clear_nmi();
        }
    }
    uint8_t _ppu_read(uint16_t address)
    {
        return mapper->ppu_read(address);
    }
    void _ppu_write(uint16_t address, uint8_t value)
    {
        mapper->ppu_write(address, value);
    }
    void _add_cycle()
    {
        cpu->add_cycle();
    }
    void _irq(bool irq)
    {
        if (irq) {
            cpu->execute_irq();
        }
        else {
            cpu->clear_irq();
        }
    }
    float _mix_mapper_audio(float sample)
    {
        return mapper->mix_audio(sample);
    }
    };

} //namespace nes