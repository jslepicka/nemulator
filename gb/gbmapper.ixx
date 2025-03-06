module;

export module gb:mapper;
import std.compat;

namespace gb
{

class c_gbmapper
{
  public:
    c_gbmapper();
    virtual ~c_gbmapper();
  //bool load_rom(std::string filename);
    virtual void write_byte(uint16_t address, uint8_t data);
    virtual uint8_t read_byte(uint16_t address);
    virtual void reset();
    virtual void config_ram(int ram_size);
    bool rumble;
    uint8_t *rom;
    std::unique_ptr<uint8_t[]> ram;

  private:
  protected:
    uint32_t ram_size;
};

} //namespace gb