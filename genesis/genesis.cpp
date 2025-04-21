module;
#include <cassert>
module genesis;
import m68k;
import crc32;

namespace genesis
{

c_genesis::c_genesis()
{
    ram = std::make_unique<unsigned char[]>(64 * 1024);
    m68k = std::make_unique<c_m68k>(
            [this](uint32_t address) { return this->read_word(address); },
            [this](uint32_t address, uint16_t value) { this->write_word(address, value); },
            [this](uint32_t address) { return this->read_byte(address); },
            [this](uint32_t address, uint8_t value) { this->write_byte(address, value); },
            &ipl,
            &stalled
    );
    vdp = std::make_unique<c_vdp>(&ipl,
         [this](uint32_t address) { return this->read_word(address); },
        &stalled
    );
}

c_genesis::~c_genesis()
{
}

int c_genesis::load()
{
    std::ifstream file;
    file.open(path_file, std::ios_base::in | std::ios_base::binary);
    if (file.fail())
        return 0;
    file.seekg(0, std::ios_base::end);
    file_length = (int)file.tellg();
    file.seekg(0, std::ios_base::beg);
    //if (file_length == 0 || (file_length & (file_length - 1)) || file_length < 0x2000) {
    //    return 0;
    //}
    //int alloc_size = file_length < 0x4000 ? 0x4000 : file_length;
    
    rom = std::make_unique_for_overwrite<uint8_t[]>(file_length);
    
    file.read((char *)rom.get(), file_length);
    file.close();
    crc32 = get_crc32(rom.get(), file_length);

    //for (auto &c : crc_table) {
    //    if (c.crc == crc32 && c.has_sram) {
    //        has_sram = 1;
    //        load_sram();
    //    }
    //}

    //if (file_length == 0x2000) {
    //    std::memcpy(rom.get() + 0x2000, rom.get(), 0x2000);
    //}
    //loaded = 1;
    //reset();
    //return file_length;
    reset();
    loaded = 1;
    return 1;
}

//int c_genesis::load_sram()
//{
//    //std::ifstream file;
//    //file.open(sram_path_file, std::ios_base::in | std::ios_base::binary);
//    //if (file.fail())
//    //    return 1;
//
//    //file.seekg(0, std::ios_base::end);
//    //int l = (int)file.tellg();
//
//    //if (l != 8192)
//    //    return 1;
//
//    //file.seekg(0, std::ios_base::beg);
//
//    //file.read((char *)cart_ram, 8192);
//    //file.close();
//    return 1;
//}
//
//int c_genesis::save_sram()
//{
//    //std::ofstream file;
//    //file.open(sram_path_file, std::ios_base::out | std::ios_base::binary);
//    //if (file.fail())
//    //    return 1;
//    //file.write((char *)cart_ram, 8192);
//    //file.close();
//    return 0;
//}

int c_genesis::reset()
{
    //z80->reset();
    //vdp->reset();
    //psg->reset();
    //std::memset(ram.get(), 0x00, 8192);
    //irq = 0;
    //nmi = 0;
    //page[0] = rom.get();
    //page[1] = file_length > 0x4000 ? rom.get() + 0x4000 : rom.get();
    //page[2] = file_length > 0x8000 ? rom.get() + 0x8000 : rom.get();
    //nationalism = 0;
    //ram_select = 0;
    //joy = 0xFFFF;
    //psg_cycles = 0;
    //last_psg_run = 0;
    ipl = 0;
    m68k->reset();
    vdp->reset();
    last_bus_request = 0;
    stalled = 0;
    th1 = 0;
    th2 = 0;
    joy1 = 0;
    joy2 = 0;
    return 0;
}

int c_genesis::emulate_frame()
{
    for (int i = 0; i < 262; i++) {
        m68k->execute(488);

        vdp->draw_scanline();
    }
    return 0;
}

uint8_t c_genesis::read_byte(uint32_t address)
{
    if (address < 0x400000) {
        //probably not correct.  how is on-cart ram in this address space
        //handled?  is that even a thing?
        //address %= file_length;
        //testing
        //address &= 0xFFFF;
        //assert(address < file_length);
        return rom[address];
    }
    else if (address >= 0x00C00000 && address <= 0xC0001E) {
        return vdp->read_byte(address);
    }
    else if (address < 0xFF0000) {
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
                    uint16_t ret = (th2 & 0x40) | (0xFF & 0x3F);

                    return ret;
                }
                else {
                    uint16_t ret = (0xFF & 0x3) | ((0xFF >> 10) & 0xC);
                    if (ret != 0) {
                        int x = 1;
                    }
                    return ret;
                }
                return 0x00;
            case 0xA11100:
                //68k bus access? incomplete
                return last_bus_request;
            default:
                return 0x00;
        }
    }
    else {
        return ram[address - 0xFF0000];
    }
    return 0;
}

uint16_t c_genesis::read_word(uint32_t address)
{
    //assert(!(address & 1));
    if (address < 0x400000) {
        //testing
        //address &= 0xFFFF;
        address %= file_length;
        assert(address < file_length);
        uint16_t a = (rom[address] << 8) | rom[address + 1];
        //uint16_t b = std::byteswap(*((uint16_t *)rom.get() + (address >> 1)));
        return a;
    }
    else if (address >= 0x00C00000 && address <= 0xC0001E) {
        return vdp->read_word(address);
    }
    else if (address < 0xFF0000) {
        int x = 1;
    }
    else {
        return (ram[address - 0xFF0000] << 8) | ram[address - 0xFF0000 + 1];
    }
    return 0;
}

void c_genesis::write_byte(uint32_t address, uint8_t value)
{
    if (address < 0x400000) {
        //assert(0);
        //testing
        //address &= 0xFFFF;
        rom[address] = value;
    }
    else if (address >= 0x00C00000 && address <= 0xC0001E) {
        vdp->write_byte(address, value);
    }
    else if (address < 0xFF0000) {
        if (address == 0xa11100) {
            last_bus_request = value;
        }
        switch (address) {
            case 0xA10003:
                th1 = value;
                break;
            case 0xA10005:
                th2 = value;
                break;
            case 0xA10009:
                
                break;
            default:
                break;
        }
        int x = 1;
    }
    else {
        ram[address - 0xFF0000] = value;
    }
}

void c_genesis::write_word(uint32_t address, uint16_t value)
{
    assert(!(address & 1));
    if (address < 0x400000) {
        //testing
        //address &= 0xFFFF;
        rom[address] = value >> 8;
        rom[address + 1] = value & 0xFF;
        //assert(0);
    }
    else if (address >= 0x00C00000 && address <= 0xC0001E) {
        vdp->write_word(address, value);
    }
    else if (address < 0xFF0000) {
        switch (address) {
            case 0x00A11100:
                last_bus_request = value;
                break;
        }
        int x = 1;
    }
    else {
        ram[address - 0xFF0000] = value >> 8;
        ram[address - 0xFF0000 + 1] = value & 0xFF;
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