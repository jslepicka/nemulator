export module nes:interfaces;
import nemulator.std;

//these are all of the callbacks from components thorugh the nes system
//can think of it like a bus interface
//this pattern allows the compiler to optimize away virtual calls

export template <typename Derived> class i_nes_callbacks
{
  public:
    void on_ppu_clock() { static_cast<Derived *>(this)->_on_ppu_clock(); }
    void on_cpu_clock() { static_cast<Derived *>(this)->_on_cpu_clock(); }
    void on_sprite_eval(bool in_eval) { static_cast<Derived *>(this)->_on_sprite_eval(in_eval); }
    void on_nmi(bool nmi) { static_cast<Derived *>(this)->_on_nmi(nmi); }
    uint8_t ppu_read(uint16_t address) { return static_cast<Derived *>(this)->_ppu_read(address); }
    void ppu_write(uint16_t address, uint8_t values) { static_cast<Derived *>(this)->_ppu_write(address, values); }
    void add_cycle() { static_cast<Derived *>(this)->_add_cycle(); }
};