module;

export module nes:mapper.mapper228;
import :mapper;
import class_registry;
import nemulator.std;

namespace nes
{

//Action 52, Cheetamen II

class c_mapper228 : public c_mapper, register_class<nes_mapper_registry, c_mapper228>
{
  public:
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 228,
                .constructor = []() { return std::make_unique<c_mapper228>(); },
            },
        };
    }

    void write_byte(unsigned short address, unsigned char value) override
    {
        if (address >= 0x8000) {
            int prg_mode = address & 0x20;
            int prg_page = (address >> 6) & 0x1F;
            const int prg_layout[] = {0, 1, 0, 2};
            int prg_chip = prg_layout[(address >> 11) & 0x03];

            SetChrBank8k(((address & 0x0F) << 2) | value & 0x3);
            if (address & 0x2000)
                set_mirroring(MIRRORING_HORIZONTAL);
            else
                set_mirroring(MIRRORING_VERTICAL);
            if (prg_mode) {
                int a = 1;
            //no idea if this is correct.  cheetamen ii and action 52
            //both seem to use the other mode
                int page = prg_page + (prg_chip * 32);
                SetPrgBank16k(PRG_8000, page);
                SetPrgBank16k(PRG_C000, page);
            }
            else {
                SetPrgBank32k((prg_page >> 1) + (prg_chip * 16));
            }
        }
        else if (address >= 0x4020 && address < 0x6000) {
            regs[address & 0x03] = value & 0x0F;
        }
        else
            c_mapper::write_byte(address, value);
    }

    unsigned char read_byte(unsigned short address) override
    {
        if (address >= 0x4020 && address < 0x6000)
            return regs[address & 0x03];
        else
            return c_mapper::read_byte(address);
    }

    void reset() override
    {
        SetChrBank8k(0);
        for (int i = 0; i < 4; i++)
            regs[i] = 0;
    }

  private:
    int regs[4];
};

} //namespace nes
