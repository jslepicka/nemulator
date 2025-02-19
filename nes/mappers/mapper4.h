#pragma once
#include "..\mapper.h"

namespace nes {

class c_mapper4 : public c_mapper, register_class<nes_mapper_registry, c_mapper4>
{
public:
    c_mapper4();
    ~c_mapper4();
    unsigned char ppu_read(unsigned short address);
    void ppu_write(unsigned short address, unsigned char value);
    virtual void write_byte(unsigned short address, unsigned char value);
    virtual unsigned char read_byte(unsigned short address);
    virtual void reset();
    virtual void clock(int cycles);

    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 4,
                .name = "MMC3",
                .constructor = []() { return std::make_unique<c_mapper4>(); },
            },
            {
                .number = 220,
                .name = "MMC3 (FCEUX Debugging)",
                .constructor = []() { return std::make_unique<c_mapper4>(); },
            },
        };
    }
protected:
    virtual void fire_irq();
    int irq_mode;
    void clock_irq_counter();
    int current_address;
    int previous_address;
    int low_count;
    void check_a12(int address);
    bool prgSelect;
    bool chrXor;
    int commandNumber;
    int lastbank;
    bool irqEnabled;
    int irqLatch;
    unsigned char irqCounter;
    int irqCounterReload;
    bool irqReload;
    bool fireIRQ;
    int irq_asserted;

    int chr[6];
    int prg[2];
    virtual void Sync();

    int prg_offset;
    int chr_offset;
    int prg_xor;
    int chr_xor;

    int prg_mask;
    int chr_mask;

    int last_prg_page;
    int four_screen;
};

} //namespace nes
