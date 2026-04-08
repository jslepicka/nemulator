module;

export module gb;
import nemulator.std;
import :sm83;
import :ppu;
import :mapper;
import :apu;
import :common;

import nemulator.buttons;
import system;
import class_registry;

namespace gb
{

export class c_gb : public c_system, register_class<system_registry, c_gb>
{
  public:
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
                .num_sound_channels = 2,
                .volume = pow(10.0f, -7.0f / 20.0f), //reduce by 7dB
                .constructor = []() { return std::make_unique<c_gb>(GB_MODEL::DMG); },
            },
            {
                .name = "Nintendo Game Boy Color",
                .identifier = "gbc",
                .display_info = display_info,
                .button_map = button_map,
                .num_sound_channels = 2,
                .volume = pow(10.0f, -7.0f / 20.0f),
                .constructor = []() { return std::make_unique<c_gb>(GB_MODEL::CGB); },
            },
        };
    }

    c_gb(GB_MODEL model)
    {
        cpu = std::make_unique<c_sm83<c_gb>>(*this);
        ppu = std::make_unique<c_gbppu<c_gb>>(*this);
        apu = std::make_unique<c_gbapu>();
        ram = std::make_unique<uint8_t[]>(RAM_SIZE);
        hram = std::make_unique<uint8_t[]>(128);
        loaded = 0;
        mapper = 0;
        ram_size = 0;
        wram_bank = 1;
        this->model = model;
    }

    ~c_gb()
    {
        if (loaded && pak_features & PAK_FEATURES::BATTERY) {
            save_sram();
        }
    }

    int load()
    {    
        //uint8_t logo[] = {
        //0xCE,0xED,0x66,0x66,0xCC,0x0D,0x00,0x0B,
        //0x03,0x73,0x00,0x83,0x00,0x0C,0x00,0x0D,
        //0x00,0x08,0x11,0x1F,0x88,0x89,0x00,0x0E,
        //0xDC,0xCC,0x6E,0xE6,0xDD,0xDD,0xD9,0x99,
        //0xBB,0xBB,0x67,0x63,0x6E,0x0E,0xEC,0xCC,
        //0xDD,0xDC,0x99,0x9F,0xBB,0xB9,0x33,0x3E
        //};
        std::ifstream file;
        file.open(path_file, std::fstream::in | std::fstream::binary);
        if (file.fail())
            return false;
        file.seekg(0, std::ios_base::end);
        int file_length = (int)file.tellg();

        //printf("read %d bytes\n", file_length);
        file.seekg(0, std::ios_base::beg);
        rom = std::make_unique_for_overwrite<uint8_t[]>(std::max(32768, file_length));
        //memcpy(rom + 0x104, logo, sizeof(logo));

        file.read((char *)rom.get(), file_length);
        file.close();

        std::memcpy(title, rom.get() + 0x134, 16);
        cart_type = *(rom.get() + 0x147);
        rom_size = *(rom.get() + 0x148);
        header_ram_size = *(rom.get() + 0x149);

        if (file_length != (32768 << rom_size)) {
            return 0;
        }

        auto &r = mapper_registry::get_registry();
        auto m = r.find(cart_type);
        if (m == r.end()) {
            return false;
        }
        mapper = (m->second).constructor();
        pak_features = (m->second).pak_features;

        mapper->rom = rom.get();

        if (pak_features & PAK_FEATURES::RAM && header_ram_size && header_ram_size < 6) {
            const int size[] = {0, 2 * 1024, 8 * 1024, 32 * 1024, 128 * 1024, 64 * 1024};
            ram_size = size[header_ram_size];
            mapper->config_ram(ram_size);
            if (pak_features & PAK_FEATURES::BATTERY) {
                load_sram();
            }
        }
        else if (cart_type == 5 || cart_type == 6) {
            //mbc2 has 512x4bits of RAM
            ram_size = 512;
            mapper->config_ram(ram_size);
            if (cart_type == 6)
                load_sram();
        }
        else {
            //disable battery save if anything is invalid
            pak_features &= ~PAK_FEATURES::BATTERY;
        }

        if (pak_features & PAK_FEATURES::RUMBLE) {
            mapper->rumble = true;
        }

        reset();
        loaded = 1;
        return file_length;
    }

    void write_byte(uint16_t address, uint8_t data)
    {
        switch (address >> 12) {
            case 0: //0000 - 3FFF
            case 1:
            case 2:
            case 3:
            case 4: //4000 - 7FFF
            case 5:
            case 6:
            case 7:
                mapper->write_byte(address, data);
                break;
            case 8:
            case 9:
                ppu->write_byte(address, data);
                break;
            case 0xA:
            case 0xB:
                mapper->write_byte(address, data);
                break;
            case 0xC:
            case 0xD:
                write_wram(address, data);
                break;
            case 0xE:
            case 0xF:
                if (address <= 0xFDFF) {
                    write_wram(address, data);
                }
                else if (address <= 0xFE9F) {
                    //OAM
                    ppu->write_byte(address, data);
                }
                else if (address <= 0xFEFF) {
                    //unusable
                }
                else if (address <= 0xFF7F) {
                    write_io(address, data);
                }
                else if (address <= 0xFFFE) {
                    hram[address - 0xFF80] = data;
                }
                else {
                    IE = data;
                }
                break;
            default: {
                int x = 1;
            } break;
        }
    }

    void write_word(uint16_t address, uint16_t data)
    {
        write_byte(address, data & 0xFF);
        write_byte(address + 1, data >> 8);
    }

    uint8_t read_byte(uint16_t address)
    {
        switch (address >> 12) {
            case 0: //0000 - 3FFF
            case 1:
            case 2:
            case 3:
            case 4: //4000 - 7FFF
            case 5:
            case 6:
            case 7:
                return mapper->read_byte(address);
                break;
            case 8:
            case 9:
                return ppu->read_byte(address);
                break;
            case 0xA:
            case 0xB:
                return mapper->read_byte(address);
                break;
            case 0xC:
            case 0xD:
                return read_wram(address);
                break;
            case 0xE:
            case 0xF:
                if (address <= 0xFDFF) {
                    return read_wram(address);
                }
                else if (address <= 0xFE9F) {
                    //OAM
                    return 0;
                }
                else if (address <= 0xFEFF) {
                    //unusable
                    return 0;
                }
                else if (address <= 0xFF7F) {
                    return read_io(address);
                }
                else if (address <= 0xFFFE) {
                    return hram[address - 0xFF80];
                }
                else {
                    return IE;
                }
                break;
            default:
                return 0;
        }
    }

    uint16_t read_word(uint16_t address)
    {
        uint8_t lo = read_byte(address);
        uint8_t hi = read_byte(address + 1);
        return lo | (hi << 8);
    }

    int reset()
    {
        cpu->reset(0x100);
        mapper->reset();
        ppu->reset();
        apu->reset();
        IE = 0;
        IF = 0;
        SB = 0;
        SC = 0;

        DIV = 0;
        TIMA = 0;
        TMA = 0;
        TAC = 0;
        divider = 0;
        last_TAC_out = 0;
        next_input = input = -1;

        JOY = 0xCF;
        serial_transfer_count = 0;
        last_serial_clock = 0;
        wram_bank = 1;

        std::memset(ram.get(), 0, RAM_SIZE);
        std::memset(hram.get(), 0, 128);

        return 0;
    }

    int emulate_frame()
    {
        static int frame_count = 0;
        apu->clear_buffers();
        //70224 PPU cycles per scanline
        //divided by 4 to get CPU clock
        //for scanline renderer:
        //456 PPU cycles
        //114 CPU cycles
        //154 times
        //4.2MHz master clock
        //    divided by 2 to get ppu memory clock = 2.1MHz
        //    divided by 4 to get cpu clock = 1.05MHz
        for (int line = 0; line < 154; line++) {
            //update input on line 144, right before vblank
            //Wizards & Warriors reads input at line 16 and 144 when
            //paused.  If input is updated too early, the second read at 144
            //overwrites the change detected on line 16, and makes it
            //impossible to resume from pause.
            if (line == 0x90) {
                input = next_input;
                read_joy();
            }
            ppu->execute(456);
        }
        if (++frame_count == 60) {
            int x = 1;
        }
        return 0;
    }

    uint32_t *get_fb()
    {
        return ppu->get_fb();
    }

    void clock_timer()
    {    
        //this is clocked @ 4.2MHz.  Should be ok to simply increment by 4
        //since all CPU cycles are multiples of 4, and side effects of incrementing
        //timer (register updates an interrupts) are only detectable on cpu cycle
        //boundaries.

        int TAC_out;
        int serial_clock;
        int divisors[] = {0x200, 0x08, 0x20, 0x80};
        divider += 4;
        TAC_out = divider & divisors[TAC & 0x3];

        //clock serial output @ 8kHz
        serial_clock = divider & 0x100;

        if (last_serial_clock && (!serial_clock)) {
            if (serial_transfer_count) {
                if (--serial_transfer_count == 0) {
                    IF |= 0x8;
                    SC &= ~0x80;
                }
            }
        }
        last_serial_clock = serial_clock;
        if (TAC_out && (TAC & 0x4)) {
            TAC_out = 1;
        }
        else {
            TAC_out = 0;
        }
        if (last_TAC_out && (!TAC_out)) {
            if (TIMA == 0xFF) {
                TIMA = TMA;
                IF |= 0x4;
            }
            else {
                TIMA = (TIMA + 1) & 0xFF;
            }
        }
        last_TAC_out = TAC_out;
    }

    void clock_cpu()
    {
        cpu->execute(4);
    }
    void clock_apu()
    {
        apu->clock();
    }
    void set_vblank_irq(int status)
    {
        if (status) {
            IF |= 1;
        }
        else {
            IF &= ~1;
        }
    }

    void set_input(int input)
    {
        next_input = ~input;
    }

    void set_stat_irq(int status)
    {
        if (status) {
            IF |= 2;
        }
        else {
            IF &= ~2;
        }
    }

    int is_loaded()
    {
        return loaded;
    }

    int *get_video()
    {
        return (int *)ppu->get_fb();
    }

    int get_sound_buf(const float **buf)
    {
        return apu->get_buffer(buf);
    }

    void set_audio_freq(double freq)
    {
        apu->set_audio_rate(freq);
    }

    void enable_mixer()
    {
        apu->enable_mixer();
    }

    void disable_mixer()
    {
        apu->disable_mixer();
    }

    GB_MODEL get_model() const
    {
        return model;
    }

  private:
    int load_sram()
    {
        std::ifstream file;
        file.open(sram_path_file, std::fstream::in | std::fstream::binary);
        if (file.fail()) {
            std::fill_n(mapper->ram.get(), ram_size, 0xFF);
            return 0;
        }
        file.seekg(0, std::ios_base::end);
        int file_length = (int)file.tellg();
        file.seekg(0, std::ios_base::beg);

        if (file_length != ram_size) {
            return 0;
        }

        file.read((char *)mapper->ram.get(), ram_size);
        file.close();
        return 1;
    }

    int save_sram()
    {
        std::ofstream file;
        file.open(sram_path_file, std::fstream::trunc | std::fstream::binary);
        if (file.fail()) {
            return 0;
        }

        file.write((char *)mapper->ram.get(), ram_size);
        file.close();
        return 1;
    }

    uint8_t read_wram(uint16_t address)
    {
        if (address & 0x1000) {
            return ram[(address & 0xFFF) + (wram_bank * 0x1000)];
        }
        else {
            return ram[address & 0xFFF];
        }
    }

    void write_wram(uint16_t address, uint8_t data)
    {
        if (address & 0x1000) {
            ram[(address & 0xFFF) + (wram_bank * 0x1000)] = data;
        }
        else {
            ram[address & 0xFFF] = data;
        }
    }

    uint8_t read_io(uint16_t address)
    {
        switch ((address >> 4) & 0x7) {
            case 0:
                switch (address & 0xF) {
                    case 0:
                        return read_joy();
                    case 1:
                        //serial
                        return 0xFF;
                    case 2:
                        //serial
                        return SC;
                    case 4:
                        return (divider & 0xFF00) >> 8;
                    case 5:
                        return TIMA;
                    case 6:
                        return TMA;
                    case 7:
                        return TAC;
                    case 0xF:
                        return IF;
                    default:
                        return 0;
                }
                break;
            case 1:
            case 2:
            case 3:
                return apu->read_byte(address);
            case 4:
            case 5:
            case 6:
                return ppu->read_byte(address);
            case 7:
                if (address == 0xFF70) {
                    return wram_bank;
                }
                else {
                    return 0;
                }
            default:
                return 0;
        }
    }

    void write_io(uint16_t address, uint8_t data)
    {
        switch ((address >> 4) & 0x7) {
            case 0:
                switch (address & 0xF) {
                    case 0:
                        write_joy(data);
                        break;
                    case 1:
                        //serial
                        SB = data;
                        break;
                    case 2:
                        //serial
                        SC = data;
                        if ((SC & 0x81) == 0x81) {
                            serial_transfer_count = 8;
                        }
                        break;
                    case 4:
                        divider = 0;
                        break;
                    case 5:
                        TIMA = data;
                        break;
                    case 6:
                        TMA = data;
                        break;
                    case 7:
                        TAC = data;
                        break;
                    case 0xF:
                        IF = data;
                        break;
                    default:
                        break;
                }
                break;
            case 1:
            case 2:
            case 3:
                apu->write_byte(address, data);
                break;
            case 4:
            case 5:
            case 6:
                ppu->write_byte(address, data);
                break;
            case 7:
                if (address == 0xFF70) {
                    if (model == GB_MODEL::CGB) {
                        wram_bank = data & 0x7;
                        if (wram_bank == 0) {
                            wram_bank = 1;
                        }
                    }
                }
                else {
                    //nothing
                    int x = 1;
                }
                break;
            default:
                break;
        }
    }

    uint8_t read_joy()
    {
        uint8_t prev = JOY & 0xF;
        switch (JOY & 0x30) {
            case 0x30:
                JOY = (JOY & 0xF0) | 0xF;
                break;
            case 0x20:
                JOY = (JOY & 0xF0) | (input & 0xF);
                break;
            case 0x10:
                JOY = (JOY & 0xF0) | ((input >> 4) & 0xF);
                break;
            case 0:
                JOY = (JOY & 0xF0) | ~((~(input & 0xF)) | (~((input >> 4) & 0xF)));
                break;
        }
        JOY |= 0xC0;

        uint8_t next = JOY & 0xF;

        if ((prev ^ next) & ~next) {
            IF |= 0x10;
        }

        return JOY;
    }

    void write_joy(uint8_t data)
    {
        JOY = (JOY & (~0x30)) | (data & 0x30);
        read_joy();
    }


  public:
    int IE; //interrput enable register
    int IF; //interrupt flag register
    std::unique_ptr<c_sm83<c_gb>> cpu;
    std::unique_ptr<c_gbapu> apu;
    std::unique_ptr<c_gbppu<c_gb>> ppu;

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

    int pak_features;
    int loaded;
    int serial_transfer_count;
    int last_serial_clock;

    GB_MODEL model;
    static const int RAM_SIZE = 32768;
};

} //namespace gb