#include "cpu.h"
#include "mapper.h"
#include "nes.h"
#include "ppu.h"

#define INLINE __forceinline
//#define INLINE

namespace nes {

//replaced all zeros with 3 so that invalid opcodes don't cause cpu loop to spin
const int c_cpu::cycle_table[] = {
//       0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
        21, 18,  3, 24,  9,  9, 15, 15,  9,  6,  6,  6, 12, 12, 18, 18, //0F
         6, 15,  3, 24, 12, 12, 18, 18,  6, 12,  6, 21, 12, 12, 21, 21, //1F
        18, 18,  3, 24,  9,  9, 15, 15, 12,  6,  6,  6, 12, 12, 18, 18, //2F
         6, 15,  3, 24, 12, 12, 18, 18,  6, 12,  6, 21, 12, 12, 21, 21, //3F
        18, 18,  3, 24,  9,  9, 15, 15,  9,  6,  6,  6,  9, 12, 18, 18, //4F
         6, 15,  3, 24, 12, 12, 18, 18,  6, 12,  6, 21, 12, 12, 21, 21, //5F
        18, 18,  3, 24,  9,  9, 15, 15, 12,  6,  6,  6, 15, 12, 18, 18, //6F
         6, 15,  3, 24, 12, 12, 18, 18,  6, 12,  6, 21, 12, 12, 21, 21, //7F
         6, 18,  6, 18,  9,  9,  9,  9,  6,  6,  6,  6, 12, 12, 12, 12, //8F
         6, 18,  3, 18, 12, 12, 12, 12,  6, 15,  6, 15, 15, 15, 15, 15, //9F
         6, 18,  6, 18,  9,  9,  9,  9,  6,  6,  6,  6, 12, 12, 12, 12, //AF
         6, 15,  3, 15, 12, 12, 12, 12,  6, 12,  6, 12, 12, 12, 12, 12, //BF
         6, 18,  6, 24,  9,  9, 15, 15,  6,  6,  6,  6, 12, 12, 18, 18, //CF
         6, 15,  3, 24, 12, 12, 18, 18,  6, 12,  6, 21, 12, 12, 21, 21, //DF
         6, 18,  6, 24,  9,  9, 15, 15,  6,  6,  6,  6, 12, 12, 18, 18, //EF
         6, 15,  3, 24, 12, 12, 18, 18,  6, 12,  6, 21, 12, 12, 21, 21, //FF
        21, 21,  6, 12
};


c_cpu::c_cpu()
{
    nes = 0;
}

c_cpu::~c_cpu()
{
}

void c_cpu::add_cycle()
{
    available_cycles++;
}

int c_cpu::reset()
{
    cycles = 0;
    nmi_delay = 0;
    irq_delay = 0;
    S = (unsigned char*)&SR;
    *S = 0;
    SR.Unused = 1;
    SR.I = 1;
    do_nmi = false;
    do_irq = false;
    available_cycles = 0;
    fetch_opcode = true;
    opcode = 0;
    required_cycles = 0;
    irq_pending = false;
    nmi_pending = false;
    PC = makeword(nes->read_byte(0xFFFC), nes->read_byte(0xFFFD));
    SP = 0xFF;
    dma_dst = 0;
    dma_src = 0;
    dma_pos = 0;
    do_apu_dma = 0;
    odd_cycle = 0;
    return 1;
}

void c_cpu::execute()
{
    //for (;;)
    //{
        if (fetch_opcode)
        {
            if (do_apu_dma)
            {
                opcode = 0x103;
                do_apu_dma--;
            }
            else if (dma_pos)
            {
                opcode = 0x102;
            }
            else if (do_nmi && nmi_delay == 0)
            {
                opcode = 0x100;
                do_nmi = false;
                nmi_delay = 0;
            }
            else if (do_irq && irq_delay == 0 && !SR.I)
            {
                opcode = 0x101;
                irq_delay = 0;
            }
            else {
                opcode = nes->read_byte(PC++);
            }
            irq_delay = 0;
            nmi_delay = 0;
            required_cycles += cycle_table[opcode];
            fetch_opcode = false;
        }
        if (required_cycles <= available_cycles)
        {
            available_cycles -= required_cycles;
            required_cycles = 0;
            fetch_opcode = true;
            execute_opcode();
        }
    //    else
    //        break;
    //}
}

void c_cpu::do_sprite_dma(unsigned char* dst, int source_address)
{
    dma_dst = dst;
    dma_src = source_address;
    dma_pos = 256;
}

void c_cpu::execute_opcode()
{
    //int sl = nes->ppu->current_scanline;
    //int c = nes->ppu->current_cycle;
    switch (opcode)
    {
    case 0x00: BRK(); break;
    case 0x01: indirect_x(); ORA(); break;
    case 0x02: STP(); break;
    case 0x03: indirect_x(); SLO(); break;
    case 0x04: zeropage(); break; //unofficial d NOP
    case 0x05: zeropage(); ORA(); break;
    case 0x06: zeropage(); ASL(); break;
    case 0x07: zeropage(); SLO(); break;
    case 0x08: PHP(); break;
    case 0x09: immediate(); ORA(); break;
    case 0x0A: ASL_R(A); break;
    case 0x0B: immediate(); ANC(); break; //unofficial ANC
    case 0x0C: absolute(); break; //unofficial a NOP
    case 0x0D: absolute(); ORA(); break;
    case 0x0E: absolute(); ASL(); break;
    case 0x0F: absolute(); SLO(); break;
    case 0x10: BPL(); break;
    case 0x11: indirect_y_pc(); ORA(); break;
    case 0x12: STP(); break;
    case 0x13: indirect_y(); SLO(); break;
    case 0x14: zeropage_x(); break; //unofficial d,x NOP
    case 0x15: zeropage_x(); ORA(); break;
    case 0x16: zeropage_x(); ASL(); break;
    case 0x17: zeropage_x(); SLO(); break;
    case 0x18: CLC(); break;
    case 0x19: absolute_y_pc(); ORA(); break;
    case 0x1A: break; //unofficial NOP
    case 0x1B: absolute_y(); SLO(); break;
    case 0x1C: absolute_x_pc(); break; //unofficial a,x NOP
    case 0x1D: absolute_x_pc(); ORA(); break;
    case 0x1E: absolute_x_rmw(); ASL(); break;
    case 0x1F: absolute_x_rmw(); SLO(); break;
    case 0x20: absolute(); JSR(); break;
    case 0x21: indirect_x(); AND(); break;
    case 0x22: STP(); break;
    case 0x23: indirect_x(); RLA(); break;
    case 0x24: zeropage(); BIT(); break;
    case 0x25: zeropage(); AND(); break;
    case 0x26: zeropage(); ROL(); break;
    case 0x27: zeropage(); RLA(); break;
    case 0x28: PLP(); break;
    case 0x29: immediate(); AND(); break;
    case 0x2A: ROL_R(A); break;
    case 0x2B: immediate(); ANC(); break; //unofficial ANC
    case 0x2C: absolute(); BIT(); break;
    case 0x2D: absolute(); AND(); break;
    case 0x2E: absolute(); ROL(); break;
    case 0x2F: absolute(); RLA(); break;
    case 0x30: BMI(); break;
    case 0x31: indirect_y_pc(); AND(); break;
    case 0x32: STP(); break;
    case 0x33: indirect_y(); RLA(); break;
    case 0x34: zeropage_x(); break; //unofficial d,x NOP
    case 0x35: zeropage_x(); AND(); break;
    case 0x36: zeropage_x(); ROL(); break;
    case 0x37: zeropage_x(); RLA(); break;
    case 0x38: SEC(); break;
    case 0x39: absolute_y_pc(); AND(); break;
    case 0x3A: break; //unofficial NOP
    case 0x3B: absolute_y(); RLA(); break;
    case 0x3C: absolute_x_pc(); break; //unofficial a,x NOP
    case 0x3D: absolute_x_pc(); AND(); break;
    case 0x3E: absolute_x_rmw(); ROL(); break;
    case 0x3F: absolute_x_rmw(); RLA(); break;
    case 0x40: RTI(); break;
    case 0x41: indirect_x(); EOR(); break;
    case 0x42: STP(); break;
    case 0x43: indirect_x(); SRE(); break;
    case 0x44: zeropage(); break; //unofficial d NOP
    case 0x45: zeropage(); EOR(); break;
    case 0x46: zeropage(); LSR(); break;
    case 0x47: zeropage(); SRE(); break;
    case 0x48: PHA(); break;
    case 0x49: immediate(); EOR(); break;
    case 0x4A: LSR_R(A); break;
    case 0x4B: ALR(); break;
    case 0x4C: absolute(); JMP(); break;
    case 0x4D: absolute(); EOR(); break;
    case 0x4E: absolute(); LSR(); break;
    case 0x4F: absolute(); SRE(); break;
    case 0x50: BVC(); break;
    case 0x51: indirect_y_pc(); EOR(); break;
    case 0x52: STP(); break;
    case 0x53: indirect_y(); SRE(); break;
    case 0x54: zeropage_x(); break; //unofficial d,x NOP
    case 0x55: zeropage_x(); EOR(); break;
    case 0x56: zeropage_x(); LSR(); break;
    case 0x57: zeropage_x(); SRE(); break;
    case 0x58: CLI(); break;
    case 0x59: absolute_y_pc(); EOR(); break;
    case 0x5A: break; //unofficial NOP
    case 0x5B: absolute_y(); SRE(); break;
    case 0x5C: absolute_x_pc(); break; //unofficial a,x NOP
    case 0x5D: absolute_x_pc(); EOR(); break;
    case 0x5E: absolute_x_rmw(); LSR(); break;
    case 0x5F: absolute_x_rmw(); SRE(); break;
    case 0x60: RTS(); break;
    case 0x61: indirect_x(); ADC(); break;
    case 0x62: STP(); break;
    case 0x63: indirect_x(); RRA(); break;
    case 0x64: zeropage(); break; //unofficial d NOP
    case 0x65: zeropage(); ADC(); break;
    case 0x66: zeropage(); ROR(); break;
    case 0x67: zeropage(); RRA(); break;
    case 0x68: PLA(); break;
    case 0x69: immediate(); ADC(); break;
    case 0x6A: ROR_R(A); break;
    case 0x6B: immediate(); ARR(); break; //unofficial ARR
    case 0x6C: indirect(); JMP(); break;
    case 0x6D: absolute(); ADC(); break;
    case 0x6E: absolute(); ROR(); break;
    case 0x6F: absolute(); RRA(); break;
    case 0x70: BVS(); break;
    case 0x71: indirect_y_pc(); ADC(); break;
    case 0x72: STP(); break;
    case 0x73: indirect_y(); RRA(); break;
    case 0x74: zeropage_x(); break; //unofficial d,x NOP
    case 0x75: zeropage_x(); ADC(); break;
    case 0x76: zeropage_x(); ROR(); break;
    case 0x77: zeropage_x(); RRA(); break;
    case 0x78: SEI(); break;
    case 0x79: absolute_y_pc(); ADC(); break;
    case 0x7A: break; //unofficial NOP
    case 0x7B: absolute_y(); RRA(); break;
    case 0x7C: absolute_x_pc(); break; //unofficial a,x NOP
    case 0x7D: absolute_x_pc(); ADC(); break;
    case 0x7E: absolute_x_rmw(); ROR(); break;
    case 0x7F: absolute_x_rmw(); RRA(); break;
    case 0x80: immediate(); break; //unofficial i NOP
    case 0x81: indirect_x_ea(); STA(); break;
    case 0x82: immediate(); break; //unofficial i NOP
    case 0x83: indirect_x(); SAX(); break;
    case 0x84: zeropage_ea(); STY(); break;
    case 0x85: zeropage_ea(); STA(); break;
    case 0x86: zeropage_ea(); STX(); break;
    case 0x87: zeropage_ea(); SAX(); break;
    case 0x88: DEY(); break;
    case 0x89: SKB(); break;
    case 0x8A: TXA(); break;
    case 0x8B: immediate();  XAA(); break;
    case 0x8C: absolute_ea(); STY(); break;
    case 0x8D: absolute_ea(); STA(); break;
    case 0x8E: absolute_ea(); STX(); break;
    case 0x8F: absolute_ea(); SAX(); break;
    case 0x90: BCC(); break;
    case 0x91: indirect_y_ea(); STA(); break;
    case 0x92: STP(); break;
    case 0x93: indirect_y(); SHA(); break;
    case 0x94: zeropage_x_ea(); STY(); break;
    case 0x95: zeropage_x_ea(); STA(); break;
    case 0x96: zeropage_y_ea(); STX(); break;
    case 0x97: zeropage_y_ea(); SAX(); break;
    case 0x98: TYA(); break;
    case 0x99: absolute_y_ea(); STA(); break;
    case 0x9A: TXS(); break;
    case 0x9B: absolute_y(); TAS(); break;
    case 0x9C: absolute_x(); SHY(); break;
    case 0x9D: absolute_x_ea(); STA(); break;
    case 0x9E: absolute_y(); SHX(); break;
    case 0x9F: absolute_y(); SHA(); break;
    case 0xA0: immediate(); LDY(); break;
    case 0xA1: indirect_x(); LDA(); break;
    case 0xA2: immediate(); LDX(); break;
    case 0xA3: indirect_x(); LAX(); break;
    case 0xA4: zeropage(); LDY(); break;
    case 0xA5: zeropage(); LDA(); break;
    case 0xA6: zeropage(); LDX(); break;
    case 0xA7: zeropage(); LAX(); break;
    case 0xA8: TAY(); break;
    case 0xA9: immediate(); LDA(); break;
    case 0xAA: TAX(); break;
    case 0xAB: immediate(); LAX(); break;
    case 0xAC: absolute(); LDY(); break;
    case 0xAD: absolute(); LDA(); break;
    case 0xAE: absolute(); LDX(); break;
    case 0xAF: absolute(); LAX(); break;
    case 0xB0: BCS(); break;
    case 0xB1: indirect_y_pc(); LDA(); break;
    case 0xB2: STP(); break;
    case 0xB3: indirect_y_pc(); LAX(); break;
    case 0xB4: zeropage_x(); LDY(); break;
    case 0xB5: zeropage_x(); LDA(); break;
    case 0xB6: zeropage_y(); LDX(); break;
    case 0xB7: zeropage_y(); LAX(); break;
    case 0xB8: CLV(); break;
    case 0xB9: absolute_y_pc(); LDA(); break;
    case 0xBA: TSX(); break;
    case 0xBB: absolute_y_pc(); LAS(); break;
    case 0xBC: absolute_x_pc(); LDY(); break;
    case 0xBD: absolute_x_pc(); LDA(); break;
    case 0xBE: absolute_y_pc(); LDX(); break;
    case 0xBF: absolute_y_pc(); LAX(); break;
    case 0xC0: immediate(); CPY(); break;
    case 0xC1: indirect_x(); CMP(); break;
    case 0xC2: immediate(); break; //unofficial i NOP
    case 0xC3: indirect_x(); DCP(); break;
    case 0xC4: zeropage(); CPY(); break;
    case 0xC5: zeropage(); CMP(); break;
    case 0xC6: zeropage(); DEC(); break;
    case 0xC7: zeropage(); DCP(); break;
    case 0xC8: INY(); break;
    case 0xC9: immediate(); CMP(); break;
    case 0xCA: DEX(); break;
    case 0xCB: immediate();  AXS(); break;
    case 0xCC: absolute(); CPY(); break;
    case 0xCD: absolute(); CMP(); break;
    case 0xCE: absolute(); DEC(); break;
    case 0xCF: absolute(); DCP(); break;
    case 0xD0: BNE(); break;
    case 0xD1: indirect_y_pc(); CMP(); break;
    case 0xD2: STP(); break;
    case 0xD3: indirect_y(); DCP(); break;
    case 0xD4: zeropage_x(); break; //unofficial d,x NOP
    case 0xD5: zeropage_x(); CMP(); break;
    case 0xD6: zeropage_x(); DEC(); break;
    case 0xD7: zeropage_x(); DCP(); break;
    case 0xD8: CLD(); break;
    case 0xD9: absolute_y_pc(); CMP(); break;
    case 0xDA: break; //unofficial NOP
    case 0xDB: absolute_y(); DCP(); break;
    case 0xDC: absolute_x_pc(); break; //unofficial a,x NOP
    case 0xDD: absolute_x_pc(); CMP(); break;
    case 0xDE: absolute_x_rmw(); DEC(); break;
    case 0xDF: absolute_x_rmw(); DCP(); break;
    case 0xE0: immediate(); CPX(); break;
    case 0xE1: indirect_x(); SBC(); break;
    case 0xE2: immediate(); break; //unofficial i NOP
    case 0xE3: indirect_x(); ISC(); break;
    case 0xE4: zeropage(); CPX(); break;
    case 0xE5: zeropage(); SBC(); break;
    case 0xE6: zeropage(); INC(); break;
    case 0xE7: zeropage(); ISC(); break;
    case 0xE8: INX(); break;
    case 0xE9: immediate(); SBC(); break;
    case 0xEA: break;
    case 0xEB: immediate(); SBC(); break;
    case 0xEC: absolute(); CPX(); break;
    case 0xED: absolute(); SBC(); break;
    case 0xEE: absolute(); INC(); break;
    case 0xEF: absolute(); ISC(); break;
    case 0xF0: BEQ(); break;
    case 0xF1: indirect_y_pc(); SBC(); break;
    case 0xF2: STP(); break;
    case 0xF3: indirect_y(); ISC(); break;
    case 0xF4: zeropage_x(); break; //unofficial d,x NOP
    case 0xF5: zeropage_x(); SBC(); break;
    case 0xF6: zeropage_x(); INC(); break;
    case 0xF7: zeropage_x(); ISC(); break;
    case 0xF8: SED(); break;
    case 0xF9: absolute_y_pc(); SBC(); break;
    case 0xFA: break; //unofficial NOP
    case 0xFB: absolute_y(); ISC(); break;
    case 0xFC: absolute_x_pc(); break; //unofficial a,x NOP
    case 0xFD: absolute_x_pc(); SBC(); break;
    case 0xFE: absolute_x_rmw(); INC(); break;
    case 0xFF: absolute_x_rmw(); ISC(); break;
    case 0x100: //NMI
        //Battletoads debugging
    //{
    //    char x[256];
    //    sprintf(x, "NMI at scanline %d, cycle %d\n", sl, c);
    //    OutputDebugString(x);
    //}
        push(hibyte(PC));
        push(lobyte(PC));
        SR.B = false;
        push(*S);
        SR.I = true;
        PC = makeword(nes->read_byte(0xFFFA), nes->read_byte(0xFFFB));
        break;
    case 0x101: //IRQ
        push(hibyte(PC));
        push(lobyte(PC));
        SR.B = false;
        push(*S);
        SR.I = true;
        PC = makeword(nes->read_byte(0xFFFE), nes->read_byte(0xFFFF));
        break;
    case 0x102: //Sprite DMA
        *(dma_dst++) = nes->read_byte(dma_src++);
        dma_pos--;

        //Sprite DMA burns 513 CPU cycles, so after the last DMA, burn another one.
        if (dma_pos == 0) {
            required_cycles += 3;
            if (odd_cycle) {
                required_cycles += 3;
            }
        }
        break;
    case 0x103: //APU DMA
        break;
    default:
        __assume(0);
    }
}

INLINE void c_cpu::push(uint8_t value)
{
    nes->write_byte(SP-- | 0x100, value);
}

INLINE uint8_t c_cpu::pop()
{
    return nes->read_byte(++SP | 0x100);
}

INLINE void c_cpu::setn(uint8_t value)
{
    SR.N = value & 0x80;
}

INLINE void c_cpu::setz(uint8_t value)
{
    SR.Z = !value;
}

INLINE uint16_t c_cpu::makeword(uint8_t lo, uint8_t hi)
{
    return (hi << 8) | lo;
}

INLINE uint8_t c_cpu::hibyte(uint16_t value)
{
    return value >> 8;
}

INLINE uint8_t c_cpu::lobyte(uint16_t value)
{
    return value & 0xFF;
}

void c_cpu::execute_apu_dma()
{
    //doApuDMA++;
    //if (doApuDMA > 1)
    //    int x = 1;
    required_cycles += 12;
}

int c_cpu::irq_checked()
{
    //irqs are checked 2 cycles before an opcode completes
    //if this check has already occured, return 1
    //return (fetchOpcode || ((requiredCycles > availableCycles) && ((requiredCycles - availableCycles) < 6)));
    if (dma_pos) {
        int x = 1;
    }
    if (fetch_opcode || available_cycles > (required_cycles - 3))
        return 1;
    else
        return 0;
}

void c_cpu::execute_nmi()
{
    //int sl = nes->ppu->current_scanline;
    //int c = nes->ppu->current_cycle;
    nmi_delay = irq_checked();
    do_nmi = true;
}

void c_cpu::execute_irq()
{
    irq_delay = irq_checked();
    do_irq = true;
}

void c_cpu::clear_irq()
{
    do_irq = false;
    nmi_delay = 0;
}

void c_cpu::clear_nmi()
{
    if (do_nmi) {
        int x = 1;
    }
    do_nmi = false;
}

INLINE void c_cpu::immediate()
{
    M = nes->read_byte(PC++);
}

INLINE void c_cpu::zeropage()
{
    address = nes->read_byte(PC++);
    M = nes->read_byte(address);
}

INLINE void c_cpu::zeropage_ea()
{
    address = nes->read_byte(PC++);
}

INLINE void c_cpu::zeropage_x()
{
    address = nes->read_byte(PC++);
    address += X;
    address &= 0xFF;
    M = nes->read_byte(address);
}

INLINE void c_cpu::zeropage_x_ea()
{
    address = nes->read_byte(PC++);
    address += X;
    address &= 0xFF;
}

INLINE void c_cpu::zeropage_y()
{
    address = nes->read_byte(PC++);
    address += Y;
    address &= 0xFF;
    M = nes->read_byte(address);
}

INLINE void c_cpu::zeropage_y_ea()
{
    address = nes->read_byte(PC++);
    address += Y;
    address &= 0xFF;
}

INLINE void c_cpu::absolute()
{
    unsigned char lo = nes->read_byte(PC++);
    unsigned char hi = nes->read_byte(PC++);
    address = makeword(lo, hi);
    M = nes->read_byte(address);
}

INLINE void c_cpu::absolute_ea()
{
    unsigned char lo = nes->read_byte(PC++);
    unsigned char hi = nes->read_byte(PC++);
    address = makeword(lo, hi);
}

INLINE void c_cpu::absolute_x()
{
    unsigned char lo = nes->read_byte(PC++);
    unsigned char hi = nes->read_byte(PC++);
    address = makeword(lo, hi);

    unsigned short intermediate = (address & 0xFF00) | ((address + X) & 0xFF);
    M = nes->read_byte(intermediate);
    address += X;
    if (address != intermediate)
    {
        M = nes->read_byte(address);
    }
}

INLINE void c_cpu::absolute_x_rmw()
{
    unsigned char lo = nes->read_byte(PC++);
    unsigned char hi = nes->read_byte(PC++);
    address = makeword(lo, hi);

    unsigned short intermediate = (address & 0xFF00) | ((address + X) & 0xFF);
    M = nes->read_byte(intermediate);
    address += X;
    M = nes->read_byte(address);
}

INLINE void c_cpu::absolute_x_pc()
{
    unsigned char lo = nes->read_byte(PC++);
    unsigned char hi = nes->read_byte(PC++);
    address = makeword(lo, hi);
    if (address == 0x2000) {
        int x = 1;
    }
    unsigned short intermediate = (address & 0xFF00) | ((address + X) & 0xFF);
    M = nes->read_byte(intermediate);
    address += X;
    if (address != intermediate)
    {
        required_cycles += 3;
        M = nes->read_byte(address);
    }
}

INLINE void c_cpu::absolute_x_ea()
{
    unsigned char lo = nes->read_byte(PC++);
    unsigned char hi = nes->read_byte(PC++);
    address = makeword(lo, hi);

    unsigned short intermediate = (address & 0xFF00) | ((address + X) & 0xFF);
    nes->read_byte(intermediate);
    address += X;
}

INLINE void c_cpu::absolute_y()
{
    unsigned char lo = nes->read_byte(PC++);
    unsigned char hi = nes->read_byte(PC++);
    address = makeword(lo, hi);

    unsigned short intermediate = (address & 0xFF00) | ((address + Y) & 0xFF);
    M = nes->read_byte(intermediate);
    address += Y;
    if (address != intermediate)
    {
        M = nes->read_byte(address);
    }
}

INLINE void c_cpu::absolute_y_pc()
{
    unsigned char lo = nes->read_byte(PC++);
    unsigned char hi = nes->read_byte(PC++);
    address = makeword(lo, hi);

    unsigned short intermediate = (address & 0xFF00) | ((address + Y) & 0xFF);
    M = nes->read_byte(intermediate);
    address += Y;
    if (address != intermediate)
    {
        required_cycles += 3;
        M = nes->read_byte(address);
    }
}

INLINE void c_cpu::absolute_y_ea()
{
    unsigned char lo = nes->read_byte(PC++);
    unsigned char hi = nes->read_byte(PC++);
    address = makeword(lo, hi);

    unsigned short intermediate = (address & 0xFF00) | ((address + Y) & 0xFF);
    nes->read_byte(intermediate);
    address += Y;
}

INLINE void c_cpu::indirect()    //For indirect jmp
{
    unsigned char lo = nes->read_byte(PC++);
    unsigned char hi = nes->read_byte(PC++);
    unsigned short tempaddress = makeword(lo, hi);
    unsigned char pcl = nes->read_byte(tempaddress);
    lo++;
    tempaddress = makeword(lo, hi);
    unsigned char pch = nes->read_byte(tempaddress);
    address = makeword(pcl, pch);
}

INLINE void c_cpu::indirect_x()    //Pre-indexed indirect
{
    //TODO: I think this is ok, but need to verify
    unsigned char hi = nes->read_byte(PC++);
    hi += X;
    address = makeword(nes->read_byte(hi), nes->read_byte((hi + 1) & 0xFF));
    M = nes->read_byte(address);
}

INLINE void c_cpu::indirect_x_ea()    //Pre-indexed indirect
{
    //TODO: I think this is ok, but need to verify
    unsigned char hi = nes->read_byte(PC++);
    hi += X;
    address = makeword(nes->read_byte(hi), nes->read_byte((hi + 1) & 0xFF));
}

INLINE void c_cpu::indirect_y()    //Post-indexed indirect
{
    unsigned char temp = nes->read_byte(PC++);
    address = makeword(nes->read_byte(temp), nes->read_byte((temp + 1) & 0xFF));

    unsigned short intermediate = (address & 0xFF00) | ((address + Y) & 0xFF);
    M = nes->read_byte(intermediate);
    address += Y;
    if (address != intermediate)
    {
        M = nes->read_byte(address);
    }
}

INLINE void c_cpu::indirect_y_pc()    //Post-indexed indirect
{
    unsigned char temp = nes->read_byte(PC++);
    address = makeword(nes->read_byte(temp), nes->read_byte((temp + 1) & 0xFF));

    unsigned short intermediate = (address & 0xFF00) | ((address + Y) & 0xFF);
    M = nes->read_byte(intermediate);
    address += Y;
    if (address != intermediate)
    {
        required_cycles += 3;
        M = nes->read_byte(address);
    }
}

INLINE void c_cpu::indirect_y_ea()    //Post-indexed indirect
{
    unsigned char temp = nes->read_byte(PC++);
    address = makeword(nes->read_byte(temp), nes->read_byte((temp + 1) & 0xFF));

    unsigned short intermediate = (address & 0xFF00) | ((address + Y) & 0xFF);
    nes->read_byte(intermediate);
    address += Y;
}

INLINE void c_cpu::branch(int condition)
{
    signed char offset = nes->read_byte(PC++);
    if (condition)
    {
        int old_pc = PC;
        PC += offset;
        required_cycles += 3 + ((((PC ^ old_pc) & 0xFF00) != 0) * 3);
    }
}

INLINE void c_cpu::ADC()
{
    unsigned int temp = A + M + (SR.C ? 1 : 0);
    SR.C = temp > 0xFF ? true : false;
    //SR.V = (((~(A^M)) & 0x80) & ((A^temp) & 0x80)) ? true : false;
    //if A and result have different sign and M and result have different sign, set overflow
    //127 + 1 = -128 or -1 - -3 = 2 will overflow
    //127 + -128
    SR.V = (A ^ temp) & (M ^ temp) & 0x80 ? true : false;
    A = temp & 0xFF;
    setn(A);
    setz(A);
}
INLINE void c_cpu::ANC()
{
    A = A & M;
    setn(A);
    setz(A);
    SR.C = SR.N;
}
INLINE void c_cpu::AND()
{
    A = A & M;
    setn(A);
    setz(A);
}
INLINE void c_cpu::ALR()
{
    A &= nes->read_byte(PC++);
    SR.C = A & 0x1;
    SR.N = 0;
    A >>= 1;
    setz(A);
}
INLINE void c_cpu::ARR()
{
    A = A & M;
    int oldcarry = SR.C ? 1 : 0;
    SR.C = (A & 0x01) ? true : false;
    A >>= 1;
    if (oldcarry)
    {
        A |= 0x80;
    }
    setn(A);
    setz(A);
    SR.C = (A & 0x40) == 0x40;
    SR.V = ((A & 0x40) ^ ((A & 0x20) << 1)) == 0x40;
}
INLINE void c_cpu::ASL()
{
    SR.C = M & 0x80 ? true : false;
    M <<= 1;
    nes->write_byte(address, M);
    setn(M);
    setz(M);
}
INLINE void c_cpu::ASL_R(unsigned char& reg)
{
    SR.C = reg & 0x80 ? true : false;
    reg <<= 1;
    setn(reg);
    setz(reg);
}
INLINE void c_cpu::AXS()
{
    int x = A & X;
    int temp = x - M;
    SR.C = x >= M;
    X = temp & 0xFF;
    setn(X);
    setz(X);
}
INLINE void c_cpu::BCC()
{
    branch(SR.C == false);
}
INLINE void c_cpu::BCS()
{
    branch(SR.C == true);
}
INLINE void c_cpu::BEQ()
{
    branch(SR.Z == true);
}
INLINE void c_cpu::BIT()
{
    unsigned char result = A & M;
    setz(result);
    SR.V = M & 0x40 ? true : false;
    setn(M);
}
INLINE void c_cpu::BMI()
{
    branch(SR.N == true);
}
INLINE void c_cpu::BNE()
{
    branch(SR.Z == false);
}
INLINE void c_cpu::BPL()
{
    branch(SR.N == false);
}
INLINE void c_cpu::BRK()
{
    push(hibyte(PC + 1));
    push(lobyte(PC + 1));
    push(*S | 0x30);
    SR.B = true;
    SR.I = true;
    PC = makeword(nes->read_byte(0xFFFE), nes->read_byte(0xFFFF));
}
INLINE void c_cpu::BVC()
{
    branch(SR.V == false);
}
INLINE void c_cpu::BVS()
{
    branch(SR.V == true);
}
INLINE void c_cpu::CLC()
{
    SR.C = false;
}
INLINE void c_cpu::CLD()
{
    SR.D = false;
}
INLINE void c_cpu::CLI()
{
    SR.I = false;
    if (do_irq)
        irq_delay = 1;
}
INLINE void c_cpu::CLV()
{
    SR.V = false;
}
INLINE void c_cpu::CMP()
{
    unsigned char temp = A - M;
    setz(temp);
    setn(temp);
    SR.C = M <= A ? true : false;
}
INLINE void c_cpu::CPX()
{
    unsigned char temp = X - M;
    setz(temp);
    setn(temp);
    SR.C = M <= X ? true : false;
}
INLINE void c_cpu::CPY()
{
    unsigned char temp = Y - M;
    setz(temp);
    setn(temp);
    SR.C = M <= Y ? true : false;
}
INLINE void c_cpu::DCP()
{
    DEC();
    CMP();
}
INLINE void c_cpu::DEC()
{
    M--;
    nes->write_byte(address, M);
    setn(M);
    setz(M);
}
INLINE void c_cpu::DEX()
{
    X--;
    setn(X);
    setz(X);
}
INLINE void c_cpu::DEY()
{
    Y--;
    setn(Y);
    setz(Y);
}
INLINE void c_cpu::EOR()
{
    A ^= M;
    setn(A);
    setz(A);
}
INLINE void c_cpu::INC()
{
    nes->write_byte(address, M);
    M++;
    nes->write_byte(address, M);
    setn(M);
    setz(M);
}
INLINE void c_cpu::INX()
{
    X++;
    setn(X);
    setz(X);
}
INLINE void c_cpu::INY()
{
    Y++;
    setn(Y);
    setz(Y);
}
INLINE void c_cpu::ISC()
{
    INC();
    SBC();
}
INLINE void c_cpu::JMP()
{
    PC = address;
}
INLINE void c_cpu::JSR()
{
    PC--;
    push(hibyte(PC));
    push(lobyte(PC));
    PC = address;
}
INLINE void c_cpu::LAS()
{
    unsigned char temp = M & SP;
    A = X = SP = temp;
    setn(A);
    setz(A);
}
INLINE void c_cpu::LAX()
{
    LDX();
    TXA();
}
INLINE void c_cpu::LDA()
{
    A = M;
    setn(A);
    setz(A);
}
INLINE void c_cpu::LDX()
{
    X = M;
    setn(X);
    setz(X);
}
INLINE void c_cpu::LDY()
{
    Y = M;
    setn(Y);
    setz(Y);
}
INLINE void c_cpu::LSR()
{
    SR.C = M & 0x01 ? true : false;
    M >>= 1;
    setz(M);
    SR.N = false;
    nes->write_byte(address, M);
}
INLINE void c_cpu::LSR_R(unsigned char& reg)
{
    SR.C = reg & 0x01 ? true : false;
    reg >>= 1;
    setz(reg);
    SR.N = false;
}
INLINE void c_cpu::ORA()
{
    A |= M;
    setz(A);
    setn(A);
}

INLINE void c_cpu::PHA()
{
    push(A);
}
INLINE void c_cpu::PHP()
{
    push(*S | 0x30);
}
INLINE void c_cpu::PLA()
{
    A = pop();
    setn(A);
    setz(A);
}
INLINE void c_cpu::PLP()
{
    bool prev_i = SR.I;
    *S = pop();
    SR.B = false;
    SR.Unused = true;
    if (do_irq && SR.I == false && SR.I != prev_i)
        irq_delay = 1;
}
INLINE void c_cpu::RLA()
{
    ROL();
    AND();
}
INLINE void c_cpu::ROL()
{
    nes->write_byte(address, M);
    int oldcarry = SR.C ? 1 : 0;
    SR.C = (M & 0x80) ? true : false;
    M <<= 1;
    if (oldcarry)
    {
        M |= 0x01;
    }
    setn(M);
    setz(M);
    nes->write_byte(address, M);
}
INLINE void c_cpu::ROL_R(unsigned char& reg)
{
    int oldcarry = SR.C ? 1 : 0;
    SR.C = (reg & 0x80) ? true : false;
    reg <<= 1;
    if (oldcarry)
    {
        reg |= 0x01;
    }
    setn(reg);
    setz(reg);
}
INLINE void c_cpu::ROR()
{
    int oldcarry = SR.C ? 1 : 0;
    SR.C = (M & 0x01) ? true : false;
    M >>= 1;
    if (oldcarry)
    {
        M |= 0x80;
    }
    setn(M);
    setz(M);
    nes->write_byte(address, M);
}
INLINE void c_cpu::ROR_R(unsigned char& reg)
{
    int oldcarry = SR.C ? 1 : 0;
    SR.C = (reg & 0x01) ? true : false;
    reg >>= 1;
    if (oldcarry)
    {
        reg |= 0x80;
    }
    setn(reg);
    setz(reg);
}
INLINE void c_cpu::RRA()
{
    ROR();
    ADC();
}
INLINE void c_cpu::RTI()
{
    bool prev_i = SR.I;
    *S = pop();
    SR.B = false;
    unsigned char pcl = pop();
    unsigned char pch = pop();
    PC = makeword(pcl, pch);
    if (do_irq && SR.I == false && SR.I != prev_i)
        irq_delay = 1;
}
INLINE void c_cpu::RTS()
{
    unsigned char pcl = pop();
    unsigned char pch = pop();
    PC = makeword(pcl, pch);
    PC++;
}
INLINE void c_cpu::SAX()
{
    nes->write_byte(address, A & X);
}
INLINE void c_cpu::SBC()
{
    M ^= 0xFF;
    ADC();
}
INLINE void c_cpu::SEC()
{
    SR.C = true;
}
INLINE void c_cpu::SED()
{
    SR.D = true;
}
INLINE void c_cpu::SEI()
{
    SR.I = true;
}
INLINE void c_cpu::SHA()
{
    unsigned char addr_high = address >> 8;
    unsigned char addr_low = address & 0xFF;
    unsigned char temp = A & X & (addr_high + 1);
    unsigned short dest = ((unsigned short)temp << 8) | addr_low;
    nes->write_byte(dest, temp);
}
INLINE void c_cpu::SHY()
{
    unsigned char addr_high = address >> 8;
    unsigned char addr_low = address & 0xFF;
    unsigned char temp = Y & (addr_high + 1);
    unsigned short dest = ((unsigned short)temp << 8) | addr_low;
    nes->write_byte(dest, temp);
}
INLINE void c_cpu::SHX()
{
    unsigned char addr_high = address >> 8;
    unsigned char addr_low = address & 0xFF;
    unsigned char temp = X & (addr_high + 1);
    unsigned short dest = ((unsigned short)temp << 8) | addr_low;
    nes->write_byte(dest, temp);
}
INLINE void c_cpu::SKB()
{
    PC++;
}
INLINE void c_cpu::SLO()
{
    ASL();
    ORA();
}
INLINE void c_cpu::SRE()
{
    LSR();
    EOR();
}
INLINE void c_cpu::STA()
{
    nes->write_byte(address, A);
}
INLINE void c_cpu::STP()
{
    //halt
}
INLINE void c_cpu::STX()
{
    nes->write_byte(address, X);
}
INLINE void c_cpu::STY()
{
    nes->write_byte(address, Y);
}
INLINE void c_cpu::TAS()
{
    SP = A & X;
    unsigned char addr_high = address >> 8;
    unsigned char addr_low = address & 0xFF;
    unsigned char temp = A & X & (addr_high + 1);
    unsigned short dest = ((unsigned short)temp << 8) | addr_low;
    nes->write_byte(dest, temp);
}
INLINE void c_cpu::TAX()
{
    X = A;
    setn(X);
    setz(X);
}
INLINE void c_cpu::TAY()
{
    Y = A;
    setn(Y);
    setz(Y);
}
INLINE void c_cpu::TXA()
{
    A = X;
    setn(A);
    setz(A);
}
INLINE void c_cpu::TYA()
{
    A = Y;
    setn(A);
    setz(A);
}
INLINE void c_cpu::TSX()
{
    X = SP;
    setn(X);
    setz(X);
}
INLINE void c_cpu::TXS()
{
    SP = X;
}
INLINE void c_cpu::XAA()
{
    //unpredictable behavior
}

} //namespace nes