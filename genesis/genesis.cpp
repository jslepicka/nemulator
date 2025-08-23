module;
#include <cassert>
#include <stdio.h>
#include <Windows.h>
module genesis;
import m68k;
import crc32;


namespace genesis
{

c_genesis::c_genesis()
{
    m68k = std::make_unique<c_m68k>(
        [this](uint32_t address) { return this->read_word(address); },
        [this](uint32_t address, uint16_t value) { this->write_word(address, value); },
        [this](uint32_t address) { return this->read_byte(address); },
        [this](uint32_t address, uint8_t value) { this->write_byte(address, value); },
        [this]() { this->vdp->ack_irq(); },
        &ipl,
        &stalled
    );
    vdp = std::make_unique<c_vdp>(&ipl,
        [this](uint32_t address) { return this->read_word(address); },
        [this](int x_res) { this->on_mode_switch(x_res); },
        &stalled
    );
    z80 = std::make_unique<c_z80>(
        [this](uint16_t address) { return this->z80_read_byte(address); },
        [this](uint16_t address, uint8_t value) { this->z80_write_byte(address, value); },
        [this](uint8_t port) { return this->z80_read_port(port); }, //read port
        [this](uint8_t port, uint8_t value) { this->z80_write_byte(port, value); }, //write port
        nullptr, //int ack
        &z80_nmi, //nmi
        &z80_irq, //irq
        nullptr  //data bus
    );
    cart_ram_start = 0;
    cart_ram_end = 0;
    z80_has_bus = 1;
    z80_reset = 1;
    has_sram = 0;
    is_ps4 = 0;
    ps4_ram_access = 0;
}

c_genesis::~c_genesis()
{
    close_sram();
}

uint8_t c_genesis::z80_read_port(uint8_t port)
{
    return 0;
}

void c_genesis::z80_write_port(uint8_t port, uint8_t value)
{
    int x = 1;
}

void c_genesis::on_mode_switch(int x_res)
{
    crop_right = 320 - x_res;
}

int c_genesis::load()
{
    std::ifstream file;
    file.open(path_file, std::ios_base::in | std::ios_base::binary);
    if (file.fail())
        return 0;
    file.seekg(0, std::ios_base::end);
    file_length = (int)file.tellg();
    rom_size = file_length;

    //round up rom_size to nearest power of 2
    if ((file_length & (file_length - 1)) != 0) {
        int v = 1;
        while (v <= file_length) {
            v <<= 1;
        }
        rom_size = v;
    }
    file.seekg(0, std::ios_base::beg);
    
    rom = std::make_unique_for_overwrite<uint8_t[]>(rom_size);
    
    file.read((char *)rom.get(), file_length);
    file.close();
    crc32 = get_crc32(rom.get(), file_length);
    rom_mask = rom_size - 1;

    rom_end = std::byteswap(*(uint32_t *)(rom.get() + 0x1A4));

    if (rom[0x1B0] == 'R' && rom[0x1B1] == 'A') {
        cart_ram_start = std::byteswap(*(uint32_t *)(rom.get() + 0x1B4));
        cart_ram_start &= ~1;
        cart_ram_end = std::byteswap(*(uint32_t *)(rom.get() + 0x1B8));
        cart_ram_end |= 1;
        cart_ram_size = cart_ram_end - cart_ram_start + 1;
        cart_ram = std::make_unique<uint8_t[]>(cart_ram_size);
        if (rom[0x1B2] & 0x40) {
            has_sram = 1;
            open_sram();
        }
    }
    const char ps4_title[] = "PHANTASY STAR The end of the millennium";
    if (memcmp(&rom[0x120], ps4_title, sizeof(ps4_title) - 1) == 0) {
        is_ps4 = 1;
    }

    reset();
    loaded = 1;
    return 1;
}

void c_genesis::open_sram()
{
    std::ifstream file;
    file.open(sram_path_file, std::ios_base::in | std::ios_base::binary);
    if (file.fail()) {
        return;
    }
    file.seekg(0, std::ios_base::end);
    int len = (int)file.tellg();
    if (len != cart_ram_size) {
        int x = 1;
        return;
    }
    file.seekg(0, std::ios_base::beg);
    file.read((char *)cart_ram.get(), len);
    file.close();
}

void c_genesis::close_sram()
{
    if (has_sram) {
        std::ofstream file;
        file.open(sram_path_file, std::ios_base::out | std::ios_base::binary);
        if (file.fail()) {
            return;
        }
        file.write((char *)cart_ram.get(), cart_ram_size);
        file.close();
    }
}

int c_genesis::reset()
{
    ipl = 0;
    m68k->reset();
    vdp->reset();
    z80->reset();
    last_bus_request = 0;
    stalled = 0;
    th1 = 0;
    th2 = 0;
    joy1 = -1;
    joy2 = -1;
    std::memset(ram, 0, sizeof(ram));
    std::memset(z80_ram, 0, sizeof(z80_ram));
    crop_right = 0;
    z80_reset = 0;
    z80_busreq = 0;
    z80_nmi = 0;
    z80_irq = 0;
    return 0;
}

int c_genesis::emulate_frame()
{
    uint32_t hblank_len = 50;
    for (int i = 0; i < 262; i++) {
        m68k->execute(488 - hblank_len);
        vdp->draw_scanline();
        m68k->execute(hblank_len);
        if (z80_has_bus && z80_reset) {
            z80->execute(228);
        }
        vdp->clear_hblank();
    }
    return 0;
}

uint8_t c_genesis::z80_read_byte(uint16_t address)
{
    //char buf[256];
    //sprintf(buf, "z80 read from %x\n", address);
    //OutputDebugString(buf);
    if (address < 0x4000) {
        return z80_ram[address & 0x1FFF];
    }
    else if (address < 0x6000) {
        //ym2162
        return 0;
    }
    else if (address < 0x6100) {
        //bank register
        return 0xFF;
    }
    else if (address < 0x7F00) {
        //unused
        return 0;
    }
    else if (address < 0x8000) {
        //vdp
        return 0;
    }
    else {
        return 0;
    }
}

void c_genesis::z80_write_byte(uint16_t address, uint8_t value)
{
    //char buf[256];
    //sprintf(buf, "z80 write %x to %x\n", value, address);
    //OutputDebugString(buf);
    if (address < 0x4000) {
        z80_ram[address & 0x1FFF] = value;
    }
    else if (address < 0x6000) {
        //ym2162
    }
    else if (address < 0x6100) {
        //bank register
        int x = 1;
    }
    else if (address < 0x7F00) {
        //unused
    }
    else if (address < 0x8000) {
        //vdp
    }
    else {
        int x = 1;
    }
}

uint8_t c_genesis::read_byte(uint32_t address)
{
    if (address < 0x400000) {
        if (is_ps4) {
            if (ps4_ram_access && address >= cart_ram_start && address <= cart_ram_end) {
                return cart_ram[address - cart_ram_start];
            }
            else {
                return rom[address & rom_mask];
            }
        }
        //probably not correct.  how is on-cart ram in this address space
        //handled?  is that even a thing?
        if (cart_ram_start && address >= cart_ram_start && address <= cart_ram_end) {
            return cart_ram[address - cart_ram_start];
        }
        else {
            return rom[address & rom_mask];
        }
    }
    else if (address >= 0x00C00000 && address <= 0xC0001E) {
        return vdp->read_byte(address);
    }
    else if (address >= 0xA00000 && address < 0xA10000) {
        //return 0;
        address &= 0x7FFF;
        if (address < 0x4000) {
            return z80_ram[address & 0x1FFF];
        }
        else {
            return 0;
        }
    }
    else if (address < 0xE00000) {
        switch (address) {
            case 0xA10001:
                return 0xA0;
            case 0xA10003:
                if (joy1 != 0xffffffff) {
                    int x = 1;
                }
                if (th1 & 0x40) {
                    uint16_t ret = 0x40 | (joy1 & 0x3F);

                    return ret;
                }
                else {
                    uint16_t ret = (joy1 & 0x3) | ((joy1 >> 8) & 0x30);
                    if (ret != 0) {
                        int x = 1;
                    }
                    return ret;
                }
                return 0x00;
            case 0xA10005:
                if (th2 & 0x40) {
                    uint16_t ret = 0x40 | (0xFF & 0x3F);

                    return ret;
                }
                else {
                    uint16_t ret = (0xFF & 0x3) | ((0xFFFF >> 8) & 0x30);
                    if (ret != 0) {
                        int x = 1;
                    }
                    return ret;
                }
                return 0x00;
            case 0xA11100:
            case 0xA11101:
                //68k bus access? incomplete
                //return last_bus_request;
                return (z80_has_bus && z80_reset);
            case 0xA11200:
            case 0xA11201:
                return z80_reset;
            default:
                return 0x00;
        }
    }
    else {
        return ram[address & 0xFFFF];
    }
    return 0;
}

uint16_t c_genesis::read_word(uint32_t address)
{
    assert(!(address & 1));
    if (address < 0x400000) {
        if (is_ps4) {
            if (ps4_ram_access && address >= cart_ram_start && address <= cart_ram_end) {
                return std::byteswap(*(uint16_t *)(cart_ram.get() + address - cart_ram_start));
            }
            else {
                address &= rom_mask;
                return std::byteswap(*((uint16_t *)(rom.get() + address)));
            }
        }
        if (cart_ram_start && address >= cart_ram_start && address <= cart_ram_end) {
            assert(0);
            return std::byteswap(*(uint16_t *)(cart_ram.get() + address - cart_ram_start));
        }
        else {
            address &= rom_mask;
            return std::byteswap(*((uint16_t *)(rom.get() + address)));
        }
    }
    else if (address >= 0x00C00000 && address <= 0xC0001E) {
        return vdp->read_word(address);
    }
    else if (address >= 0xA00000 && address < 0xA10000) {
        //return 0;
        address &= 0x7FFF;
        if (address < 0x4000) {
            return (z80_ram[address & 0x1FFF] << 8) | (z80_ram[(address + 1) & 0x1FFF] & 0xFF);
        }
        else {
            return 0;
        }
    }
    else if (address < 0xE00000) {
        switch (address) {
            case 0xA11100:
            case 0xA11101:
                return (z80_has_bus && z80_reset);
                break;
            case 0xA11200:
            case 0xA11201:
                return z80_reset;
                break;
        }
        int x = 1;
    }
    else if (address >= 0xE00000) {
        return std::byteswap(*(uint16_t *)&ram[address & 0xFFFF]);
    }
    return 0;
}

void c_genesis::write_byte(uint32_t address, uint8_t value)
{
    if (address < 0x400000) {
        if (is_ps4) {
            if (ps4_ram_access && address >= cart_ram_start && address <= cart_ram_end) {
                cart_ram[address - cart_ram_start] = value;
            }
        }
        if (cart_ram_start && address >= cart_ram_start && address <= cart_ram_end)
        {
            cart_ram[address - cart_ram_start] = value;
        }
        else {
            int x = 1;
        }
    }
    else if (address >= 0x00C00000 && address <= 0xC0001E) {
        vdp->write_byte(address, value);
    }
    else if (address >= 0xA00000 && address < 0xA10000) {
        address &= 0x7FFF;
        if (address < 0x4000) {
            z80_ram[address & 0x1FFF] = value;
        }
        else {
            int x = 1;
        }
    }
    else if (address < 0xE00000) {
        switch (address) {
            case 0xA10003:
                th1 = value;
                break;
            case 0xA10005:
                th2 = value;
                break;
            case 0xA10009:
                
                break;
            case 0xA11100:
            case 0xA11101:
                z80_busreq = value;
                z80_has_bus = !value;
                break;
            case 0xA11200:
            case 0xA11201:
                z80_reset = value;
                if (!z80_reset) {
                    z80->reset();
                }
                break;
            case 0xA130F1:
                ps4_ram_access = value;
                break;
            default:
                break;
        }
        int x = 1;
    }
    else {
        ram[address & 0xFFFF] = value;
    }
}

void c_genesis::write_word(uint32_t address, uint16_t value)
{
    assert(!(address & 1));
    if (address < 0x400000) {
        if (is_ps4) {
            if (ps4_ram_access && address >= cart_ram_start && address <= cart_ram_end) {
                assert(0);
                cart_ram[address - cart_ram_start] = value >> 8;
                cart_ram[address - cart_ram_start + 1] = value & 0xFF;
            }
        }
        if (cart_ram_start && address >= cart_ram_start && address <= cart_ram_end)
        {
            assert(0);
            cart_ram[address - cart_ram_start] = value >> 8;
            cart_ram[address - cart_ram_start + 1] = value & 0xFF;
        }
    }
    else if (address >= 0x00C00000 && address <= 0xC0001E) {
        vdp->write_word(address, value);
    }
    else if (address >= 0xA00000 && address < 0xA10000) {
        address &= 0x7FFF;
        if (address < 0x4000) {
            z80_ram[address & 0x1FFF] = value >> 8;
            z80_ram[(address + 1) & 0x1FFF] = value & 0xFF;
        }
        else {
            int x = 1;
        }
    }
    else if (address < 0xE00000) {
        switch (address) {
            case 0x00A11100:
                z80_busreq = value;
                z80_has_bus = !value;
                break;
            case 0xA11200:
                z80_reset = value;
                if (!z80_reset) {
                    z80->reset();
                }
                break;
        }
        int x = 1;
    }
    else {
        ram[address & 0xFFFF] = value >> 8;
        ram[(address + 1) & 0xFFFF] = value & 0xFF;
    }
}


void c_genesis::set_input(int input)
{
    joy1 = input;
    if (joy1) {
        int x = 1;
    }
    joy1 = ~joy1;
}

int *c_genesis::get_video()
{
    return (int*)vdp->frame_buffer;
}

int c_genesis::get_sound_bufs(const short **buf_l, const short **buf_r)
{
    *buf_l = nullptr;
    *buf_r = nullptr;
    return 0;
}
void c_genesis::set_audio_freq(double freq)
{
}

void c_genesis::enable_mixer()
{
}

void c_genesis::disable_mixer()
{
}

} //namespace genesis