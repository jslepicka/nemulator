module;
#include <cassert>
export module sms;
import nemulator.std;
import nemulator.buttons;
import crc32;
import :vdp;
import :common;
import :crc;
export import :psg;
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

    c_sms(SMS_MODEL model)
    {
        bus.ctx = this;
        bus.read_byte = &thunk<c_sms, &c_sms::read_byte>;
        bus.write_byte = &thunk<c_sms, &c_sms::write_byte>;

        io_bus.ctx = this;
        io_bus.read_byte = &thunk<c_sms, &c_sms::read_port>;
        io_bus.write_byte = &thunk<c_sms, &c_sms::write_port>;

        this->model = model;
        z80 = std::make_unique<c_z80<c_sms>>(*this, &nmi, &irq, &data_bus);
        if (model == SMS_MODEL::SMS) {
            vdp = std::make_unique<c_vdp<SMS_MODEL::SMS>>(&irq);
        }
        else if (model == SMS_MODEL::GAMEGEAR) {
            vdp = std::make_unique<c_vdp<SMS_MODEL::GAMEGEAR>>(&irq);
        }
        else {
            assert(0);
        }

        psg = std::make_unique<c_psg>();
        ram = std::make_unique<unsigned char[]>(8192);
        codemasters = 0;
    }

    ~c_sms()
    {
        if (has_sram)
            save_sram();
    }

    int emulate_frame()
    {
        psg->clear_buffer();
        for (int i = 0; i < 262; i++) {
            z80->execute(228);
            vdp->eval_sprites();
            vdp->draw_scanline();
        }
        catchup_psg();
        return 0;
    }

    uint8_t read_byte(uint16_t address)
    {
        if (address < 0xC000) {
            if (address < 0x400) //always read first 1k from rom
            {
                return rom[address];
            }
            else {
                int p = (address >> 14) & 0x3;
                if (p == 2 && (ram_select & 0x8))
                    return cart_ram[address & 0x1FFF];
                __assume(p < 3);
                return page[p][address & 0x3FFF];
            }
        }
        else {
            return ram[address & 0x1FFF];
        }
    }

    void write_byte(uint16_t address, uint8_t value)
    {
        if (codemasters) {
            int reg = address >> 14;
            if (reg < 3) {
                int o = (0x4000 * (value)) % file_length;
                page[reg] = rom.get() + o;
                return;
            }
        }
        if (address < 0xC000) {
            if (address >= 0x8000 && (ram_select & 0x8)) {
                cart_ram[address & 0x1FFF] = value;
            }
        }
        else {
        //ram
            if (!codemasters && address >= 0xFFFC) {
            //paging registers
                switch (address & 0x3) {
                    case 0:
                        ram_select = value;
                        break;
                    case 1:
                    case 2:
                    case 3: {
                        int p = (address & 0x3) - 1;
                        int o = (0x4000 * (value)) % file_length;
                        page[p] = rom.get() + o;
                    } break;
                }
            }
            address &= 0x1FFF;
            ram[address] = value;
        }
    }

    uint16_t read_word(uint16_t address)
    {
        int lo = read_byte(address);
        int hi = read_byte(address + 1);
        return lo | (hi << 8);
    }

    void write_word(uint16_t address, uint16_t value)
    {
        write_byte(address, value & 0xFF);
        write_byte(address + 1, value >> 8);
    }

    void write_port(uint8_t port, uint8_t value)
    {
        switch (port >> 6) {
            case 0:
                if (port == 0x3F) {
                    nationalism = value;
                }
                break;
            case 1:
                catchup_psg();
                psg->write(value);
                break;
            case 2:
                switch (port & 0x1) {
                    case 0: //VDP data
                        vdp->write_data(value);
                        break;
                    case 1: //VDP control
                        vdp->write_control(value);
                        break;
                }
                break;
            case 3:
                //printf("Port write to joypad: %02X\n", port);
                break;
            default:
                //printf("Port write error\n");
                break;
        }
    }

    uint8_t read_port(uint8_t port)
    {
        switch (port >> 6) {
            case 0:
                if (model == SMS_MODEL::GAMEGEAR) {
                    return joy >> 31;
                }
                return 0;
            case 1:
                //TODO: need to differentiate between even and odd reads to return either h or v vdp counters
                if (port & 0x1) {
                    int x = 1;
                }
                return vdp->get_scanline();
            case 2:
                //printf("Port read from VDP\n");
                switch (port & 0x1) {
                    case 0: //VDP data
                        //printf("Port write to VDP data\n");
                        return vdp->read_data();
                    case 1: //VDP control
                        //printf("Port write to VDP control\n");
                        return vdp->read_control();
                }
                return 0;
            case 3:
                //printf("Port read from joypad: %02X\n", port);
                if (port == 0xDC) {
                    return joy & 0xFF;
                }
                else if (port == 0xDD) {
                    //invert and select TH output enable bits
                    int out = (nationalism ^ 0xFF) & 0xA;
                    //and TH bits with TH values
                    out = nationalism & (out << 4);
                    //shift bits into position
                    out = (out & 0x80) | ((out << 1) & 0x40);
                    out = ((joy >> 8) & 0x3F) | out;
                    return out;
                }
                return 0xFF;
            default:
                //printf("Port read error\n");
                return 0;
        }
    }

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

    int reset()
    {
        z80->reset();
        vdp->reset();
        psg->reset();
        std::memset(ram.get(), 0x00, 8192);
        irq = 0;
        nmi = 0;
        page[0] = rom.get();
        page[1] = file_length > 0x4000 ? rom.get() + 0x4000 : rom.get();
        page[2] = file_length > 0x8000 ? rom.get() + 0x8000 : rom.get();
        nationalism = 0;
        ram_select = 0;
        joy = 0xFFFF;
        psg_cycles = 0;
        last_psg_run = 0;
        return 0;
    }

    int *get_video()
    {
        return vdp->get_frame_buffer();
    }

    int get_sound_buf(const float **buf)
    {
        return psg->get_buffer(buf);
    }

    void set_audio_freq(double freq)
    {
        psg->set_audio_rate(freq);
    }
    
    int load()
    {
        std::ifstream file;
        file.open(path_file, std::ios_base::in | std::ios_base::binary);
        if (file.fail())
            return 0;
        file.seekg(0, std::ios_base::end);
        file_length = (int)file.tellg();
        int has_header = (file_length % 1024) != 0;
        int offset = 0;
        if (has_header) {
            offset = file_length - ((file_length / 1024) * 1024);
            file_length = file_length - offset;
        }
        file.seekg(offset, std::ios_base::beg);
        if (file_length == 0 || (file_length & (file_length - 1)) || file_length < 0x2000) {
            return 0;
        }
        int alloc_size = file_length < 0x4000 ? 0x4000 : file_length;
        rom = std::make_unique_for_overwrite<unsigned char[]>(alloc_size);
        file.read((char *)rom.get(), file_length);
        file.close();
        crc32 = get_crc32(rom.get(), file_length);

        for (auto &c : crc_table) {
            if (c.crc == crc32 && c.has_sram) {
                has_sram = 1;
                load_sram();
            }
            if (c.crc == crc32 && c.codemasters) {
                codemasters = 1;
            }
        }

        if (file_length == 0x2000) {
            std::memcpy(rom.get() + 0x2000, rom.get(), 0x2000);
        }
        loaded = 1;
        reset();
        return file_length;
    }
    
    int is_loaded()
    {
        return loaded;
    }

    void enable_mixer()
    {
        psg->enable_mixer();
    }

    void disable_mixer()
    {
        psg->disable_mixer();
    }

    void set_input(int input)
    {
        if (model == SMS_MODEL::SMS)
            nmi = input & 0x8000'0000;
        joy = ~input;
    }

    SMS_MODEL get_model() const
    {
        return model;
    }

  private:
    int load_sram()
    {
        std::ifstream file;
        file.open(sram_path_file, std::ios_base::in | std::ios_base::binary);
        if (file.fail())
            return 1;

        file.seekg(0, std::ios_base::end);
        int l = (int)file.tellg();

        if (l != 8192)
            return 1;

        file.seekg(0, std::ios_base::beg);

        file.read((char *)cart_ram, 8192);
        file.close();
        return 1;
    }

    int save_sram()
    {
        std::ofstream file;
        file.open(sram_path_file, std::ios_base::out | std::ios_base::binary);
        if (file.fail())
            return 1;
        file.write((char *)cart_ram, 8192);
        file.close();
        return 0;
    }

    void catchup_psg()
    {
        int num_cycles = (int)(z80->retired_cycles - last_psg_run);
        last_psg_run = z80->retired_cycles;
        psg->clock(num_cycles);
    }

  private:
    int irq;
    int nmi;
    s_bus<uint16_t> bus;
    s_bus<uint8_t> io_bus;
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
    int codemasters;
    uint64_t last_psg_run;
    unsigned char *page[3];
    unsigned char cart_ram[16384];

};

} //namespace sms