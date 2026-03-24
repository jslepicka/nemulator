export module m68k:callbacks;
import nemulator.std;

export template <typename Derived> class i_m68k_callbacks
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
	uint8_t m68k_read_byte(uint32_t address)
	{
		return derived()->_m68k_read_byte(address);
	}
	void m68k_write_byte(uint32_t address, uint8_t data)
	{
		derived()->_m68k_write_byte(address, data);
	}
	uint16_t m68k_read_word(uint32_t address)
	{
		return derived()->_m68k_read_word(address);
	}
	void m68k_write_word(uint32_t address, uint16_t data)
	{
		derived()->_m68k_write_word(address, data);
	}
};