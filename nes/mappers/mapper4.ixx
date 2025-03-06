module;

export module nes:mapper.mapper4;
import :mapper;
import class_registry;
import std;

namespace nes
{

export class c_mapper4 : public c_mapper, register_class<nes_mapper_registry, c_mapper4>
{
  public:
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 4,
                .name = "MMC3",
                .constructor = []() { return std::make_unique<c_mapper4>(); },
            },
            {
                .number = 220,
                .name = "MMC3 (FCEUX Debugging)",
                .constructor = []() { return std::make_unique<c_mapper4>(); },
            },
        };
    }

    unsigned char ppu_read(unsigned short address) override
    {
        if (!in_sprite_eval)
            check_a12(address);
        return c_mapper::ppu_read(address);
    }

    void ppu_write(unsigned short address, unsigned char value) override
    {
        if (!in_sprite_eval)
            check_a12(address);
        c_mapper::ppu_write(address, value);
    }

    void write_byte(unsigned short address, unsigned char value) override
    {
        if (address >= 0x8000) {
            switch (address & 0xE001) {
                case 0x8000:
                    commandNumber = value & 0x07;
                    prgSelect = value & 0x40 ? true : false;
                    prg_xor = prgSelect << 1;
                    chrXor = value & 0x80 ? true : false;
                    chr_xor = chrXor << 2;
                    Sync();
                    break;
                case 0x8001:
                    switch (commandNumber) {
                        case 0:
                        case 1:
                            chr[commandNumber] = value & 0xFE;
                            break;
                        case 2:
                        case 3:
                        case 4:
                        case 5:
                            chr[commandNumber] = value;
                            break;
                        case 6:
                        case 7:
                            prg[commandNumber - 6] = value;
                            break;
                    }
                    Sync();
                    break;
                case 0xa000:
                    if (!four_screen) {
                        if (value & 0x01)
                            set_mirroring(MIRRORING_HORIZONTAL);
                        else
                            set_mirroring(MIRRORING_VERTICAL);
                    }
                    break;
                case 0xa001:
                    writeProtectSram = value & 0x40 ? true : false;
                    sram_enabled = value & 0x80;
                    break;
                case 0xc000:
                    //if (value == 0)
                    //    fireIRQ = true;
                    irqCounterReload = value;
                    //irqCounter = Value;
                    break;
                case 0xc001:
                    irqReload = true;
                    //probably should be commented out
                    //irqCounter = 0;
                    //irqLatch = Value;
                    break;
                case 0xe000:
                    irqEnabled = false;
                    if (irq_asserted) {
                        clear_irq();
                        irq_asserted = 0;
                    }
                    break;
                case 0xe001:
                    irqEnabled = true;
                    break;
                default:
                    break;
            }
        }
        else
            c_mapper::write_byte(address, value);
    }

    unsigned char read_byte(unsigned short address) override
    {
        if (submapper == 1) {
            switch (address >> 12) {
                case 6:
                case 7:
                    return address >> 8;
                    break;
                default:
                    return c_mapper::read_byte(address);
            }
        }
        else
            return c_mapper::read_byte(address);
    }

    void reset() override
    {
        if (header->Rcb1.Fourscreen) {
            //if four screen, disallow changes to mirroring mode
            four_screen = 1;
        }

        irqCounter = 0;
        irqEnabled = false;
        irqLatch = 0;
        irqReload = false;
        irqCounterReload = 0;
        fireIRQ = false;
        previous_address = 0;
        current_address = 100;
        low_count = 0;
        last_prg_page = prgRomPageCount8k - 1;
        irq_mode = 1;
        irq_asserted = 0;

        prg_xor = 0;
        chr_xor = 0;
        prg_offset = 0;
        chr_offset = 0;
        prg_mask = 0xFF;
        chr_mask = 0xFF;

        if (submapper == 2) {
            irq_mode = 0;
        }

        //TODO: Not sure if these are the correct initialization values

        prg[0] = 0;
        prg[1] = 1;
        chr[0] = 0;
        chr[1] = 2;
        chr[2] = 4;
        chr[3] = 5;
        chr[4] = 6;
        chr[5] = 7;
        Sync();
    }

    void clock(int cycles) override
    {
        if (!(current_address & 0x1000)) {
            low_count -= cycles;
            if (low_count < 0)
                low_count = 0;
        }
        else
            low_count = 12; //12 required for wario's woods
    }

  protected:
    int irq_mode;
    int current_address;
    int previous_address;
    int low_count;
    bool prgSelect;
    bool chrXor;
    bool irqEnabled;
    bool irqReload;
    unsigned char irqCounter;
    bool fireIRQ;
    int commandNumber;
    int lastbank;
    int irqLatch;
    int irqCounterReload;
    int irq_asserted;
    int chr[6];
    int prg[2];
    int prg_offset;
    int chr_offset;
    int prg_xor;
    int chr_xor;
    int prg_mask;
    int chr_mask;
    int last_prg_page;
    int four_screen = 0;

    virtual void fire_irq()
    {
        execute_irq();
        irq_asserted = 1;
    }
    void clock_irq_counter()
    {
        /*

        MMC3A and non-Sharp MMC3B edge-detect
        Sharp-MMC3B and MMC3C level-detect *DEFAULT BEHAVIOR*

        Only matters when irqCounterReload is set to 0, but some games rely on specific behavior.
        These games need to be CRC-detected.

        */
        unsigned char previous_count = irqCounter;
        if (irqCounter == 0 || irqReload) {
            irqCounter = irqCounterReload;
            irqReload = false;
        }
        else {
            irqCounter--;
        }

        if ((irq_mode == 1 || previous_count != 0) && irqCounter == 0 && irqEnabled) {
            if (!irq_asserted) {
                fire_irq();
            }
        }
    }

    void check_a12(int address)
    {
        current_address = address;

        if ((address & 0x1000) && low_count == 0)
            clock_irq_counter();
    }

    virtual void Sync()
    {
        SetPrgBank8k(PRG_8000 ^ prg_xor, (prg[0] & prg_mask) + prg_offset);
        SetPrgBank8k(PRG_A000, (prg[1] & prg_mask) + prg_offset);
        SetPrgBank8k(PRG_C000 ^ prg_xor, ((last_prg_page - 1) & prg_mask) + prg_offset);
        SetPrgBank8k(PRG_E000, (last_prg_page & prg_mask) + prg_offset);

        SetChrBank1k(CHR_0000 ^ chr_xor, (chr[0] & chr_mask) + chr_offset);
        SetChrBank1k(CHR_0400 ^ chr_xor, (chr[0] & chr_mask) + 1 + chr_offset);
        SetChrBank1k(CHR_0800 ^ chr_xor, (chr[1] & chr_mask) + chr_offset);
        SetChrBank1k(CHR_0C00 ^ chr_xor, (chr[1] & chr_mask) + 1 + chr_offset);
        SetChrBank1k(CHR_1000 ^ chr_xor, (chr[2] & chr_mask) + chr_offset);
        SetChrBank1k(CHR_1400 ^ chr_xor, (chr[3] & chr_mask) + chr_offset);
        SetChrBank1k(CHR_1800 ^ chr_xor, (chr[4] & chr_mask) + chr_offset);
        SetChrBank1k(CHR_1C00 ^ chr_xor, (chr[5] & chr_mask) + chr_offset);
    }
};

} //namespace nes
