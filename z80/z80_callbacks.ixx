export module z80:callbacks;
import nemulator.std;

export template <typename Derived> class i_z80_callbacks
{
    __forceinline Derived *derived() noexcept
    {
        //without this assume, the compiler generates null pointer checks before each
        //call, reducing performance
        __assume(this != nullptr);
        Derived *d = static_cast<Derived *>(this);
        return d;
    }
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