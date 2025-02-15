#include "gb.h"
#include "gbapu.h"
#include "gbmapper.h"
#include "gbppu.h"
#include "mbc1.h"
#include "mbc2.h"
#include "mbc3.h"
#include "mbc5.h"
#include "sm83.h"
#include <algorithm>

void strip_extension(char *path);

// clang-format off
const std::map<int, c_gb::s_pak> c_gb::pak_factory = {
    {0,    {[]() { return std::make_unique<c_gbmapper>(); }, PAK_FEATURES::NONE}},
    {1,    {[]() { return std::make_unique<c_mbc1>(); },     PAK_FEATURES::NONE}},
    {2,    {[]() { return std::make_unique<c_mbc1>(); },     PAK_FEATURES::RAM}},
    {3,    {[]() { return std::make_unique<c_mbc1>(); },     PAK_FEATURES::RAM | PAK_FEATURES::BATTERY}},
    {5,    {[]() { return std::make_unique<c_mbc2>(); },     PAK_FEATURES::NONE}},
    {6,    {[]() { return std::make_unique<c_mbc2>(); },     PAK_FEATURES::BATTERY}},
    {0x19, {[]() { return std::make_unique<c_mbc5>(); },     PAK_FEATURES::NONE}},
    {0x1A, {[]() { return std::make_unique<c_mbc5>(); },     PAK_FEATURES::RAM}},
    {0x1B, {[]() { return std::make_unique<c_mbc5>(); },     PAK_FEATURES::RAM | PAK_FEATURES::BATTERY}},
    {0x1C, {[]() { return std::make_unique<c_mbc5>(); },     PAK_FEATURES::RUMBLE}},
    {0x1D, {[]() { return std::make_unique<c_mbc5>(); },     PAK_FEATURES::RAM | PAK_FEATURES::RUMBLE}},
    {0x1E, {[]() { return std::make_unique<c_mbc5>(); },     PAK_FEATURES::RAM | PAK_FEATURES::BATTERY | PAK_FEATURES::RUMBLE}},
};
// clang-format on

// clang-format off
const std::vector<c_system::load_info_t> c_gb::load_info = {
    {
        .game_type = GAME_GB,
        .extension = "gb",
        .constructor = []() { return new c_gb(GB_MODEL::DMG); },
    },
    {
        .game_type = GAME_GBC,
        .extension = "gbc",
        .constructor = []() { return new c_gb(GB_MODEL::CGB); },
    },
};
// clang-format on

c_gb::c_gb(GB_MODEL model)
{
    system_name = model == GB_MODEL::CGB ? "Nintendo Game Boy Color" : "Nintendo Game Boy";
    display_info.fb_width = 160;
    display_info.fb_height = 144;
    display_info.aspect_ratio = 4.7 / 4.3;

    cpu = std::make_unique<c_sm83>(this);
    ppu = std::make_unique<c_gbppu>(this);
    apu = std::make_unique<c_gbapu>(this);
    ram = std::make_unique<uint8_t[]>(RAM_SIZE);
    hram = std::make_unique<uint8_t[]>(128);
    loaded = 0;
    mapper = 0;
    ram_size = 0;
    wram_bank = 1;
    this->model = model;
    button_map = {
        { BUTTON_1RIGHT,  0x01},
        { BUTTON_1LEFT,   0x02},
        { BUTTON_1UP,     0x04},
        { BUTTON_1DOWN,   0x08},
        { BUTTON_1A,      0x10},
        { BUTTON_1B,      0x20},
        { BUTTON_1SELECT, 0x40},
        { BUTTON_1START,  0x80},
    };
}

c_gb::~c_gb()
{
    if (loaded && pak->features & PAK_FEATURES::BATTERY) {
        save_sram();
    }
}

int c_gb::reset()
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

    JOY = 0xFF;
    serial_transfer_count = 0;
    last_serial_clock = 0;
    wram_bank = 1;

    memset(ram.get(), 0, RAM_SIZE);
    memset(hram.get(), 0, 128);

    return 0;
}

int *c_gb::get_video()
{
    return (int *)ppu->get_fb();
}

void c_gb::set_audio_freq(double freq)
{
    apu->set_audio_rate(freq);
}

int c_gb::get_sound_bufs(const short **buf_l, const short **buf_r)
{
    return apu->get_buffers(buf_l, buf_r);
}

int c_gb::load()
{

    sprintf_s(pathFile, "%s\\%s", path, filename);
    char temp[MAX_PATH];
    sprintf_s(temp, "%s\\%s", sram_path, filename);
    strip_extension(temp);
    sprintf_s(sramPath, "%s.sav", temp);

   //uint8_t logo[] = {
  //0xCE,0xED,0x66,0x66,0xCC,0x0D,0x00,0x0B,
  //0x03,0x73,0x00,0x83,0x00,0x0C,0x00,0x0D,
  //0x00,0x08,0x11,0x1F,0x88,0x89,0x00,0x0E,
  //0xDC,0xCC,0x6E,0xE6,0xDD,0xDD,0xD9,0x99,
  //0xBB,0xBB,0x67,0x63,0x6E,0x0E,0xEC,0xCC,
  //0xDD,0xDC,0x99,0x9F,0xBB,0xB9,0x33,0x3E
  //};
    std::ifstream file;
    file.open(pathFile, std::fstream::in | std::fstream::binary);
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

    memcpy(title, rom.get() + 0x134, 16);
    cart_type = *(rom.get() + 0x147);
    rom_size = *(rom.get() + 0x148);
    header_ram_size = *(rom.get() + 0x149);

    if (file_length != (32768 << rom_size)) {
        return 0;
    }

    //printf("title: %s\n", title);
  //printf("cart_type: %d\n", cart_type);
  //printf("rom_size: %d\n", 32768 << rom_size);
  //printf("ram_size: %d\n", ram_size);
    auto i = pak_factory.find(cart_type);
    if (i == pak_factory.end()) {
        return false;
    }
    pak = (s_pak *)&(i->second);
    mapper = pak->mapper();

    mapper->rom = rom.get();

    if (pak->features & PAK_FEATURES::RAM && header_ram_size && header_ram_size < 6) {
        const int size[] = {0, 2 * 1024, 8 * 1024, 32 * 1024, 128 * 1024, 64 * 1024};
        ram_size = size[header_ram_size];
        mapper->config_ram(ram_size);
        if (pak->features & PAK_FEATURES::BATTERY) {
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
        pak->features &= ~PAK_FEATURES::BATTERY;
    }
    enum test
    {
        a = 0,
        b = 1,
        c = 2,
    };

    if (pak->features & PAK_FEATURES::RUMBLE) {
        mapper->rumble = true;
    }

    reset();
    loaded = 1;
    return file_length;
}

int c_gb::load_sram()
{
    std::ifstream file;
    file.open(sramPath, std::fstream::in | std::fstream::binary);
    if (file.fail()) {
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

int c_gb::save_sram()
{
    std::ofstream file;
    file.open(sramPath, std::fstream::trunc | std::fstream::binary);
    if (file.fail()) {
        return 0;
    }

    file.write((char *)mapper->ram.get(), ram_size);
    file.close();
    return 1;
}

uint16_t c_gb::read_word(uint16_t address)
{
    uint8_t lo = read_byte(address);
    uint8_t hi = read_byte(address + 1);
    return lo | (hi << 8);
}

uint8_t c_gb::read_wram(uint16_t address)
{
    if (address & 0x1000) {
        return ram[(address & 0xFFF) + (wram_bank * 0x1000)];
    }
    else {
        return ram[address & 0xFFF];
    }
}

void c_gb::write_wram(uint16_t address, uint8_t data)
{
    if (address & 0x1000) {
        ram[(address & 0xFFF) + (wram_bank * 0x1000)] = data;
    }
    else {
        ram[address & 0xFFF] = data;
    }
}

uint8_t c_gb::read_joy()
{
    if ((JOY & 0x30) == 0x20) {
        return 0xE0 | (input & 0xF);// | (JOY & 0x30);
    }
    else if ((JOY & 0x30) == 0x10) {
        return 0xD0 | ((input >> 4) & 0xF);// | (JOY & 0x30);
    }
    else {
        return 0xFF;
    }
}

void c_gb::write_joy(uint8_t data)
{
    JOY = (JOY & (~0x30)) | (data & 0x30);
}

uint8_t c_gb::read_io(uint16_t address)
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

void c_gb::write_io(uint16_t address, uint8_t data)
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

uint8_t c_gb::read_byte(uint16_t address)
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

void c_gb::write_word(uint16_t address, uint16_t data)
{
    write_byte(address, data & 0xFF);
    write_byte(address + 1, data >> 8);
}

void c_gb::write_byte(uint16_t address, uint8_t data)
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

void c_gb::clock_timer()
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

int c_gb::emulate_frame()
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
            if ((input & 0xFF) != 0xFF) {
                IF |= 0x10;
            }
        }
        ppu->execute(456);
    }
    if (++frame_count == 60) {
        int x = 1;
    }
    return 0;
}

uint32_t *c_gb::get_fb()
{
    return ppu->get_fb();
}

void c_gb::set_vblank_irq(int status)
{
    if (status) {
        IF |= 1;
    }
    else {
        IF &= ~1;
    }
}

void c_gb::set_stat_irq(int status)
{
    if (status) {
        IF |= 2;
    }
    else {
        IF &= ~2;
    }
}
void c_gb::set_input(int input)
{
    next_input = ~input;
}

void c_gb::enable_mixer()
{
    apu->enable_mixer();
}

void c_gb::disable_mixer()
{
    apu->disable_mixer();
}