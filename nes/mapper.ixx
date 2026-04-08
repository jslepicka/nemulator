module;

export module nes:mapper;
import class_registry;
import :ines;
import nemulator.std;

namespace nes
{

export enum class MAPPER_CLOCK_RATE : int
{
    NONE = 0,
    CPU = 1
};

export enum MIRRORING
{
    MIRRORING_HORIZONTAL = 0,
    MIRRORING_VERTICAL = 1,
    MIRRORING_ONESCREEN_LOW = 2,
    MIRRORING_ONESCREEN_HIGH = 3,
    MIRRORING_FOURSCREEN = 4
};

export class c_mapper
{
  public:
    c_mapper()
    {
        writeProtectSram = false;
        sram_enabled = 1;
        hasSram = false;
        renderingBg = 0;
        sram = 0;
        mapperName = "NROM";
        submapper = 0;
        chrRom = nullptr;
        std::memset(vram, 0, 4096);

        for (int i = 0; i < 4; i++)
            name_table[i] = vram;
        in_sprite_eval = 0;
        mirroring_mode = 0;
        ppu_cycle = nullptr;
    }

    virtual ~c_mapper()
    {
        close_sram();
        if (chrRam)
            delete[] chrRom;
    }

    virtual unsigned char read_byte(unsigned short address)
    {
        if (address >= 0x6000 && address < 0x8000) {
            if (sram_enabled)
                return sram[address - 0x6000];
            else
                return 0;
        }
        unsigned char val = *(prgBank[(address >> 13) & 3] + (address & 0x1FFF));
        return val;
    }

    virtual void write_byte(unsigned short address, unsigned char value)
    {
        if (address >= 0x6000 && address < 0x8000) {
            if (sram_enabled && !writeProtectSram)
                sram[address - 0x6000] = value;
        }
    }

    virtual void ppu_clock()
    {
    }

    virtual void cpu_clock()
    {
    }

    virtual void reset()
    {
        SetPrgBank16k(PRG_C000, header->PrgRomPageCount - 1);
        memset(vram, 0, 4096);
    }

    virtual int load_image()
    {
        prgRom = (prgRomBank *)(image + sizeof(iNesHeader));
        if (header->ChrRomPageCount > 0) {
            chrRam = false;
            chrRom = (chrRomBank *)(image + sizeof(iNesHeader) + (header->PrgRomPageCount * 16384));
        }
        else {
            chrRam = true;
            if (chrRom != nullptr)
                delete[] chrRom;
            chrRom = new chrRomBank[1];
            std::memset(chrRom, 0, 8192);
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

    virtual float mix_audio(float sample)
    {
        return sample;
    }

    void set_submapper(int submapper)
    {
        this->submapper = submapper;
    }

    virtual unsigned char ppu_read(unsigned short address)
    {
        address &= 0x3FFF;
        if (address & 0x2000) {
            return *(name_table[(address >> 10) & 3] + (address & 0x3FF));
        }
        else {
            return read_chr(address);
        }
    }

    virtual void ppu_write(unsigned short address, unsigned char value)
    {
        address &= 0x3FFF;
        if (address & 0x2000) {
            *(name_table[(address >> 10) & 3] + (address & 0x3FF)) = value;
        }
        else {
            write_chr(address, value);
        }
    }

    virtual int get_nwc_time()
    {
        return 0;
    }
    
    virtual int switch_disk()
    {
        return 0;
    }

    int get_mirroring()
    {
        return mirroring_mode;
    }

    int close_sram()
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

protected:
    void set_mirroring(int mode)
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

    void SetPrgBank8k(int bank, int value)
    {
        prgBank[bank] = pPrgRom + ((value % prgRomPageCount8k) * 0x2000);
    }

    void SetPrgBank16k(int bank, int value)
    {
        unsigned char *base = pPrgRom + ((value % header->PrgRomPageCount) * 0x4000);
        prgBank[bank] = base;
        prgBank[bank + 1] = base + 0x2000;
    }

    void SetPrgBank32k(int value)
    {
        unsigned char *base = pPrgRom + ((value % prgRomPageCount32k) * 0x8000);
        for (int i = PRG_8000; i <= PRG_E000; i++)
            prgBank[i] = base + (0x2000 * i);
    }

    virtual void SetChrBank1k(int bank, int value)
    {
        chrBank[bank] = pChrRom + ((chrRam ? (value & 0x7) : (value % chrRomPageCount1k)) * 0x400);
    }

    void SetChrBank2k(int bank, int value)
    {
        unsigned char *base = pChrRom + ((chrRam ? (value & 0x3) : (value % chrRomPageCount2k)) * 0x800);
        for (int i = bank; i < bank + 2; i++)
            chrBank[i] = base + (0x400 * (i - bank));
    }

    void SetChrBank4k(int bank, int value)
    {
        unsigned char *base = pChrRom + ((chrRam ? (value & 0x1) : (value % chrRomPageCount4k)) * 0x1000);
        for (int i = bank; i < bank + 4; i++)
            chrBank[i] = base + (0x400 * (i - bank));
    }

    void SetChrBank8k(int value)
    {
        unsigned char *base = pChrRom + ((chrRam ? 0 : (value % header->ChrRomPageCount)) * 0x2000);
        for (int i = CHR_0000; i <= CHR_1C00; i++)
            chrBank[i] = base + (0x400 * i);
    }

    virtual void write_chr(unsigned short address, unsigned char value)
    {
        if (chrRam)
            *(chrBank[(address >> 10) % 8] + (address & 0x3FF)) = value;
    }

    virtual unsigned char read_chr(unsigned short address)
    {
        return *(chrBank[(address >> 10) % 8] + (address & 0x3FF));
    }

    int open_sram()
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

  public:
    int renderingBg;
    iNesHeader *header = nullptr;
    unsigned char *image = nullptr;
    std::string sramFilename;
    std::string image_path;
    const char *mapperName;
    int in_sprite_eval;
    int file_length;
    struct s_mapper_info
    {
        int number;
        std::string name;
        bool needs_clock = false;
        std::function<std::unique_ptr<c_mapper>()> constructor;
    };
    int crc32;
    std::function<void()> execute_irq = nullptr;
    std::function<void()> clear_irq = nullptr;
    std::function<int()> ppu_get_drawing_bg = nullptr;
    std::function<int()> ppu_get_sprite_size = nullptr;
    uint64_t *ppu_cycle;

  protected:
    enum
    {
        CHR_0000,
        CHR_0400,
        CHR_0800,
        CHR_0C00,
        CHR_1000,
        CHR_1400,
        CHR_1800,
        CHR_1C00
    };

    enum
    {
        PRG_8000,
        PRG_A000,
        PRG_C000,
        PRG_E000
    };

    bool hasSram;
    bool writeProtectSram;
    int sram_enabled;
    int mirroring_mode;

    unsigned char *name_table[8];

    std::unique_ptr<unsigned char[]> sram;
    int prgRomPageCount8k;
    int prgRomPageCount16k;
    int prgRomPageCount32k;
    int chrRomPageCount1k;
    int chrRomPageCount2k;
    int chrRomPageCount4k;

    typedef unsigned char prgRomBank[16384];
    typedef unsigned char chrRomBank[8192];
    prgRomBank *prgRom;
    chrRomBank *chrRom;
    unsigned char *pChrRom;
    unsigned char *pPrgRom;

    unsigned char *prgBank[4];
    unsigned char *chrBank[8];
    int *patternBank[8];
    bool chrRam;
    int submapper;
    alignas(64) unsigned char vram[4096];
};

export class nes_mapper_registry : public c_class_registry<std::map<int, c_mapper::s_mapper_info>>
{
  public:
    static void _register(std::vector<c_mapper::s_mapper_info> mapper_info)
    {
        for (auto &mi : mapper_info) {
            if (mi.name == "") {
                mi.name = "Mapper " + std::to_string(mi.number);
            }
            get_registry()[mi.number] = mi;
        }
    }
};

} //namespace nes