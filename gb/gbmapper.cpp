module;

module gb:mapper;

namespace gb
{

c_gbmapper::c_gbmapper()
{
    ram = nullptr;
    rumble = false;
}

c_gbmapper::~c_gbmapper()
{
}

void c_gbmapper::reset()
{
}

uint8_t c_gbmapper::read_byte(uint16_t address)
{
    if (address < 0x8000) {
        return *(rom + address);
    }
    else {
        //printf("invalid address %d reading from cart\n", address);
        return 0;
    }
}

void c_gbmapper::write_byte(uint16_t address, uint8_t data)
{
}

void c_gbmapper::config_ram(int ram_size)
{
    this->ram_size = ram_size;
    ram = std::make_unique<uint8_t[]>(ram_size);
}

} //namespace gb