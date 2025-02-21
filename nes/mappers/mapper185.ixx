export module nes_mapper.mapper185;
import nes_mapper;
import class_registry;
import std;

namespace nes
{

class c_mapper185 : public c_mapper, register_class<nes_mapper_registry, c_mapper185>
{
  public:
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 185,
                .constructor = []() { return std::make_unique<c_mapper185>(); },
            },
        };
    }

    void write_byte(unsigned short address, unsigned char value) override
    {
        if (address >= 0x8000) {
            if (value & 0x0F && value != 0x13)
                chr_protected = 0;
            else
                chr_protected = 1;
        }
        else
            c_mapper::write_byte(address, value);
    }

    unsigned char read_chr(unsigned short address) override
    {
        if (chr_protected)
            return 0x12;
        else
            return c_mapper::read_chr(address);
    }

    void reset() override
    {
        chr_protected = 0;
        c_mapper::reset();
    }

  private:
    int chr_protected;
};

} //namespace nes
