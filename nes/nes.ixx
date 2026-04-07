module;
#include <cassert>
export module nes;
import nemulator.std;
import :ppu;
import :apu;
import :cpu;
import :joypad;
import :cartdb;
import nemulator.buttons;
import system;
import class_registry;
import :mapper;
import :ines;
import :game_genie;
import :callbacks;
import crc32;

namespace nes
{

export class c_nes : public c_system, register_class<system_registry, c_nes>, public i_nes_callbacks<c_nes>
{
    friend class i_nes_callbacks<c_nes>;
  public:
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

    c_nes()
    {
        ram = 0;
        sram = 0;
        cpu = 0;
        ppu = 0;
        mapper = 0;
        image = 0;
        joypad = 0;
        loaded = false;
        limit_sprites = false;
        crc32 = 0;
        game_genie = std::make_unique<c_game_genie>();
        mapper_info = 0;
    }

    int reset()
    {
        if (!ram) {
            ram = std::make_unique_for_overwrite<unsigned char[]>(2048);
        }
        std::memset(ram.get(), 0xFF, 2048);

        mapper->close_sram();
        apu->reset();
        ppu->reset();

        ppu->set_sprite_limit(limit_sprites);
        mapper->execute_irq = [&]() { cpu.get()->execute_irq(); };
        mapper->clear_irq = [&]() { cpu.get()->clear_irq(); };
        mapper->ppu_get_drawing_bg = [&]() { return ppu.get()->drawing_bg; };
        mapper->ppu_get_sprite_size = [&]() { return ppu.get()->get_sprite_size(); };
        mapper->header = header;
        mapper->image = image.get();
        if (mapper->load_image() == -1) {
            return 1;
        }
        mapper->reset();
        cpu->reset();
        joypad->reset();
        loaded = true;

        //game_genie->add_code("IPVGZGZE");
        //game_genie->add_code("LEIIXZ");
        //game_genie->add_code("GXVAAASA");
        ppu_cycle = 0;
        return 1;
    }

    int emulate_frame()
    {
        if (!loaded)
            return 1;
        apu->clear_buffer();
        for (int scanline = 0; scanline < 262; scanline++) {
            if (scanline == 261 || (scanline >= 0 && scanline <= 239)) {
                ppu->eval_sprites();
            }
            ppu->run_ppu_line();
        }
        return 0;
    }

    void set_audio_freq(double freq)
    {
        apu->set_audio_rate(freq);
    }

    int get_nwc_time()
    {
        return mapper->get_nwc_time();
    }

    int is_loaded()
    {
        return loaded;
    }

    void set_input(int input)
    {
        joypad->joy1 = input & 0xFF;
        joypad->joy2 = (input >> 8) & 0xFF;
    }

    int *get_video()
    {
        return ppu->get_frame_buffer();
    }

    int get_sound_buf(const float **buf)
    {
        return apu->get_buffer(buf);
    }

    unsigned char _dmc_read(unsigned short address)
    {
        cpu->execute_apu_dma();
        return _read_byte(address);
    }

    int load()
    {
        int submapper = -1;
        cpu = std::make_unique<c_cpu<c_nes>>(*this);
        joypad = std::make_unique<c_joypad>();
        apu = std::make_unique<c_apu<c_nes>>(*this);

        mapperNumber = LoadImage(path_file);

        auto cartdb_entry = cartdb.find(crc32);
        if (cartdb_entry != cartdb.end()) {
            s_cartdb c = cartdb_entry->second;
            if (c.mapper != -1) {
                mapperNumber = c.mapper;
            }
            if (c.submapper != -1) {
                submapper = c.submapper;
            }
            if (c.mirroring != -1) {
                switch (c.mirroring) {
                    case 0:
                    case 1:
                        header->Rcb1.Mirroring = c.mirroring;
                        break;
                    case 4:
                        header->Rcb1.Fourscreen = 1;
                }
            }
        }

        auto &r = nes_mapper_registry::get_registry();
        auto m = r.find(mapperNumber);
        if (m == r.end())
            return 0;
        mapper = (m->second).constructor();
        mapper_info = &(m->second);
        if (submapper != -1) {
            mapper->set_submapper(submapper);
        }
        mapper->image_path = path;
        mapper->sramFilename = sram_path_file;
        mapper->crc32 = crc32;
        mapper->file_length = file_length;
        ppu = std::make_unique<c_ppu<c_nes>>(*this);
        ppu->clock_mapper = mapper_info->needs_clock;
        ppu->ppu_cycle = &ppu_cycle;
        mapper->ppu_cycle = &ppu_cycle;

        reset();
        return 1;
    }

    std::string &get_mapper_name()
    {
        static std::string unknown = "Unknown";
        if (mapper_info) {
            return mapper_info->name;
        }
        else {
            return unknown;
        }
    }

    int get_mapper_number()
    {
        return mapperNumber;
    }

    int get_mirroring_mode()
    {
        if (mapper)
            return mapper->get_mirroring();
        else
            return 0;
    }

    void set_sprite_limit(bool limit_sprites)
    {
        this->limit_sprites = limit_sprites;
        ppu->set_sprite_limit(limit_sprites);
    }

    bool get_sprite_limit()
    {
        if (ppu) {
            return ppu->get_sprite_limit();
        }
        return false;
    }

    void set_submapper(int submapper)
    {
        mapper->set_submapper(submapper);
    }

    void _write_byte(uint16_t address, uint8_t value)
    {
        switch (address >> 12) {
            case 0:
            case 1:
                ram[address & 0x7FF] = value;
                break;
            case 2:
            case 3:
                ppu->write_byte(address & 0x2007, value);
                break;
            case 4:
                if (address == 0x4014) {
                    cpu->do_sprite_dma(ppu->get_sprite_memory(), (value & 0xFF) << 8);
                }
                else if (address == 0x4016) {
                    joypad->write_byte(address, value);
                }
                else if (address >= 0x4000 && address <= 0x4017) {
                    apu->write_byte(address, value);
                }
                else
                    mapper->write_byte(address, value);
                break;
            default:
                mapper->write_byte(address, value);
                break;
        }
    }

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

    void enable_mixer()
    {
        apu->enable_mixer();
    }

    void disable_mixer()
    {
        apu->disable_mixer();
    }

    int switch_disk()
    {
        return mapper->switch_disk();
    }

  private:
    int LoadImage(std::string &pathFile)
    {
        std::ifstream file;
        int m = -1;

        file.open(pathFile, std::ios_base::in | std::ios_base::binary);
        if (file.fail())
            return -1;
        file.seekg(0, std::ios_base::end);
        file_length = (int)file.tellg();
        file.seekg(0, std::ios_base::beg);
        image = std::make_unique_for_overwrite<unsigned char[]>(file_length);
        file.read((char *)image.get(), file_length);
        file.close();
        if (file_length < sizeof(iNesHeader)) {
            return -1;
        }
        header = (iNesHeader *)image.get();

        char ines_signature[] = {'N', 'E', 'S', 0x1a};
        char fds_signature[] = "\x01*NINTENDO-HVC*";
        char fds_fwnes_signature[] = "FDS\x1A";
        char nsf_signature[] = {'N', 'E', 'S', 'M', 0x1A};

        if (std::memcmp(header->Signature, ines_signature, 4) == 0) {

        //if expected file size doesn't match real file size, try to fix chr rom page count
        //fixes fire emblem
            int expected_file_size =
                (header->PrgRomPageCount * 16384) + (header->ChrRomPageCount * 8192) + sizeof(iNesHeader);
            int actual_chr_size = file_length - (header->PrgRomPageCount * 16384) - sizeof(iNesHeader);

            if (file_length != expected_file_size && header->ChrRomPageCount != 0) {
                header->ChrRomPageCount = (actual_chr_size / 8192);
            }

            unsigned char *h = (unsigned char *)&header->Rcb2;

            if (*h & 0x0C)
                *h = 0;
            m = (header->Rcb1.mapper_lo) | (header->Rcb2.mapper_hi << 4);
        }
        else if (std::memcmp(image.get(), fds_signature, sizeof(fds_signature) - 1) == 0 ||
                 std::memcmp(image.get(), fds_fwnes_signature, sizeof(fds_fwnes_signature) - 1) == 0) {
            m = 0x103;
        //m = -1;
        }
        else if (std::memcmp(image.get(), nsf_signature, 5) == 0) {
            m = 0x102;
        }

        if (m != -1 && m != 0x102) {
            crc32 = get_crc32((unsigned char *)image.get() + sizeof(iNesHeader), file_length - sizeof(iNesHeader));
        }
        return m;
    }

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

  public:
    bool loaded;
    iNesHeader *header;
  private:
    uint64_t ppu_cycle;
    std::unique_ptr<c_cpu<c_nes>> cpu;
    std::unique_ptr<c_ppu<c_nes>> ppu;
    std::unique_ptr<c_mapper> mapper;
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
};

} //namespace nes