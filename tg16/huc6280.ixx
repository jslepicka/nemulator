module;
#include <cassert>
export module tg16:huc6280;
import nemulator.std;

#define INLINE __forceinline

namespace tg16
{

export template <typename Sys> class c_huc6280
{
    Sys &sys;
    // clang-format off
    static constexpr int cycle_table[260] = {
    //   0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
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
    // clang-format on
  public:
    c_huc6280(Sys &sys) : sys(sys)
    {
    }
    ~c_huc6280()
    {
    }
    int available_cycles;
    int executed_cycles;
    int odd_cycle;

    void execute_nmi()
    {
        nmi_delay = irq_checked();
        do_nmi = true;
    }

    void execute_irq()
    {
        irq_delay = irq_checked();
        do_irq = true;
    }

    void clear_irq()
    {
        do_irq = false;
        nmi_delay = 0;
    }

    void clear_nmi()
    {
        if (do_nmi) {
            int x = 1;
        }
        do_nmi = false;
    }

    void execute()
    {
        while (true) {
            if (fetch_opcode) {
                if (do_apu_dma) {
                    opcode = 0x103;
                    do_apu_dma--;
                }
                else if (dma_pos) {
                    opcode = 0x102;
                }
                else if (do_nmi && nmi_delay == 0) {
                    opcode = 0x100;
                    do_nmi = false;
                    nmi_delay = 0;
                }
                else if (do_irq && irq_delay == 0 && !SR.I) {
                    opcode = 0x101;
                    irq_delay = 0;
                }
                else {
                    if (PC == 0xE0A2) {
                        int x = 1;
                    }

                    opcode = read_byte(PC++);
                    std::printf("%4X: %2X\n", PC - 1, opcode);
                }
                irq_delay = 0;
                nmi_delay = 0;
                required_cycles += cycle_table[opcode];
                fetch_opcode = false;
            }
            if (required_cycles <= available_cycles) {
                available_cycles -= required_cycles;
                required_cycles = 0;
                fetch_opcode = true;
                execute_opcode();
            }
            else {
                return;
            }
        }
    }

    void add_cycle()
    {
        available_cycles++;
    }

    uint8_t MR[8];

    uint32_t translate_address(uint32_t address)
    {
        int bank = address >> 13;
        address &= 0x1FFF;
        uint32_t x = MR[bank] << 13;
        address |= (MR[bank] << 13);
        return address;
    }

    uint8_t read_byte(uint16_t address)
    {
        return sys.read_byte(translate_address(address));
    }

    void write_byte(uint16_t address, uint8_t value)
    {
        sys.write_byte(translate_address(address), value);
    }

    int reset()
    {
        cycles = 0;
        nmi_delay = 0;
        irq_delay = 0;
        S = (unsigned char *)&SR;
        *S = 0;
        SR.T = 0;
        SR.I = 1;
        do_nmi = false;
        do_irq = false;
        available_cycles = 0;
        fetch_opcode = true;
        opcode = 0;
        required_cycles = 0;
        irq_pending = false;
        nmi_pending = false;
        std::memset(MR, 0, sizeof(MR));
        PC = makeword(read_byte(0xFFFE), read_byte(0xFFFF));
        SP = 0xFF;
        dma_dst = 0;
        dma_src = 0;
        dma_pos = 0;
        do_apu_dma = 0;
        odd_cycle = 0;
        return 1;
    }

    void execute_apu_dma()
    {
        required_cycles += 12;
    }

    void do_sprite_dma(unsigned char *dst, int source_address)
    {
        dma_dst = dst;
        dma_src = source_address;
        dma_pos = 256;
    }

  private:
    int cycles;
    unsigned char A;                //Accumulator
    unsigned char X;                //X Index Register
    unsigned char Y;                //Y Index Register
    struct
    {
        bool C : 1;        //Carry
        bool Z : 1;        //Zero
        bool I : 1;        //Interrupt enable
        bool D : 1;        //Decimal
        bool B : 1;        //BRK
        bool T : 1;
        bool V : 1;        //Overflow
        bool N : 1;        //Sign
    } SR;
    unsigned char *S;    //Status Register
    unsigned char SP;            //Stack Pointer
    unsigned char M;                //Memory
    unsigned short PC;            //Program Counter
    unsigned short address;
    bool fetch_opcode;
    int nmi_delay;
    int irq_delay;
    int do_apu_dma;
    bool do_irq;
    bool do_nmi;
    bool irq_pending;
    bool nmi_pending;
    int dma_pos;
    int required_cycles;
    unsigned char *dma_dst;
    int dma_src;

    int opcode;

    /////////////////////////////////////////

    // clang-format off
    void execute_opcode()
    {
        switch (opcode)
        {
            case 0x00: BRK(); break;
            case 0x01: indirect_x(); ORA(); break;
            case 0x02: SXY(); break;
            case 0x03: assert(0); break;
            case 0x04: assert(0); break; //unofficial d NOP
            case 0x05: zeropage(); ORA(); break;
            case 0x06: zeropage(); ASL(); break;
            case 0x07: assert(0); break;
            case 0x08: PHP(); break;
            case 0x09: immediate(); ORA(); break;
            case 0x0A: ASL_R(A); break;
            case 0x0B: assert(0); break; //unofficial ANC
            case 0x0C: assert(0); break; //unofficial a NOP
            case 0x0D: absolute(); ORA(); break;
            case 0x0E: absolute(); ASL(); break;
            case 0x0F: assert(0); break;
            case 0x10: BPL(); break;
            case 0x11: indirect_y_pc(); ORA(); break;
            case 0x12: assert(0); break;
            case 0x13: assert(0); break;
            case 0x14: assert(0); break; //unofficial d,x NOP
            case 0x15: zeropage_x(); ORA(); break;
            case 0x16: zeropage_x(); ASL(); break;
            case 0x17: assert(0); break;
            case 0x18: CLC(); break;
            case 0x19: absolute_y_pc(); ORA(); break;
            case 0x1A: INC_A(); break; //unofficial NOP
            case 0x1B: assert(0); break;
            case 0x1C: assert(0); break; //unofficial a,x NOP
            case 0x1D: absolute_x_pc(); ORA(); break;
            case 0x1E: absolute_x_rmw(); ASL(); break;
            case 0x1F: assert(0); break;
            case 0x20: absolute(); JSR(); break;
            case 0x21: indirect_x(); AND(); break;
            case 0x22: assert(0); break;
            case 0x23: assert(0); break;
            case 0x24: zeropage(); BIT(); break;
            case 0x25: zeropage(); AND(); break;
            case 0x26: zeropage(); ROL(); break;
            case 0x27: assert(0); break;
            case 0x28: PLP(); break;
            case 0x29: immediate(); AND(); break;
            case 0x2A: ROL_R(A); break;
            case 0x2B: assert(0); ANC(); break; //unofficial ANC
            case 0x2C: absolute(); BIT(); break;
            case 0x2D: absolute(); AND(); break;
            case 0x2E: absolute(); ROL(); break;
            case 0x2F: assert(0); break;
            case 0x30: BMI(); break;
            case 0x31: indirect_y_pc(); AND(); break;
            case 0x32: assert(0); break;
            case 0x33: assert(0); break;
            case 0x34: assert(0); break; //unofficial d,x NOP
            case 0x35: zeropage_x(); AND(); break;
            case 0x36: zeropage_x(); ROL(); break;
            case 0x37: assert(0); break;
            case 0x38: SEC(); break;
            case 0x39: absolute_y_pc(); AND(); break;
            case 0x3A: assert(0); break; //unofficial NOP
            case 0x3B: assert(0); break;
            case 0x3C: assert(0); break; //unofficial a,x NOP
            case 0x3D: absolute_x_pc(); AND(); break;
            case 0x3E: absolute_x_rmw(); ROL(); break;
            case 0x3F: assert(0); break;
            case 0x40: RTI(); break;
            case 0x41: indirect_x(); EOR(); break;
            case 0x42: assert(0); break;
            case 0x43: immediate(); TMA(); break;
            case 0x44: assert(0); break; //unofficial d NOP
            case 0x45: zeropage(); EOR(); break;
            case 0x46: zeropage(); LSR(); break;
            case 0x47: assert(0); break;
            case 0x48: PHA(); break;
            case 0x49: immediate(); EOR(); break;
            case 0x4A: LSR_R(A); break;
            case 0x4B: assert(0); break;
            case 0x4C: absolute(); JMP(); break;
            case 0x4D: absolute(); EOR(); break;
            case 0x4E: absolute(); LSR(); break;
            case 0x4F: assert(0); break;
            case 0x50: BVC(); break;
            case 0x51: indirect_y_pc(); EOR(); break;
            case 0x52: assert(0); break;
            case 0x53: immediate(); TAM(); break;
            case 0x54: CSL(); break; //CSL
            case 0x55: zeropage_x(); EOR(); break;
            case 0x56: zeropage_x(); LSR(); break;
            case 0x57: assert(0); break;
            case 0x58: CLI(); break;
            case 0x59: absolute_y_pc(); EOR(); break;
            case 0x5A: PHY(); break; //unofficial NOP
            case 0x5B: assert(0); break;
            case 0x5C: assert(0); break; //unofficial a,x NOP
            case 0x5D: absolute_x_pc(); EOR(); break;
            case 0x5E: absolute_x_rmw(); LSR(); break;
            case 0x5F: assert(0); break;
            case 0x60: RTS(); break;
            case 0x61: indirect_x(); ADC(); break;
            case 0x62: CLA(); break;
            case 0x63: assert(0); break;
            case 0x64: zeropage_ea(); STZ(); break; //unofficial d NOP
            case 0x65: zeropage(); ADC(); break;
            case 0x66: zeropage(); ROR(); break;
            case 0x67: assert(0); break;
            case 0x68: PLA(); break;
            case 0x69: immediate(); ADC(); break;
            case 0x6A: ROR_R(A); break;
            case 0x6B: assert(0); break; //unofficial ARR
            case 0x6C: indirect(); JMP(); break;
            case 0x6D: absolute(); ADC(); break;
            case 0x6E: absolute(); ROR(); break;
            case 0x6F: assert(0); break;
            case 0x70: BVS(); break;
            case 0x71: indirect_y_pc(); ADC(); break;
            case 0x72: indirect2(); ADC(); break;
            case 0x73: assert(0); break;
            case 0x74: assert(0); break; //unofficial d,x NOP
            case 0x75: zeropage_x(); ADC(); break;
            case 0x76: zeropage_x(); ROR(); break;
            case 0x77: assert(0); break;
            case 0x78: SEI(); break;
            case 0x79: absolute_y_pc(); ADC(); break;
            case 0x7A: PLY(); break; //unofficial NOP
            case 0x7B: assert(0); break;
            case 0x7C: assert(0); break; //unofficial a,x NOP
            case 0x7D: absolute_x_pc(); ADC(); break;
            case 0x7E: absolute_x_rmw(); ROR(); break;
            case 0x7F: assert(0); break;
            case 0x80: BRA(); break; //branch relative
            case 0x81: indirect_x_ea(); STA(); break;
            case 0x82: CLX(); break; //unofficial i NOP
            case 0x83: assert(0); break;
            case 0x84: zeropage_ea(); STY(); break;
            case 0x85: zeropage_ea(); STA(); break;
            case 0x86: zeropage_ea(); STX(); break;
            case 0x87: assert(0); break;
            case 0x88: DEY(); break;
            case 0x89: SKB(); break;
            case 0x8A: TXA(); break;
            case 0x8B: assert(0); break;
            case 0x8C: absolute_ea(); STY(); break;
            case 0x8D: absolute_ea(); STA(); break;
            case 0x8E: absolute_ea(); STX(); break;
            case 0x8F: assert(0); break;
            case 0x90: BCC(); break;
            case 0x91: indirect_y_ea(); STA(); break;
            case 0x92: indirect2(); STA(); break;
            case 0x93: assert(0); break;
            case 0x94: zeropage_x_ea(); STY(); break;
            case 0x95: zeropage_x_ea(); STA(); break;
            case 0x96: zeropage_y_ea(); STX(); break;
            case 0x97: assert(0); break;
            case 0x98: TYA(); break;
            case 0x99: absolute_y_ea(); STA(); break;
            case 0x9A: TXS(); break;
            case 0x9B: absolute_y(); TAS(); break;
            case 0x9C: absolute_ea(); STZ(); break;
            case 0x9D: absolute_x_ea(); STA(); break;
            case 0x9E: assert(0); break;
            case 0x9F: assert(0); break;
            case 0xA0: immediate(); LDY(); break;
            case 0xA1: indirect_x(); LDA(); break;
            case 0xA2: immediate(); LDX(); break;
            case 0xA3: assert(0); break;
            case 0xA4: zeropage(); LDY(); break;
            case 0xA5: zeropage(); LDA(); break;
            case 0xA6: zeropage(); LDX(); break;
            case 0xA7: assert(0); break;
            case 0xA8: TAY(); break;
            case 0xA9: immediate(); LDA(); break;
            case 0xAA: TAX(); break;
            case 0xAB: assert(0); break;
            case 0xAC: absolute(); LDY(); break;
            case 0xAD: absolute(); LDA(); break;
            case 0xAE: absolute(); LDX(); break;
            case 0xAF: assert(0); break;
            case 0xB0: BCS(); break;
            case 0xB1: indirect_y_pc(); LDA(); break;
            case 0xB2: assert(0); break;
            case 0xB3: assert(0); break;
            case 0xB4: zeropage_x(); LDY(); break;
            case 0xB5: zeropage_x(); LDA(); break;
            case 0xB6: zeropage_y(); LDX(); break;
            case 0xB7: assert(0); break;
            case 0xB8: CLV(); break;
            case 0xB9: absolute_y_pc(); LDA(); break;
            case 0xBA: TSX(); break;
            case 0xBB: absolute_y_pc(); LAS(); break;
            case 0xBC: absolute_x_pc(); LDY(); break;
            case 0xBD: absolute_x_pc(); LDA(); break;
            case 0xBE: absolute_y_pc(); LDX(); break;
            case 0xBF: assert(0); break;
            case 0xC0: immediate(); CPY(); break;
            case 0xC1: indirect_x(); CMP(); break;
            case 0xC2: CLY(); break;
            case 0xC3: assert(0); break;
            case 0xC4: zeropage(); CPY(); break;
            case 0xC5: zeropage(); CMP(); break;
            case 0xC6: zeropage(); DEC(); break;
            case 0xC7: assert(0); break;
            case 0xC8: INY(); break;
            case 0xC9: immediate(); CMP(); break;
            case 0xCA: DEX(); break;
            case 0xCB: assert(0); break;
            case 0xCC: absolute(); CPY(); break;
            case 0xCD: absolute(); CMP(); break;
            case 0xCE: absolute(); DEC(); break;
            case 0xCF: assert(0); break;
            case 0xD0: BNE(); break;
            case 0xD1: indirect_y_pc(); CMP(); break;
            case 0xD2: assert(0); break;
            case 0xD3: assert(0); break;
            case 0xD4: CSH(); break; //unofficial d,x NOP
            case 0xD5: zeropage_x(); CMP(); break;
            case 0xD6: zeropage_x(); DEC(); break;
            case 0xD7: assert(0); break;
            case 0xD8: CLD(); break;
            case 0xD9: absolute_y_pc(); CMP(); break;
            case 0xDA: PHX(); break; //unofficial NOP
            case 0xDB: assert(0); break;
            case 0xDC: assert(0); break; //unofficial a,x NOP
            case 0xDD: absolute_x_pc(); CMP(); break;
            case 0xDE: absolute_x_rmw(); DEC(); break;
            case 0xDF: assert(0); break;
            case 0xE0: immediate(); CPX(); break;
            case 0xE1: indirect_x(); SBC(); break;
            case 0xE2: assert(0); break; //unofficial i NOP
            case 0xE3: assert(0); break;
            case 0xE4: zeropage(); CPX(); break;
            case 0xE5: zeropage(); SBC(); break;
            case 0xE6: zeropage(); INC(); break;
            case 0xE7: assert(0); break;
            case 0xE8: INX(); break;
            case 0xE9: immediate(); SBC(); break;
            case 0xEA: break;
            case 0xEB: assert(0); break;
            case 0xEC: absolute(); CPX(); break;
            case 0xED: absolute(); SBC(); break;
            case 0xEE: absolute(); INC(); break;
            case 0xEF: assert(0); break;
            case 0xF0: BEQ(); break;
            case 0xF1: indirect_y_pc(); SBC(); break;
            case 0xF2: assert(0); break;
            case 0xF3: assert(0); break;
            case 0xF4: assert(0); break; //unofficial d,x NOP
            case 0xF5: zeropage_x(); SBC(); break;
            case 0xF6: zeropage_x(); INC(); break;
            case 0xF7: assert(0); break;
            case 0xF8: SED(); break;
            case 0xF9: absolute_y_pc(); SBC(); break;
            case 0xFA: PLX(); break; //unofficial NOP
            case 0xFB: assert(0); break;
            case 0xFC: assert(0); break; //unofficial a,x NOP
            case 0xFD: absolute_x_pc(); SBC(); break;
            case 0xFE: absolute_x_rmw(); INC(); break;
            case 0xFF: assert(0); break;
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
                PC = makeword(read_byte(0xFFFA), read_byte(0xFFFB));
                break;
            case 0x101: //IRQ
                push(hibyte(PC));
                push(lobyte(PC));
                SR.B = false;
                push(*S);
                SR.I = true;
                PC = makeword(read_byte(0xFFFE), read_byte(0xFFFF));
                break;
            case 0x102: //Sprite DMA
                *(dma_dst++) = read_byte(dma_src++);
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

    // clang-format on

    void CSL()
    {
        // change speed to low
        // ignoring for now
    }

    void CSH()
    {
        // change speed to high
        // ignoring for now
    }

    void TAM()
    {
        for (int i = 0; i < 8; i++) {
            if (M & (1 << i)) {
                MR[i] = A;
            }
        }
        SR.T = 0;
        
    }

    void TMA()
    {
        SR.T = 0;
        for (int i = 0; i < 8; i++) {
            if (M & (i << i)) {
                A = MR[i];
                return;
            }
        }
    }

    void BRA()
    {
        branch(true);
    }

    void CLY()
    {
        Y = 0;
        SR.T = 0;
    }

    void CLA()
    {
        A = 0;
        SR.T = 0;
    }

    void CLX()
    {
        X = 0;
        SR.T = 0;
    }

    void SXY()
    {
        std::swap(X, Y);
        SR.T = 0;
    }

    void indirect2()
    {
        uint8_t t = read_byte(PC++);
        address = makeword(read_byte(t | 0x2000), read_byte(((t + 1) & 0xFF) | 0x2000));
        M = read_byte(address);
    }

    void PHX()
    {
        push(X);
        SR.T = 0;
    }

    void PLX()
    {
        X = pop();
        setn(X);
        setz(X);
    }

    void PHY()
    {
        push(Y);
        SR.T = 0;
    }

    void PLY()
    {
        Y = pop();
        setn(Y);
        setz(Y);
    }

    void INC_A()
    {
        A++;
        setn(A);
        setz(A);
        SR.T = 0;
    }

    void STZ()
    {
        write_byte(address, 0);
    }

    void BBS()
    {
        int bit = (opcode >> 4) - 1;
        int mask = 1 << bit;
        branch(M & mask);
    }

    INLINE void push(uint8_t value)
    {
        write_byte(SP-- | 0x2100, value);
    }

    INLINE uint8_t pop()
    {
        return read_byte(++SP | 0x2100);
    }

    INLINE void setn(uint8_t value)
    {
        SR.N = value & 0x80;
    }

    INLINE void setz(uint8_t value)
    {
        SR.Z = !value;
    }

    INLINE uint16_t makeword(uint8_t lo, uint8_t hi)
    {
        return (hi << 8) | lo;
    }

    INLINE uint8_t hibyte(uint16_t value)
    {
        return value >> 8;
    }

    INLINE uint8_t lobyte(uint16_t value)
    {
        return value & 0xFF;
    }

    int irq_checked()
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

    INLINE void immediate()
    {
        M = read_byte(PC++);
    }

    INLINE void zeropage()
    {
        address = read_byte(PC++);
        address |= 0x2000;
        M = read_byte(address);
    }

    INLINE void zeropage_ea()
    {
        address = read_byte(PC++);
        address |= 0x2000;
    }

    INLINE void zeropage_x()
    {
        address = read_byte(PC++);
        address += X;
        address &= 0xFF;
        address |= 0x2000;
        M = read_byte(address);
    }

    INLINE void zeropage_x_ea()
    {
        address = read_byte(PC++);
        address += X;
        address &= 0xFF;
        address |= 0x2000;
    }

    INLINE void zeropage_y()
    {
        address = read_byte(PC++);
        address += Y;
        address &= 0xFF;
        address |= 0x2000;
        M = read_byte(address);
    }

    INLINE void zeropage_y_ea()
    {
        address = read_byte(PC++);
        address += Y;
        address &= 0xFF;
        address |= 0x2000;
    }

    INLINE void absolute()
    {
        unsigned char lo = read_byte(PC++);
        unsigned char hi = read_byte(PC++);
        address = makeword(lo, hi);
        M = read_byte(address);
    }

    INLINE void absolute_ea()
    {
        unsigned char lo = read_byte(PC++);
        unsigned char hi = read_byte(PC++);
        address = makeword(lo, hi);
    }

    INLINE void absolute_x()
    {
        unsigned char lo = read_byte(PC++);
        unsigned char hi = read_byte(PC++);
        address = makeword(lo, hi);

        unsigned short intermediate = (address & 0xFF00) | ((address + X) & 0xFF);
        M = read_byte(intermediate);
        address += X;
        if (address != intermediate)
        {
            M = read_byte(address);
        }
    }

    INLINE void absolute_x_rmw()
    {
        unsigned char lo = read_byte(PC++);
        unsigned char hi = read_byte(PC++);
        address = makeword(lo, hi);

        unsigned short intermediate = (address & 0xFF00) | ((address + X) & 0xFF);
        M = read_byte(intermediate);
        address += X;
        M = read_byte(address);
    }

    INLINE void absolute_x_pc()
    {
        unsigned char lo = read_byte(PC++);
        unsigned char hi = read_byte(PC++);
        address = makeword(lo, hi);
        if (address == 0x2000) {
            int x = 1;
        }
        unsigned short intermediate = (address & 0xFF00) | ((address + X) & 0xFF);
        M = read_byte(intermediate);
        address += X;
        if (address != intermediate)
        {
            required_cycles += 3;
            M = read_byte(address);
        }
    }

    INLINE void absolute_x_ea()
    {
        unsigned char lo = read_byte(PC++);
        unsigned char hi = read_byte(PC++);
        address = makeword(lo, hi);

        unsigned short intermediate = (address & 0xFF00) | ((address + X) & 0xFF);
        read_byte(intermediate);
        address += X;
    }

    INLINE void absolute_y()
    {
        unsigned char lo = read_byte(PC++);
        unsigned char hi = read_byte(PC++);
        address = makeword(lo, hi);

        unsigned short intermediate = (address & 0xFF00) | ((address + Y) & 0xFF);
        M = read_byte(intermediate);
        address += Y;
        if (address != intermediate)
        {
            M = read_byte(address);
        }
    }

    INLINE void absolute_y_pc()
    {
        unsigned char lo = read_byte(PC++);
        unsigned char hi = read_byte(PC++);
        address = makeword(lo, hi);

        unsigned short intermediate = (address & 0xFF00) | ((address + Y) & 0xFF);
        M = read_byte(intermediate);
        address += Y;
        if (address != intermediate)
        {
            required_cycles += 3;
            M = read_byte(address);
        }
    }

    INLINE void absolute_y_ea()
    {
        unsigned char lo = read_byte(PC++);
        unsigned char hi = read_byte(PC++);
        address = makeword(lo, hi);

        unsigned short intermediate = (address & 0xFF00) | ((address + Y) & 0xFF);
        read_byte(intermediate);
        address += Y;
    }

    INLINE void indirect()    //For indirect jmp
    {
        unsigned char lo = read_byte(PC++);
        unsigned char hi = read_byte(PC++);
        unsigned short tempaddress = makeword(lo, hi);
        unsigned char pcl = read_byte(tempaddress);
        lo++;
        tempaddress = makeword(lo, hi);
        unsigned char pch = read_byte(tempaddress);
        address = makeword(pcl, pch);
    }

    INLINE void indirect_x()    //Pre-indexed indirect
    {
        //TODO: I think this is ok, but need to verify
        unsigned char hi = read_byte(PC++);
        hi += X;
        address = makeword(read_byte(hi), read_byte((hi + 1) & 0xFF));
        M = read_byte(address);
    }

    INLINE void indirect_x_ea()    //Pre-indexed indirect
    {
        //TODO: I think this is ok, but need to verify
        unsigned char hi = read_byte(PC++);
        hi += X;
        address = makeword(read_byte(hi), read_byte((hi + 1) & 0xFF));
    }

    INLINE void indirect_y()    //Post-indexed indirect
    {
        unsigned char temp = read_byte(PC++);
        address = makeword(read_byte(temp | 0x2000), read_byte(((temp + 1) & 0xFF) | 0x2000));

        unsigned short intermediate = (address & 0xFF00) | ((address + Y) & 0xFF);
        M = read_byte(intermediate);
        address += Y;
        if (address != intermediate)
        {
            M = read_byte(address);
        }
    }

    INLINE void indirect_y_pc()    //Post-indexed indirect
    {
        unsigned char temp = read_byte(PC++);
        address = makeword(read_byte(temp | 0x2000), read_byte(((temp + 1) & 0xFF) | 0x2000));

        unsigned short intermediate = (address & 0xFF00) | ((address + Y) & 0xFF);
        M = read_byte(intermediate);
        address += Y;
        if (address != intermediate)
        {
            required_cycles += 3;
            M = read_byte(address);
        }
    }

    INLINE void indirect_y_ea()    //Post-indexed indirect
    {
        unsigned char temp = read_byte(PC++);
        address = makeword(read_byte(temp | 0x2000), read_byte(((temp + 1) & 0xFF) | 0x2000));

        unsigned short intermediate = (address & 0xFF00) | ((address + Y) & 0xFF);
        read_byte(intermediate);
        address += Y;
    }

    INLINE void branch(int condition)
    {
        signed char offset = read_byte(PC++);
        if (condition)
        {
            int old_pc = PC;
            PC += offset;
            required_cycles += 3 + ((((PC ^ old_pc) & 0xFF00) != 0) * 3);
        }
    }

    INLINE void ADC()
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
    INLINE void ANC()
    {
        A = A & M;
        setn(A);
        setz(A);
        SR.C = SR.N;
    }
    INLINE void AND()
    {
        A = A & M;
        setn(A);
        setz(A);
    }
    INLINE void ALR()
    {
        A &= read_byte(PC++);
        SR.C = A & 0x1;
        SR.N = 0;
        A >>= 1;
        setz(A);
    }
    INLINE void ARR()
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
    INLINE void ASL()
    {
        SR.C = M & 0x80 ? true : false;
        M <<= 1;
        write_byte(address, M);
        setn(M);
        setz(M);
    }
    INLINE void ASL_R(unsigned char& reg)
    {
        SR.C = reg & 0x80 ? true : false;
        reg <<= 1;
        setn(reg);
        setz(reg);
    }
    INLINE void AXS()
    {
        int x = A & X;
        int temp = x - M;
        SR.C = x >= M;
        X = temp & 0xFF;
        setn(X);
        setz(X);
    }
    INLINE void BCC()
    {
        branch(SR.C == false);
    }
    INLINE void BCS()
    {
        branch(SR.C == true);
    }
    INLINE void BEQ()
    {
        branch(SR.Z == true);
    }
    INLINE void BIT()
    {
        unsigned char result = A & M;
        setz(result);
        SR.V = M & 0x40 ? true : false;
        setn(M);
    }
    INLINE void BMI()
    {
        branch(SR.N == true);
    }
    INLINE void BNE()
    {
        branch(SR.Z == false);
    }
    INLINE void BPL()
    {
        branch(SR.N == false);
    }
    INLINE void BRK()
    {
        push(hibyte(PC + 1));
        push(lobyte(PC + 1));
        push(*S | 0x30);
        SR.B = true;
        SR.I = true;
        PC = makeword(read_byte(0xFFFE), read_byte(0xFFFF));
    }
    INLINE void BVC()
    {
        branch(SR.V == false);
    }
    INLINE void BVS()
    {
        branch(SR.V == true);
    }
    INLINE void CLC()
    {
        SR.C = false;
    }
    INLINE void CLD()
    {
        SR.D = false;
    }
    INLINE void CLI()
    {
        SR.I = false;
        if (do_irq)
            irq_delay = 1;
    }
    INLINE void CLV()
    {
        SR.V = false;
    }
    INLINE void CMP()
    {
        unsigned char temp = A - M;
        setz(temp);
        setn(temp);
        SR.C = M <= A ? true : false;
    }
    INLINE void CPX()
    {
        unsigned char temp = X - M;
        setz(temp);
        setn(temp);
        SR.C = M <= X ? true : false;
    }
    INLINE void CPY()
    {
        unsigned char temp = Y - M;
        setz(temp);
        setn(temp);
        SR.C = M <= Y ? true : false;
    }
    INLINE void DCP()
    {
        DEC();
        CMP();
    }
    INLINE void DEC()
    {
        M--;
        write_byte(address, M);
        setn(M);
        setz(M);
    }
    INLINE void DEX()
    {
        X--;
        setn(X);
        setz(X);
    }
    INLINE void DEY()
    {
        Y--;
        setn(Y);
        setz(Y);
    }
    INLINE void EOR()
    {
        A ^= M;
        setn(A);
        setz(A);
    }
    INLINE void INC()
    {
        write_byte(address, M);
        M++;
        write_byte(address, M);
        setn(M);
        setz(M);
    }
    INLINE void INX()
    {
        X++;
        setn(X);
        setz(X);
    }
    INLINE void INY()
    {
        Y++;
        setn(Y);
        setz(Y);
    }
    INLINE void ISC()
    {
        INC();
        SBC();
    }
    INLINE void JMP()
    {
        PC = address;
    }
    INLINE void JSR()
    {
        PC--;
        push(hibyte(PC));
        push(lobyte(PC));
        PC = address;
    }
    INLINE void LAS()
    {
        unsigned char temp = M & SP;
        A = X = SP = temp;
        setn(A);
        setz(A);
    }
    INLINE void LAX()
    {
        LDX();
        TXA();
    }
    INLINE void LDA()
    {
        A = M;
        setn(A);
        setz(A);
    }
    INLINE void LDX()
    {
        X = M;
        setn(X);
        setz(X);
    }
    INLINE void LDY()
    {
        Y = M;
        setn(Y);
        setz(Y);
    }
    INLINE void LSR()
    {
        SR.C = M & 0x01 ? true : false;
        M >>= 1;
        setz(M);
        SR.N = false;
        write_byte(address, M);
    }
    INLINE void LSR_R(unsigned char& reg)
    {
        SR.C = reg & 0x01 ? true : false;
        reg >>= 1;
        setz(reg);
        SR.N = false;
    }
    INLINE void ORA()
    {
        A |= M;
        setz(A);
        setn(A);
    }

    INLINE void PHA()
    {
        push(A);
    }
    INLINE void PHP()
    {
        push(*S);
        SR.T = 0;
    }
    INLINE void PLA()
    {
        A = pop();
        setn(A);
        setz(A);
    }
    INLINE void PLP()
    {
        bool prev_i = SR.I;
        *S = pop();
        if (do_irq && SR.I == false && SR.I != prev_i)
            irq_delay = 1;
    }
    INLINE void RLA()
    {
        ROL();
        AND();
    }
    INLINE void ROL()
    {
        write_byte(address, M);
        int oldcarry = SR.C ? 1 : 0;
        SR.C = (M & 0x80) ? true : false;
        M <<= 1;
        if (oldcarry)
        {
            M |= 0x01;
        }
        setn(M);
        setz(M);
        write_byte(address, M);
    }
    INLINE void ROL_R(unsigned char& reg)
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
    INLINE void ROR()
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
        write_byte(address, M);
    }
    INLINE void ROR_R(unsigned char& reg)
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
    INLINE void RRA()
    {
        ROR();
        ADC();
    }
    INLINE void RTI()
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
    INLINE void RTS()
    {
        unsigned char pcl = pop();
        unsigned char pch = pop();
        PC = makeword(pcl, pch);
        PC++;
    }
    INLINE void SAX()
    {
        write_byte(address, A & X);
    }
    INLINE void SBC()
    {
        M ^= 0xFF;
        ADC();
    }
    INLINE void SEC()
    {
        SR.C = true;
    }
    INLINE void SED()
    {
        SR.D = true;
    }
    INLINE void SEI()
    {
        SR.I = true;
    }
    INLINE void SHA()
    {
        unsigned char addr_high = address >> 8;
        unsigned char addr_low = address & 0xFF;
        unsigned char temp = A & X & (addr_high + 1);
        unsigned short dest = ((unsigned short)temp << 8) | addr_low;
        write_byte(dest, temp);
    }
    INLINE void SHY()
    {
        unsigned char addr_high = address >> 8;
        unsigned char addr_low = address & 0xFF;
        unsigned char temp = Y & (addr_high + 1);
        unsigned short dest = ((unsigned short)temp << 8) | addr_low;
        write_byte(dest, temp);
    }
    INLINE void SHX()
    {
        unsigned char addr_high = address >> 8;
        unsigned char addr_low = address & 0xFF;
        unsigned char temp = X & (addr_high + 1);
        unsigned short dest = ((unsigned short)temp << 8) | addr_low;
        write_byte(dest, temp);
    }
    INLINE void SKB()
    {
        PC++;
    }
    INLINE void SLO()
    {
        ASL();
        ORA();
    }
    INLINE void SRE()
    {
        LSR();
        EOR();
    }
    INLINE void STA()
    {
        write_byte(address, A);
    }
    INLINE void STP()
    {
        //halt
    }
    INLINE void STX()
    {
        write_byte(address, X);
    }
    INLINE void STY()
    {
        write_byte(address, Y);
    }
    INLINE void TAS()
    {
        SP = A & X;
        unsigned char addr_high = address >> 8;
        unsigned char addr_low = address & 0xFF;
        unsigned char temp = A & X & (addr_high + 1);
        unsigned short dest = ((unsigned short)temp << 8) | addr_low;
        write_byte(dest, temp);
    }
    INLINE void TAX()
    {
        X = A;
        setn(X);
        setz(X);
    }
    INLINE void TAY()
    {
        Y = A;
        setn(Y);
        setz(Y);
    }
    INLINE void TXA()
    {
        A = X;
        setn(A);
        setz(A);
    }
    INLINE void TYA()
    {
        A = Y;
        setn(A);
        setz(A);
    }
    INLINE void TSX()
    {
        X = SP;
        setn(X);
        setz(X);
    }
    INLINE void TXS()
    {
        SP = X;
    }
    INLINE void XAA()
    {
        //unpredictable behavior
    }

};


}