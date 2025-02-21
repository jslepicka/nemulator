#include "memory.h"
#include "mapper.h"
#include <fstream>

#if defined(DEBUG) | defined(_DEBUG)
    #define DEBUG_NEW new(_CLIENT_BLOCK, __FILE__, __LINE__)
    #define new DEBUG_NEW
#endif

namespace nes
{

c_mapper::c_mapper()
{
    writeProtectSram = false;
    sram_enabled = 1;
    hasSram = false;
    renderingBg = 0;
    sram = 0;
    mapperName = "NROM";
    submapper = 0;
    chrRom = NULL;
    memset(vram, 0, 4096);

    for (int i = 0; i < 4; i++)
        name_table[i] = vram;
    in_sprite_eval = 0;
    mirroring_mode = 0;
}

void c_mapper::set_submapper(int submapper)
{
    this->submapper = submapper;
}

c_mapper::~c_mapper()
{
    close_sram();
    if (chrRam)
        delete[] chrRom;
}

unsigned char c_mapper::read_byte(unsigned short address)
{
    if (address >= 0x6000 && address < 0x8000) {
        if (sram_enabled)
            //return 0;
            return sram[address - 0x6000];
        else
            return 0;
    }
    unsigned char val = *(prgBank[(address >> 13) & 3] + (address & 0x1FFF));
    return val;
}

void c_mapper::write_byte(unsigned short address, unsigned char value)
{
    if (address >= 0x6000 && address < 0x8000) {
        if (sram_enabled && !writeProtectSram)
            sram[address - 0x6000] = value;
    }
}

unsigned char c_mapper::read_chr(unsigned short address)
{
    return *(chrBank[(address >> 10) % 8] + (address & 0x3FF));
}

void c_mapper::write_chr(unsigned short address, unsigned char value)
{
    if (chrRam)
        *(chrBank[(address >> 10) % 8] + (address & 0x3FF)) = value;
}

void c_mapper::reset()
{
    SetPrgBank16k(PRG_C000, header->PrgRomPageCount - 1);
    memset(vram, 0, 4096);
}

void c_mapper::SetPrgBank8k(int bank, int value)
{
    prgBank[bank] = pPrgRom + ((value % prgRomPageCount8k) * 0x2000);
}

void c_mapper::SetPrgBank16k(int bank, int value)
{
    unsigned char *base = pPrgRom + ((value % header->PrgRomPageCount) * 0x4000);
    prgBank[bank] = base;
    prgBank[bank + 1] = base + 0x2000;
}

void c_mapper::SetPrgBank32k(int value)
{
    unsigned char *base = pPrgRom + ((value % prgRomPageCount32k) * 0x8000);
    for (int i = PRG_8000; i <= PRG_E000; i++)
        prgBank[i] = base + (0x2000 * i);
}

void c_mapper::SetChrBank1k(int bank, int value)
{
    chrBank[bank] = pChrRom + ((chrRam ? (value & 0x7) : (value % chrRomPageCount1k)) * 0x400);
}

void c_mapper::SetChrBank2k(int bank, int value)
{
    unsigned char *base = pChrRom + ((chrRam ? (value & 0x3) : (value % chrRomPageCount2k)) * 0x800);
    for (int i = bank; i < bank + 2; i++)
        chrBank[i] = base + (0x400 * (i - bank));
}

void c_mapper::SetChrBank4k(int bank, int value)
{
    unsigned char *base = pChrRom + ((chrRam ? (value & 0x1) : (value % chrRomPageCount4k)) * 0x1000);
    for (int i = bank; i < bank + 4; i++)
        chrBank[i] = base + (0x400 * (i - bank));
}

void c_mapper::SetChrBank8k(int value)
{
    unsigned char *base = pChrRom + ((chrRam ? 0 : (value % header->ChrRomPageCount)) * 0x2000);
    for (int i = CHR_0000; i <= CHR_1C00; i++)
        chrBank[i] = base + (0x400 * i);
}

int c_mapper::load_image()
{
    prgRom = (prgRomBank *)(image + sizeof(iNesHeader));
    if (header->ChrRomPageCount > 0) {
        chrRam = false;
        chrRom = (chrRomBank *)(image + sizeof(iNesHeader) + (header->PrgRomPageCount * 16384));
    }
    else {
        chrRam = true;
        if (chrRom != NULL)
            delete[] chrRom;
        chrRom = new chrRomBank[1];
        memset(chrRom, 0, 8192);
    }
    pChrRom = (unsigned char *)chrRom;
    pPrgRom = (unsigned char *)prgRom;

    prgRomPageCount8k = header->PrgRomPageCount * 2;
    prgRomPageCount16k = header->PrgRomPageCount;
    prgRomPageCount32k = header->PrgRomPageCount / 2;
    if (prgRomPageCount32k < 1)
        prgRomPageCount32k = 1;
    chrRomPageCount1k = header->ChrRomPageCount * 8;
    chrRomPageCount2k = header->ChrRomPageCount * 4;
    chrRomPageCount4k = header->ChrRomPageCount * 2;

    if (header->Rcb1.Fourscreen)
        set_mirroring(MIRRORING_FOURSCREEN);
    else
        set_mirroring(header->Rcb1.Mirroring);

    for (int x = PRG_8000; x <= PRG_E000; x++)
        prgBank[x] = pPrgRom + 0x2000 * x;

    for (int x = CHR_0000; x <= CHR_1C00; x++)
        chrBank[x] = pChrRom + 0x0400 * x;

    open_sram();

    return 0;
}

int c_mapper::open_sram()
{
    if (!sram) {
        sram = std::make_unique<unsigned char[]>(8192);
    }

    if (header->Rcb1.Sram) {
        hasSram = true;
        std::ifstream file;
        file.open(sramFilename, std::ios_base::in | std::ios_base::binary);
        if (file.is_open()) {
            file.read((char *)sram.get(), 8192);
            file.close();
        }
    }
    return 0;
}

int c_mapper::close_sram()
{
    if (hasSram) {
        std::ofstream file;
        file.open(sramFilename, std::ios_base::trunc | std::ios_base::binary);
        if (file.is_open()) {
            file.write((char *)sram.get(), 8192);
            file.close();
        }
    }
    if (sram)
        sram.reset();
    return 0;
}

unsigned char c_mapper::ppu_read(unsigned short address)
{
    address &= 0x3FFF;
    if (address & 0x2000) {
        return *(name_table[(address >> 10) & 3] + (address & 0x3FF));
    }
    else {
        return read_chr(address);
    }
}

void c_mapper::ppu_write(unsigned short address, unsigned char value)
{
    address &= 0x3FFF;
    if (address & 0x2000) {
        *(name_table[(address >> 10) & 3] + (address & 0x3FF)) = value;
    }
    else {
        write_chr(address, value);
    }
}

void c_mapper::set_mirroring(int mode)
{
    switch (mode) {
        case MIRRORING_HORIZONTAL:
            name_table[0] = name_table[1] = &vram[0];
            name_table[2] = name_table[3] = &vram[0x400];
            break;
        case MIRRORING_VERTICAL:
            name_table[0] = name_table[2] = &vram[0];
            name_table[1] = name_table[3] = &vram[0x400];
            break;
        case MIRRORING_ONESCREEN_LOW:
            name_table[0] = name_table[1] = name_table[2] = name_table[3] = &vram[0];
            break;
        case MIRRORING_ONESCREEN_HIGH:
            name_table[0] = name_table[1] = name_table[2] = name_table[3] = &vram[0x400];
            break;
        case MIRRORING_FOURSCREEN:
            name_table[0] = &vram[0];
            name_table[1] = &vram[0x400];
            name_table[2] = &vram[0x800];
            name_table[3] = &vram[0xC00];
            break;
    }
    mirroring_mode = mode;
}

int c_mapper::get_mirroring()
{
    return mirroring_mode;
}

float c_mapper::mix_audio(float sample)
{
    return sample;
}

} //namespace nes