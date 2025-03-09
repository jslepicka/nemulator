module;
#include <memory>
export module nes:mapper.mapper5;
import :apu;
import :mapper;
import class_registry;
import nemulator.std;

namespace nes {

class c_mapper5 : public c_mapper, register_class<nes_mapper_registry, c_mapper5>
{
public:
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {{
            .number = 5,
            .name = "MMC5",
            .constructor = []() { return std::make_unique<c_mapper5>(); },
        }};
    }

    c_mapper5()
    {
        const int prg_ram_size = 128 * 1024;
        const int exram_size = 1 * 1024;
        prg_ram = new unsigned char[prg_ram_size];
        exram = new unsigned char[exram_size];
        memset(exram, 0, exram_size);
        memset(prg_ram, 0, prg_ram_size);
    }

    ~c_mapper5()
    {
        delete[] exram;
        delete[] prg_ram;
    }

    void write_byte(unsigned short address, unsigned char value) override
    {
        if (address >= 0x8000) {
            int x = 1;
        }
        switch (address >> 12) {
            case 5:
                if (address >= 0x5C00 && address < 0x6000) {
                    if (exram_mode == 2 || (exram_mode < 2 && inFrame)) {
                        exram[address - 0x5C00] = value;
                    }
                    else if (exram_mode < 2) {
                        exram[address - 0x5C00] = 0;
                    }
                    return;
                }
                switch (address) {
                    case 0x5001:
                    case 0x5005:
                        break;
                    case 0x5000:
                    //case 0x5001:
                    case 0x5002:
                    case 0x5003:
                    case 0x5004:
                    //case 0x5005:
                    case 0x5006:
                    case 0x5007:
                        squares[(address & 0x4) >> 2].write(address, value);
                        break;
                    case 0x5010:
                        pcm_irq_mode = value & 0x1;
                        pcm_irq_enabled = value & 0x80;
                        break;
                    case 0x5011:
                        if (pcm_irq_mode == PCM_IRQ_MODE_WRITE && value != 0)
                            pcm_data = value;
                        break;
                    case 0x5015:
                        if (value & 0x1)
                            squares[0].enable();
                        else
                            squares[0].disable();

                        if (value * 0x2)
                            squares[1].enable();
                        else
                            squares[1].disable();
                        //sound
                        break;
                    case 0x5100:
                        prgMode = value & 0x03;
                        Sync();
                        break;
                    case 0x5101:
                        chrMode = value & 0x03;
                        Sync();
                        break;
                    case 0x5102:
                    case 0x5103:
                        prg_ram_protect[address - 0x5102] = value & 0x3;
                        break;
                    case 0x5104:
                        exram_mode = value & 0x3;
                        if (exram_mode == 1) {
                            int a = 1;
                        }
                        break;
                    case 0x5105:
                        for (int i = 0; i < 4; i++) {
                            switch ((value >> (i * 2)) & 0x3) {
                                case 0:
                                    name_table[i] = vram;
                                    break;
                                case 1:
                                    name_table[i] = vram + 0x400;
                                    break;
                                case 2:
                                    name_table[i] = exram;
                                    break;
                                case 3:
                                    name_table[i] = 0;
                                    break;
                            }
                        }
                        break;
                    case 0x5106:
                        fillTile = value;
                        break;
                    case 0x5107:
                        fillColor = value & 0x03;
                        fillColor |= fillColor << 2;
                        fillColor |= fillColor << 4;
                        break;
                    case 0x5113:
                        prg_6000 = prg_ram + (value & 0xF) * 0x2000;
                        break;
                    case 0x5114:
                    case 0x5115:
                    case 0x5116:
                    case 0x5117:
                        prg_reg[address - 0x5114] = value;
                        Sync();
                        break;
                    case 0x5120:
                    case 0x5121:
                    case 0x5122:
                    case 0x5123:
                    case 0x5124:
                    case 0x5125:
                    case 0x5126:
                    case 0x5127:
                        banks[address - 0x5120] = value | (chr_high << 8);
                        lastBank = 0;
                        Sync();
                        break;
                    case 0x5128:
                    case 0x5129:
                    case 0x512A:
                    case 0x512B:
                        banks[(address - 0x5128) + 8] = value | (chr_high << 8);
                        lastBank = 1;
                        Sync();
                        break;
                    case 0x5130:
                        chr_high = value & 0x3;
                        break;
                    case 0x5200:
                        split_control = value;
                        break;
                    case 0x5201:
                        split_scroll = value;
                        break;
                    case 0x5202:
                        split_page = value;
                        break;
                    case 0x5203:
                        irqTarget = value;
                        //cpu->clear_irq();
                        break;
                    case 0x5204:
                        irqEnable = value & 0x80;
                        if (!irqEnable && irq_asserted) {
                            clear_irq();
                            irq_asserted = 0;
                        }
                        else if (irqEnable && irqPending && !irq_asserted) {
                            execute_irq();
                            irq_asserted = 1;
                        }
                        break;
                    case 0x5205:
                        multiplicand = value;
                        break;
                    case 0x5206:
                        multiplier = value;
                        break;
                    default:
                        int a = 1;
                        break;
                }
                break;
            case 6:
            case 7:
                if (prg_ram_protect[0] == 0x2 && prg_ram_protect[1] == 0x1)
                    *(prg_6000 + (address & 0x1FFF)) = value;
                break;
            default:
                *(prgBank[(address >> 13) & 3] + (address & 0x1FFF)) = value;
                break;
        }
    }

    unsigned char read_byte(unsigned short address) override
    {
        if (address == 0xFFFA || address == 0xFFFB) {
            inFrame = 0;
            last_address = 0;
            irqPending = 0;
            if (irq_asserted) {
                clear_irq();
                irq_asserted = 0;
            }
        }
        switch (address >> 12) {
            case 5:
                if (address >= 0x5C00 && address < 0x6000) {
                    if (exram_mode > 1)
                        return exram[address - 0x5C00];
                    else
                        return 0;
                }
                switch (address) {
                    case 0x5010: {
                        int retval = 0;
                        if (pcm_irq_asserted) {
                            retval = 0x80;
                            pcm_irq_asserted = 0;
                            if (!irq_asserted)
                                clear_irq();
                        }
                        return retval;
                    } break;
                    case 0x5015:
                        return (squares[0].get_status()) | ((squares[1].get_status()) << 1);
                        break;
                    case 0x5204: {
                        int val = (inFrame << 6) | (irqPending << 7);
                        irqPending = 0;
                        if (irq_asserted) {
                            irq_asserted = 0;
                            if (!pcm_irq_asserted)
                                clear_irq();
                            else {
                                int x = 1;
                            }
                        }
                        return val;
                    }
                    case 0x5205:
                        return (multiplicand * multiplier) & 0xFF;
                    case 0x5206:
                        return ((multiplicand * multiplier) >> 8) & 0xFF;
                    default:
                        return 0;
                }
                break;
            case 6:
            case 7:
                return *(prg_6000 + (address & 0x1FFF));
            default: {
                unsigned char retval = *(prgBank[(address >> 13) & 3] + (address & 0x1FFF));
                if (pcm_irq_mode == PCM_IRQ_MODE_READ) {
                    if (address >= 0x8000 || address < 0xC000) {
                        if (retval == 0) {
                            if (!irq_asserted && !pcm_irq_asserted) {
                                pcm_irq_asserted = 1;
                                execute_irq();
                            }
                        }
                        else {
                            pcm_data = retval;
                        }
                    }
                }
                return retval;
            }

                return c_mapper::read_byte(address);
        }
        //return c_mapper::read_byte(address);
    }

    void reset() override
    {
        for (int i = 0; i < 2; i++)
            squares[i].reset();
        ticks = 0;
        frame_seq_counter = CLOCKS_PER_FRAME_SEQ;
        prg_reg[0] = 0;
        prg_reg[1] = 0;
        prg_reg[2] = 0;
        prg_reg[3] = 0xFF;
        prg_6000 = prg_ram;
        prg_ram_protect[0] = 0;
        prg_ram_protect[1] = 0;
        exram_mode = 0;
        chr_high = 0;
        multiplicand = 0;
        multiplier = 0;
        last_tile = 0;

        fillTile = 0;
        fillColor = 0;
        prgMode = 3;
        chrMode = 0;
        irqPending = 0;
        irqCounter = 0;
        inFrame = 0;
        irqEnable = 0;
        irqTarget = 0;
        lastBank = 0;
        SetChrBank8k(bankA, 0);
        SetChrBank8k(bankB, 0);
        Sync();
        irq_asserted = 0;
        using_fill_table = 0;
        drawing_enabled = 0;
        split_control = 0;
        split_scroll = 0;
        split_page = 0;
        cycle = 0;
        scanline = 0;
        scroll_line = 0;
        cycles_since_rendering = 0;
        read_buffer = 0;
        last_ppu_read = 0;
        fetch_count = 0;
        irq_q = 0;
        vscroll = 0;
        htile = 0;
        in_split_region = 0;

        pcm_irq_enabled = 0;
        pcm_irq_asserted = 0;
        pcm_irq_mode = PCM_IRQ_MODE_WRITE;
        pcm_data = 0;

        last_address = 0;
        last_address_match_count = 0;
        idle_count = 0;
        ppu_is_reading = 0;

        tile_fetch_count = 0;
    }

    void clock(int cycles) override
    {
        ticks += cycles;
        while (ticks > 2) {
            ticks -= 3;
            clock_frame();

            if (ppu_is_reading) {
                idle_count = 0;
            }
            else {
                if (++idle_count == 3) {
                    inFrame = 0;
                    last_address = 0;
                }
            }
            ppu_is_reading = 0;
        }
    }

    float mix_audio(float sample) override
    {
        int square_vol = squares[0].get_output_mmc5() + squares[1].get_output_mmc5();
        float square_out = c_apu::square_lut[square_vol];
        float pcm_out = (float)pcm_data / 255.0f * .42f;

        return sample + -square_out + -pcm_out;
        //return sample * .5f + square_out * .5f;
    }


private:
    int frame_seq_counter;
    static const int CLOCKS_PER_FRAME_SEQ = 89489;
    
    int ticks;
    c_apu::c_square squares[2];
    enum {
        PCM_IRQ_MODE_WRITE,
        PCM_IRQ_MODE_READ
    };
    int pcm_irq_enabled;
    int pcm_irq_asserted;
    int pcm_irq_mode;
    int pcm_data;
    int fillTile;
    int fillColor;
    int prgMode;
    int chrMode;
    int irqPending;
    int irqCounter;
    int inFrame;
    int irqEnable;
    int irqTarget;
    int banks[12];
    int lastBank;
    unsigned char *bankA[8];
    unsigned char *bankB[8];
    int last_tile;
    int prg_reg[4];
    unsigned char *prg_ram;
    unsigned char *exram;
    unsigned char *prg_6000;
    int chr_high;
    int prg_ram_protect[2];
    int exram_mode;
    int multiplicand;
    int multiplier;
    int irq_asserted;
    int using_fill_table;
    int drawing_enabled;
    int split_control;
    int split_scroll;
    int split_page;
    int cycle;
    int scroll_line;
    int scanline;
    int cycles_since_rendering;
    int read_buffer;
    int last_ppu_read;
    int fetch_count;
    int irq_q;
    int vscroll;
    int htile;
    int split_address;
    int in_split_region;
    int last_address;
    int last_address_match_count;
    int idle_count;
    int ppu_is_reading;
    int tile_fetch_count;

    void Sync()
    {
        switch (chrMode) {
            case 0:
                SetChrBank8k(bankA, banks[7]);
                SetChrBank8k(bankB, banks[3 + 8]);
                break;
            case 1:
                SetChrBank4k(bankA, CHR_0000, banks[3]);
                SetChrBank4k(bankA, CHR_1000, banks[7]);
                SetChrBank4k(bankB, CHR_0000, banks[3 + 8]);
                SetChrBank4k(bankB, CHR_1000, banks[3 + 8]);
                break;
            case 2:
                SetChrBank2k(bankA, CHR_0000, banks[1]);
                SetChrBank2k(bankA, CHR_0800, banks[3]);
                SetChrBank2k(bankA, CHR_1000, banks[5]);
                SetChrBank2k(bankA, CHR_1800, banks[7]);

                SetChrBank2k(bankB, CHR_0000, banks[1 + 8]);
                SetChrBank2k(bankB, CHR_1000, banks[1 + 8]);
                SetChrBank2k(bankB, CHR_0800, banks[3 + 8]);
                SetChrBank2k(bankB, CHR_1800, banks[3 + 8]);
                break;
            case 3:
                SetChrBank1k(bankA, CHR_0000, banks[0]);
                SetChrBank1k(bankA, CHR_0400, banks[1]);
                SetChrBank1k(bankA, CHR_0800, banks[2]);
                SetChrBank1k(bankA, CHR_0C00, banks[3]);
                SetChrBank1k(bankA, CHR_1000, banks[4]);
                SetChrBank1k(bankA, CHR_1400, banks[5]);
                SetChrBank1k(bankA, CHR_1800, banks[6]);
                SetChrBank1k(bankA, CHR_1C00, banks[7]);

                SetChrBank1k(bankB, CHR_0000, banks[8]);
                SetChrBank1k(bankB, CHR_0400, banks[9]);
                SetChrBank1k(bankB, CHR_0800, banks[10]);
                SetChrBank1k(bankB, CHR_0C00, banks[11]);
                SetChrBank1k(bankB, CHR_1000, banks[8]);
                SetChrBank1k(bankB, CHR_1400, banks[9]);
                SetChrBank1k(bankB, CHR_1800, banks[10]);
                SetChrBank1k(bankB, CHR_1C00, banks[11]);

                break;
        }

        switch (prgMode & 0x03) {
            case 0:
                SetPrgBank32k(prg_reg[3] | 0x80);
                break;
            case 1:
                SetPrgBank16k(PRG_8000, prg_reg[1]);
                SetPrgBank16k(PRG_C000, prg_reg[3] | 0x80);
                break;
            case 2:
                SetPrgBank16k(PRG_8000, prg_reg[1]);
                SetPrgBank8k(PRG_C000, prg_reg[2]);
                SetPrgBank8k(PRG_E000, prg_reg[3] | 0x80);
                break;
            case 3:
                SetPrgBank8k(PRG_8000, prg_reg[0]);
                SetPrgBank8k(PRG_A000, prg_reg[1]);
                SetPrgBank8k(PRG_C000, prg_reg[2]);
                SetPrgBank8k(PRG_E000, prg_reg[3] | 0x80);
                break;
        }
    }

    void SetPrgBank8k(int bank, int value)
    {
        if (value & 0x80) {
            int x = 1;
        }
        prgBank[bank] = (value & 0x80 ? pPrgRom : prg_ram) + (((value & 0x7F) % prgRomPageCount8k) * 0x2000);
    }

    void SetPrgBank16k(int bank, int value)
    {
        if (value & 0x80) {
            int x = 1;
        }
        // TODO: prg_ram access shouldn't be masked with prg rom page count
        // probably doesn't matter since no game has more ram than rom, but still...
        // We should really mask with actual ram size, but that requires entry in
        // cartridge database or info from NES 2.0 header.
        unsigned char *base =
            (value & 0x80 ? pPrgRom : prg_ram) + ((((value & 0x7F) >> 1) % header->PrgRomPageCount) * 0x4000);
        prgBank[bank] = base;
        prgBank[bank + 1] = base + 0x2000;
    }

    void SetPrgBank32k(int value)
    {
        if (value & 0x80) {
            int x = 1;
        }
        unsigned char *base =
            (value & 0x80 ? pPrgRom : prg_ram) + ((((value & 0x7F) >> 2) % prgRomPageCount32k) * 0x8000);
        for (int i = PRG_8000; i <= PRG_E000; i++)
            prgBank[i] = base + (0x2000 * i);
    }

    void SetChrBank1k(unsigned char *b[8], int bank, int value)
    {
        b[bank] = pChrRom + ((chrRam ? value : (value % chrRomPageCount1k)) * 0x400);
    }

    void SetChrBank2k(unsigned char *b[8], int bank, int value)
    {
        unsigned char *base = pChrRom + ((chrRam ? value : (value % chrRomPageCount2k)) * 0x800);
        for (int i = bank; i < bank + 2; i++)
            b[i] = base + (0x400 * (i - bank));
    }

    void SetChrBank4k(unsigned char *b[8], int bank, int value)
    {
        unsigned char *base = pChrRom + ((chrRam ? value : (value % chrRomPageCount4k)) * 0x1000);
        for (int i = bank; i < bank + 4; i++)
            b[i] = base + (0x400 * (i - bank));
    }

    void SetChrBank8k(unsigned char *b[8], int value)
    {
        unsigned char *base = pChrRom + ((chrRam ? value : (value % header->ChrRomPageCount)) * 0x2000);
        for (int i = CHR_0000; i <= CHR_1C00; i++)
            b[i] = base + (0x400 * i);
    }

    unsigned char read_chr(unsigned short address) override
    {
        int page = (exram[last_tile] & 0x3F) | (chr_high << 6);
        unsigned char *base = pChrRom + ((page % chrRomPageCount4k) * 0x1000);
        unsigned char *split_base = pChrRom + ((split_page % chrRomPageCount4k) * 0x1000);
        unsigned char *bank8 = lastBank ? bankB[address >> 10] : bankA[address >> 10];

        int scroll_addr = (vscroll & 0x7) | (address & ~0x7);

        if (!inFrame) {
            return *(bank8 + (address & 0x3FF));
        }

        if (ppu_get_sprite_size()) {
            if (ppu_get_drawing_bg()) {
                if (exram_mode == 1) {
                    return *(base + (address & 0xFFF));
                }
                else if (in_split_region) {
                    return *(split_base + (scroll_addr & 0xFFF));
                }
                else
                    return *(bankB[address >> 10] + (address & 0x3FF));
            }
            else
                return *(bankA[address >> 10] + (address & 0x3FF));
        }
        else {
            if (ppu_get_drawing_bg()) {
                if (exram_mode == 1) {
                    return *(base + (address & 0xFFF));
                }
                else if (in_split_region) {
                    return *(split_base + (scroll_addr & 0xFFF));
                }
                else
                    return *(bank8 + (address & 0x3FF));
            }
            else
                return *(bank8 + (address & 0x3FF));
        }
    }

    unsigned char ppu_read(unsigned short address) override
    {
        int nt_fetch = (address >= 0x2000 && address <= 0x2FFF && (address & 0x23FF) < 0x23C0) ? 1 : 0;

        if (irqPending) {
            if (irqEnable && !irq_asserted) {
                execute_irq();
                irq_asserted = 1;
            }
        }

        if (nt_fetch) {
            in_split_region = 0;
            htile++;
        }

        if (!in_sprite_eval) {
            if (address >= 0x2000 && address <= 0x2FFF && address == last_address) {
                if (++last_address_match_count == 2) {
                    if (!inFrame) {
                        inFrame = 1;
                        vscroll = split_scroll;
                        scanline = 0;
                        irqPending = 0;
                        if (irq_asserted) {
                            clear_irq();
                            irq_asserted = 0;
                        }
                    }
                    else {
                        vscroll++;
                        if (vscroll > 239) {
                            vscroll -= 240;
                        }
                        if (++scanline == irqTarget) {
                            irqPending = 1;
                        }
                    }
                    htile = 0;
                }
            }
            else {
                last_address_match_count = 0;
            }

            last_address = address;
            ppu_is_reading = 1;
        }

        address &= 0x3FFF;
        //this needs to be changed (to 42) once sprite evaluation is done properly.
        int tt = (htile + 2) % 34;

        if ((address & 0x23FF) >= 0x23C0 && exram_mode == 1 && ppu_get_drawing_bg()) {
            int attr = (exram[last_tile] >> 6) & 0x03;
            attr |= attr << 2;
            attr |= attr << 4;
            return attr;
        }
        else if ((address & 0x23FF) >= 0x23C0 && using_fill_table) {
            using_fill_table = 0;
            return fillColor;
        }
        else if ((address & 0x23FF) >= 0x23C0 && in_split_region) {
            int vtile = (split_address & 0x3E0) >> 5;
            int attribute_address = 0x3C0 | ((vtile & 0x1C) << 1) | (tt >> 2);
            return exram[attribute_address];
        }
        else if (address >= 0x2000) {
            if ((address & 0x23FF) < 0x23C0) {
                if ((split_control & 0x80) && tt < 33) {
                    if (((split_control & 0x40) && (tt >= (split_control & 0x1F))) ||
                        (!(split_control & 0x40) && (tt < (split_control & 0x1F))))
                        in_split_region = 1;
                }
            }
            //int t = address & 0x3FF;
            last_tile = address & 0x3FF;
            if (in_split_region) {
                int t = vscroll;
                t = (t & 0xF8) << 2;
                t |= (tt & 0x1F);
                split_address = t & 0x3FF;
                return *(exram + split_address);
            }
            else if (name_table[address >> 10 & 3] == 0) {
                using_fill_table = 1;
                return fillTile;
            }
            else
                return *(name_table[(address >> 10) & 3] + (address & 0x3FF));
        }
        else {
            return read_chr(address);
        }
        return 0;
    }

    void ppu_write(unsigned short address, unsigned char value) override
    {
        address &= 0x3FFF;
        //unsure why this is necessary
        if (address >= 0x2000 && name_table[(address >> 10) & 3] == 0)
            return;
        else
            c_mapper::ppu_write(address, value);
    }

    void clock_frame()
    {
        frame_seq_counter -= 12;
        if (frame_seq_counter < 0) {
            frame_seq_counter += CLOCKS_PER_FRAME_SEQ;
            for (int i = 0; i < 2; i++)
                squares[i].clock_length();
        }
        for (int i = 0; i < 2; i++)
            squares[i].clock_timer();
    }

};

} //namespace nes
