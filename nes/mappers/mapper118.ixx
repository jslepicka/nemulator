export module nes_mapper.mapper118;
import nes_mapper;

import nes_mapper.mapper4;

namespace nes
{

//Alien Syndrome

class c_mapper118 : public c_mapper4, register_class<nes_mapper_registry, c_mapper118>
{
  public:
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 118,
                .constructor = []() { return std::make_unique<c_mapper118>(); },
            },
        };
    }

    void write_byte(unsigned short address, unsigned char value) override
    {
        switch (address & 0xE001) {
            case 0xA000:
                break;
            default:
                c_mapper4::write_byte(address, value);
                break;
        }
    }

  private:
    void Sync()
    {
        int _or = chr_xor >> 1;
        int add = chr_xor >> 2;

        name_table[0] = &vram[0x400 * ((chr[0 | _or] & 0x80) >> 7)];
        name_table[1] = &vram[0x400 * ((chr[(0 | _or) + add] & 0x80) >> 7)];
        name_table[2] = &vram[0x400 * ((chr[1 << _or] & 0x80) >> 7)];
        name_table[3] = &vram[0x400 * ((chr[(1 << _or) + add] & 0x80) >> 7)];

        c_mapper4::Sync();
    }
};

} //namespace nes
