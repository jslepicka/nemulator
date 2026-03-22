export module z80:callbacks;
import callback;
import nemulator.std;

export template <typename Derived> class i_z80_callbacks : public i_callback<Derived>
{
    using i_callback<Derived>::derived;

  public:
    uint8_t z80_read_byte(uint16_t address)
    {
        return derived()->_z80_read_byte(address);
    }
    void z80_write_byte(uint16_t address, uint8_t data)
    {
        derived()->_z80_write_byte(address, data);
    }
    void z80_int_ack()
    {
        derived()->_z80_int_ack();
    }
    uint8_t z80_read_port(uint8_t port)
    {
        return derived()->_z80_read_port(port);
    }
    void z80_write_port(uint8_t port, uint8_t data)
    {
        derived()->_z80_write_port(port, data);
    }
    
};