module;

export module nes:mapper;
import class_registry;
import :ines;
import nemulator.std;

namespace nes
{

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
    c_mapper();
    virtual ~c_mapper();
    virtual unsigned char read_byte(unsigned short address);
    virtual void write_byte(unsigned short address, unsigned char value);
    virtual void clock(int cycles) {};
    virtual void reset();
    virtual int load_image();
    virtual float mix_audio(float sample);
    int renderingBg;
    void set_submapper(int submapper);
    iNesHeader *header = nullptr;
    unsigned char *image = nullptr;
    std::string sramFilename;
    std::string image_path;
    const char *mapperName;
    int close_sram();
    int crc32;
    virtual unsigned char ppu_read(unsigned short address);
    virtual void ppu_write(unsigned short address, unsigned char value);
    virtual int get_nwc_time()
    {
        return 0;
    }
    virtual int switch_disk() { return 0; };
    int in_sprite_eval;
    //c_nes *nes;
    int get_mirroring();
    int file_length;

    struct s_mapper_info
    {
        int number;
        std::string name;
        std::function<std::unique_ptr<c_mapper>()> constructor;
    };

    std::function<void()> execute_irq = nullptr;
    std::function<void()> clear_irq = nullptr;
    std::function<int()> ppu_get_drawing_bg = nullptr;
    std::function<int()> ppu_get_sprite_size = nullptr;

  protected:
    static const int CHR_0000 = 0;
    static const int CHR_0400 = 1;
    static const int CHR_0800 = 2;
    static const int CHR_0C00 = 3;
    static const int CHR_1000 = 4;
    static const int CHR_1400 = 5;
    static const int CHR_1800 = 6;
    static const int CHR_1C00 = 7;

    static const int PRG_8000 = 0;
    static const int PRG_A000 = 1;
    static const int PRG_C000 = 2;
    static const int PRG_E000 = 3;

    bool hasSram;
    bool writeProtectSram;
    int sram_enabled;
    void set_mirroring(int mode);
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

    void SetPrgBank8k(int bank, int value);
    void SetPrgBank16k(int bank, int value);
    void SetPrgBank32k(int value);
    virtual void SetChrBank1k(int bank, int value);
    void SetChrBank2k(int bank, int value);
    void SetChrBank4k(int bank, int value);
    void SetChrBank8k(int value);
    virtual void write_chr(unsigned short address, unsigned char value);
    virtual unsigned char read_chr(unsigned short address);
    bool chrRam;

    int open_sram();

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