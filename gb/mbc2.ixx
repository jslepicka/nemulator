module;
#include <cstdint>
export module gb:mapper.mbc2;
import :mapper;

namespace gb
{

class c_mbc2 : public c_gbmapper
{
  public:
    c_mbc2();
    void write_byte(uint16_t address, uint8_t data);
    uint8_t read_byte(uint16_t address);
    void reset();

  private:
    int bank;
    int ram_bank;
    int ram_enable;
    int mode;
    int rom_banks;
};

} //namespace gb