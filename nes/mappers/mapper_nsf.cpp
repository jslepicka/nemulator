#include "mapper_nsf.h"
#include "mapper_nsf_rom.h"
#include <fstream>

namespace nes {

c_mapper_nsf::c_mapper_nsf()
{
    mapperName = "NSF";
    nsf_data = NULL;
    sram = new unsigned char[8 * 1024];

}

c_mapper_nsf::~c_mapper_nsf()
{
    if (nsf_data) {
        delete[] nsf_data;
    }
    if (sram) {
        delete[] sram;
    }
}

void c_mapper_nsf::WriteChrRom(unsigned short address, unsigned char value)
{
    chr_ram[address & 0x1FFF] = value;
}

unsigned char c_mapper_nsf::ReadChrRom(unsigned short address)
{
    if (address > 16) {
        int x = 1;
    }
    if (chr_ram[address & 0x1FFF] != 0) {
        int x = 1;
    }
    return chr_ram[address & 0x1FFF];
}

void c_mapper_nsf::WriteByte(unsigned short address, unsigned char value)
{
    if (address == 0x54F2) {
        int x = 1;
    }
    switch (address >> 12) {
    case 5:
        if (address >= 0x5300 && address < 0x5500) {
            player_rom[address - 0x5300] = value;
        }
        else if (address >= 0x5FF8 && address <= 0x5FFF) {
            int x = 1;
            banks[address - 0x5FF8] = nsf_data + value * 4096;
        }
        break;
    case 6:
    case 7:
        sram[address - 0x6000] = value;
        break;
    default:
        break;
    }
}

unsigned char c_mapper_nsf::ReadByte(unsigned short address)
{
    if (address == 0xa000) {
        int x = 1;
    }
    switch (address >> 12) {
    case 5:
        if (address >= 0x5300 && address < 0x5500) {
            return player_rom[address - 0x5300];
        }
        return 0;
    case 6:
    case 7:
        return sram[address - 0x6000];
    default:
        //NSF 'rom' is at 0x5300
        if (address >= 0xFFFA) {
            switch (address) {
            case 0xFFFA: //nmi low
                return 0xE0;
            case 0xFFFB: //nmi high
                return 0x54;
            case 0xFFFC: //reset low
                return 0x00;
            case 0xFFFD: //reset high
                return 0x53;
            case 0xFFFE: //irq low
                return 0x00;
            case 0xFFFF: //irq high
                return 0x53;
            }
        }
        if (address == 0xFFFC) {
            return 0x00;
        }
        else if (address == 0xFFFD) {
            return 0x53;
        }
        else {
            return banks[(address >> 12) & 0x7][address & 0xFFF];
        }
    }
}

void c_mapper_nsf::load_player()
{
    memcpy(player_rom, PLAYER_ROM, sizeof(PLAYER_ROM));
    return;
    std::ifstream file;
    int m = -1;

    file.open("c:\\nesasm\\player2.nes", std::ios_base::in | std::ios_base::binary);
    file.seekg(0x1300, std::ios_base::beg);
    file.read((char*)player_rom, 512);
    file.close();
}

void c_mapper_nsf::reset()
{
    int x = 1;
    int bankswitching = 0;
    for (int i = 0; i < 8; i++) {
        if (header->bank_init[i] != 0) {
            bankswitching = 1;
            break;
        }
    }
    if (!bankswitching) {
        for (int i = 0; i < 8; i++) {
            banks[i] = nsf_data + (i * 4096);
        }
    }
    else {
        for (int i = 0; i < 8; i++) {
            banks[i] = nsf_data + header->bank_init[i] * 4096;
        }
    }
    load_player();

    WriteByte(0x54F2, header->init_address & 0xFF);
    WriteByte(0x54F3, (header->init_address >> 8) & 0xFF);

    WriteByte(0x54F4, header->play_address & 0xFF);
    WriteByte(0x54F5, (header->play_address >> 8) & 0xFF);

    WriteByte(0x54F6, 1); //1 = play song
    WriteByte(0x54F7, 0); //song number
    WriteByte(0x54F8, header->song_count);

    set_mirroring(MIRRORING_HORIZONTAL);
}

int c_mapper_nsf::LoadImage()
{
    //TODO: support unaligned load address

    int x = 1;
    header = (NSF_HEADER*)image;
    int nsf_length = file_length - sizeof(NSF_HEADER);
    if (header->load_address < 0x8000) {
        return -1;
    }
    int padding = header->load_address - 0x8000;
    int total_length = padding + nsf_length;
    if (total_length < 32 * 1024) {
        nsf_data = new unsigned char[32 * 1024];
    }
    else if (total_length & 0xFFF) {
        //pad
        nsf_data = new unsigned char[(total_length + 4096) & (~0xFFF)];
    }
    else {
        nsf_data = new unsigned char[total_length];
    }
    memcpy(nsf_data + padding, image + sizeof(NSF_HEADER), nsf_length);
    return 0;
}

} //namespace nes
