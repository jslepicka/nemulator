export module nes:callbacks;
import nemulator.std;

//these are all of the callbacks from components thorugh the nes system
//can think of it like a bus interface
//this pattern allows the compiler to optimize away virtual calls

namespace nes
{

export template <typename Derived> class i_nes_callbacks
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
    // clang-format off
    void on_ppu_clock() { derived()->_on_ppu_clock(); }
    void on_cpu_clock() { derived()->_on_cpu_clock(); }
    void on_sprite_eval(bool in_eval) { derived()->_on_sprite_eval(in_eval); }
    void on_nmi(bool nmi) { derived()->_on_nmi(nmi); }
    void on_irq(bool irq) { derived()->_on_irq(irq); }
    uint8_t ppu_read(uint16_t address) { return derived()->_ppu_read(address); }
    void ppu_write(uint16_t address, uint8_t values) { derived()->_ppu_write(address, values); }
    void add_cycle() { derived()->_add_cycle(); }
    uint8_t dmc_read(uint16_t address) { return derived()->_dmc_read(address); }
    float mix_mapper_audio(float sample) { return derived()->_mix_mapper_audio(sample); }
    uint8_t read_byte(uint16_t address) { return derived()->_read_byte(address); }
    void write_byte(uint16_t address, uint8_t value) { derived()->_write_byte(address, value); }
    // clang-format on
};

} //namespace nes