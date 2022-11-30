#include "cpu.h"
#include "mapper.h"
#include "nes.h"
#include "ppu.h"

#if defined(DEBUG) | defined(_DEBUG)
#define DEBUG_NEW new(_CLIENT_BLOCK, __FILE__, __LINE__)
#define new DEBUG_NEW
#endif

#define INLINE __forceinline
//#define INLINE
#define PUSH(x) nes->write_byte(SP-- | 0x100, x)
#define POP nes->read_byte(++SP | 0x100)
#define SETN(x) SR.N = (x & 0x80) == 0x80 ? true : false
#define SETZ(x) SR.Z = x == 0 ? true : false
#define CHECK_PAGE_CROSS2(a, b) required_cycles += (((a^b)&0xFF00) != 0) * 3

//replaced all zeros with 3 so that invalid opcodes don't cause cpu loop to spin
const int c_cpu::cycle_table[] = {
		21, 18,  3,  3,  3,  9, 15,  3,  9,  6,  6,  3,  3, 12, 18,  3, //0F
		 6, 15,  3,  3,  3, 12, 18,  3,  6, 12,  3,  3,  3, 12, 21,  3, //1F
		18, 18,  3,  3,  9,  9, 15,  3, 12,  6,  6,  3, 12, 12, 18,  3, //2F
		 6, 15,  3,  3,  3, 12, 18,  3,  6, 12,  3,  3,  3, 12, 21,  3, //3F
		18, 18,  3,  3,  3,  9, 15,  3,  9,  6,  6,  6,  9, 12, 18,  3, //4F
		 6, 15,  3,  3,  3, 12, 18,  3,  6, 12,  3,  3,  3, 12, 21,  3, //5F
		18, 18,  3,  3,  3,  9, 15,  3, 12,  6,  6,  3, 15, 12, 18,  3, //6F
		 6, 15,  3,  3,  3, 12, 18,  3,  6, 12,  3,  3,  3, 12, 21,  3, //7F
		 3, 18,  3,  3,  9,  9,  9,  3,  6,  6,  6,  3, 12, 12, 12, 12, //8F
		 6, 18,  3,  3, 12, 12, 12,  3,  6, 15,  6,  3,  3, 15,  3,  3, //9F
		 6, 18,  6,  3,  9,  9,  9,  9,  6,  6,  6,  3, 12, 12, 12,  3, //AF
		 6, 15,  3, 15, 12, 12, 12,  3,  6, 12,  6,  3, 12, 12, 12,  3, //BF
		 6, 18,  3,  3,  9,  9, 15,  3,  6,  6,  6,  6, 12, 12, 18,  3, //CF
		 6, 15,  3,  3,  3, 12, 18,  3,  6, 12,  3, 21,  3, 12, 21,  3, //DF
		 6, 18,  3,  3,  9,  9, 15,  3,  6,  6,  6,  3, 12, 12, 18,  3, //EF
		 6, 15,  3,  3,  3, 12, 18,  3,  6, 12,  3, 21,  3, 12, 21,  3, //FF
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
	PC = MAKEWORD(nes->read_byte(0xFFFC), nes->read_byte(0xFFFD));
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
	for (;;)
	{
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
		else
			break;
	}
}

void c_cpu::execute3()
{
	available_cycles += 3;
	execute();
}

void c_cpu::execute_cycles(int numPpuCycles)
{
	available_cycles += numPpuCycles;
	execute();
}

void c_cpu::do_sprite_dma(unsigned char* dst, int source_address)
{
	dma_dst = dst;
	dma_src = source_address;
	dma_pos = 256;
}

void c_cpu::execute_opcode()
{
	int sl = nes->ppu->current_scanline;
	int c = nes->ppu->current_cycle;
	switch (opcode)
	{
	case 0x00: BRK(); break;
	case 0x01: indirect_x(); ORA(); break;
	case 0x05: zeropage(); ORA(); break;
	case 0x06: zeropage(); ASL(); break;
	case 0x08: PHP(); break;
	case 0x09: immediate(); ORA(); break;
	case 0x0A: ASL_R(A); break;
	case 0x0D: absolute(); ORA(); break;
	case 0x0E: absolute(); ASL(); break;
	case 0x10: BPL(); break;
	case 0x11: indirect_y_pc(); ORA(); break;
	case 0x15: zeropage_x(); ORA(); break;
	case 0x16: zeropage_x(); ASL(); break;
	case 0x18: CLC(); break;
	case 0x19: absolute_y_pc(); ORA(); break;
	case 0x1D: absolute_x_pc(); ORA(); break;
	case 0x1E: absolute_x(); ASL(); break;
	case 0x20: absolute(); JSR(); break;
	case 0x21: indirect_x(); AND(); break;
	case 0x24: zeropage(); BIT(); break;
	case 0x25: zeropage(); AND(); break;
	case 0x26: zeropage(); ROL(); break;
	case 0x28: PLP(); break;
	case 0x29: immediate(); AND(); break;
	case 0x2A: ROL_R(A); break;
	case 0x2C: absolute(); BIT(); break;
	case 0x2D: absolute(); AND(); break;
	case 0x2E: absolute(); ROL(); break;
	case 0x30: BMI(); break;
	case 0x31: indirect_y_pc(); AND(); break;
	case 0x35: zeropage_x(); AND(); break;
	case 0x36: zeropage_x(); ROL(); break;
	case 0x38: SEC(); break;
	case 0x39: absolute_y_pc(); AND(); break;
	case 0x3D: absolute_x_pc(); AND(); break;
	case 0x3E: absolute_x(); ROL(); break;
	case 0x40: RTI(); break;
	case 0x41: indirect_x(); EOR(); break;
	case 0x45: zeropage(); EOR(); break;
	case 0x46: zeropage(); LSR(); break;
	case 0x48: PHA(); break;
	case 0x49: immediate(); EOR(); break;
	case 0x4A: LSR_R(A); break;
	case 0x4B: ALR(); break;
	case 0x4C: absolute(); JMP(); break;
	case 0x4D: absolute(); EOR(); break;
	case 0x4E: absolute(); LSR(); break;
	case 0x50: BVC(); break;
	case 0x51: indirect_y_pc(); EOR(); break;
	case 0x55: zeropage_x(); EOR(); break;
	case 0x56: zeropage_x(); LSR(); break;
	case 0x58: CLI(); break;
	case 0x59: absolute_y_pc(); EOR(); break;
	case 0x5D: absolute_x_pc(); EOR(); break;
	case 0x5E: absolute_x(); LSR(); break;
	case 0x60: RTS(); break;
	case 0x61: indirect_x(); ADC(); break;
	case 0x65: zeropage(); ADC(); break;
	case 0x66: zeropage(); ROR(); break;
	case 0x68: PLA(); break;
	case 0x69: immediate(); ADC(); break;
	case 0x6A: ROR_R(A); break;
	case 0x6C: indirect(); JMP(); break;
	case 0x6D: absolute(); ADC(); break;
	case 0x6E: absolute(); ROR(); break;
	case 0x70: BVS(); break;
	case 0x71: indirect_y_pc(); ADC(); break;
	case 0x75: zeropage_x(); ADC(); break;
	case 0x76: zeropage_x(); ROR(); break;
	case 0x78: SEI(); break;
	case 0x79: absolute_y_pc(); ADC(); break;
	case 0x7D: absolute_x_pc(); ADC(); break;
	case 0x7E: absolute_x(); ROR(); break;
	case 0x81: indirect_x_ea(); STA(); break;
	case 0x84: zeropage_ea(); STY(); break;
	case 0x85: zeropage_ea(); STA(); break;
	case 0x86: zeropage_ea(); STX(); break;
	case 0x88: DEY(); break;
	case 0x89: SKB(); break;
	case 0x8A: TXA(); break;
	case 0x8C: absolute_ea(); STY(); break;
	case 0x8D: absolute_ea(); STA(); break;
	case 0x8E: absolute_ea(); STX(); break;
	case 0x8F: absolute_ea(); SAX(); break;
	case 0x90: BCC(); break;
	case 0x91: indirect_y_ea(); STA(); break;
	case 0x94: zeropage_x_ea(); STY(); break;
	case 0x95: zeropage_x_ea(); STA(); break;
	case 0x96: zeropage_y_ea(); STX(); break;
	case 0x98: TYA(); break;
	case 0x99: absolute_y_ea(); STA(); break;
	case 0x9A: TXS(); break;
	case 0x9D: absolute_x_ea(); STA(); break;
	case 0xA0: immediate(); LDY(); break;
	case 0xA1: indirect_x(); LDA(); break;
	case 0xA2: immediate(); LDX(); break;
	case 0xA4: zeropage(); LDY(); break;
	case 0xA5: zeropage(); LDA(); break;
	case 0xA6: zeropage(); LDX(); break;
	case 0xA7: zeropage(); LAX(); break;
	case 0xA8: TAY(); break;
	case 0xA9: immediate(); LDA(); break;
	case 0xAA: TAX(); break;
	case 0xAC: absolute(); LDY(); break;
	case 0xAD: absolute(); LDA(); break;
	case 0xAE: absolute(); LDX(); break;
	case 0xB0: BCS(); break;
	case 0xB1: indirect_y_pc(); LDA(); break;
	case 0xB3: indirect_y_pc(); LAX(); break;
	case 0xB4: zeropage_x(); LDY(); break;
	case 0xB5: zeropage_x(); LDA(); break;
	case 0xB6: zeropage_y(); LDX(); break;
	case 0xB8: CLV(); break;
	case 0xB9: absolute_y_pc(); LDA(); break;
	case 0xBA: TSX(); break;
	case 0xBC: absolute_x_pc(); LDY(); break;
	case 0xBD: absolute_x_pc(); LDA(); break;
	case 0xBE: absolute_y_pc(); LDX(); break;
	case 0xC0: immediate(); CPY(); break;
	case 0xC1: indirect_x(); CMP(); break;
	case 0xC4: zeropage(); CPY(); break;
	case 0xC5: zeropage(); CMP(); break;
	case 0xC6: zeropage(); DEC(); break;
	case 0xC8: INY(); break;
	case 0xC9: immediate(); CMP(); break;
	case 0xCA: DEX(); break;
	case 0xCB: AXS(); break;
	case 0xCC: absolute(); CPY(); break;
	case 0xCD: absolute(); CMP(); break;
	case 0xCE: absolute(); DEC(); break;
	case 0xD0: BNE(); break;
	case 0xD1: indirect_y_pc(); CMP(); break;
	case 0xD5: zeropage_x(); CMP(); break;
	case 0xD6: zeropage_x(); DEC(); break;
	case 0xD8: CLD(); break;
	case 0xD9: absolute_y_pc(); CMP(); break;
	case 0xDB: absolute_y(); DCP(); break;
	case 0xDD: absolute_x_pc(); CMP(); break;
	case 0xDE: absolute_x(); DEC(); break;
	case 0xE0: immediate(); CPX(); break;
	case 0xE1: indirect_x(); SBC(); break;
	case 0xE4: zeropage(); CPX(); break;
	case 0xE5: zeropage(); SBC(); break;
	case 0xE6: zeropage(); INC(); break;
	case 0xE8: INX(); break;
	case 0xE9: immediate(); SBC(); break;
	case 0xEA: break;
	case 0xEC: absolute(); CPX(); break;
	case 0xED: absolute(); SBC(); break;
	case 0xEE: absolute(); INC(); break;
	case 0xF0: BEQ(); break;
	case 0xF1: indirect_y_pc(); SBC(); break;
	case 0xF5: zeropage_x(); SBC(); break;
	case 0xF6: zeropage_x(); INC(); break;
	case 0xF8: SED(); break;
	case 0xF9: absolute_y_pc(); SBC(); break;
	case 0xFB: absolute_y(); ISC(); break;
	case 0xFD: absolute_x_pc(); SBC(); break;
	case 0xFE: absolute_x(); INC(); break;
	case 0x100: //NMI
		//Battletoads debugging
	//{
	//	char x[256];
	//	sprintf(x, "NMI at scanline %d, cycle %d\n", sl, c);
	//	OutputDebugString(x);
	//}
		PUSH(HIBYTE(PC));
		PUSH(LOBYTE(PC));
		SR.B = false;
		PUSH(*S);
		SR.I = true;
		PC = MAKEWORD(nes->read_byte(0xFFFA), nes->read_byte(0xFFFB));
		break;
	case 0x101: //IRQ
		PUSH(HIBYTE(PC));
		PUSH(LOBYTE(PC));
		SR.B = false;
		PUSH(*S);
		SR.I = true;
		PC = MAKEWORD(nes->read_byte(0xFFFE), nes->read_byte(0xFFFF));
		break;
	case 0x102: //Sprite DMA
	{
		*(dma_dst++) = nes->read_byte(dma_src++);
		dma_pos--;

		//Sprite DMA burns 513 CPU cycles, so after the last DMA, burn another one.
		if (dma_pos == 0) {
			required_cycles += 3;
			if (odd_cycle) {
				required_cycles += 3;
			}
		}
	}
	break;
	case 0x103: //APU DMA
	{int a = 1; }
	break;
	default:
	{
		int a = 1;
		//char buf[8];
		//sprintf(buf, "%x", opcode); 
		//MessageBox(NULL, buf, "illegal opcode", MB_OK);
	}
	break;
	}
}

void c_cpu::execute_apu_dma()
{
	//doApuDMA++;
	//if (doApuDMA > 1)
	//	int x = 1;
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
	int sl = nes->ppu->current_scanline;
	int c = nes->ppu->current_cycle;
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
	address = MAKEWORD(lo, hi);
	M = nes->read_byte(address);
}

INLINE void c_cpu::absolute_ea()
{
	unsigned char lo = nes->read_byte(PC++);
	unsigned char hi = nes->read_byte(PC++);
	address = MAKEWORD(lo, hi);
}

INLINE void c_cpu::absolute_x()
{
	unsigned char lo = nes->read_byte(PC++);
	unsigned char hi = nes->read_byte(PC++);
	address = MAKEWORD(lo, hi);
	int temp = address + X;
	address = temp;
	M = nes->read_byte(address);
}

INLINE void c_cpu::absolute_x_pc()
{
	unsigned char lo = nes->read_byte(PC++);
	unsigned char hi = nes->read_byte(PC++);
	address = MAKEWORD(lo, hi);
	int temp = address + X;
	CHECK_PAGE_CROSS2(address, temp);
	address = temp;
	M = nes->read_byte(address);
}

INLINE void c_cpu::absolute_x_ea()
{
	unsigned char lo = nes->read_byte(PC++);
	unsigned char hi = nes->read_byte(PC++);
	address = MAKEWORD(lo, hi);
	int temp = address + X;
	address = temp;
	nes->read_byte(address);
}

INLINE void c_cpu::absolute_y()
{
	unsigned char lo = nes->read_byte(PC++);
	unsigned char hi = nes->read_byte(PC++);
	address = MAKEWORD(lo, hi);
	int temp = address + Y;
	address = temp;
	M = nes->read_byte(address);
}

INLINE void c_cpu::absolute_y_pc()
{
	unsigned char lo = nes->read_byte(PC++);
	unsigned char hi = nes->read_byte(PC++);
	address = MAKEWORD(lo, hi);
	int temp = address + Y;
	CHECK_PAGE_CROSS2(address, temp);
	address = temp;
	M = nes->read_byte(address);
}

INLINE void c_cpu::absolute_y_ea()
{
	unsigned char lo = nes->read_byte(PC++);
	unsigned char hi = nes->read_byte(PC++);
	address = MAKEWORD(lo, hi);
	int temp = address + Y;
	address = temp;
	nes->read_byte(address);
}

INLINE void c_cpu::indirect()	//For indirect jmp
{
	unsigned char lo = nes->read_byte(PC++);
	unsigned char hi = nes->read_byte(PC++);
	unsigned short tempaddress = MAKEWORD(lo, hi);
	unsigned char pcl = nes->read_byte(tempaddress);
	lo++;
	tempaddress = MAKEWORD(lo, hi);
	unsigned char pch = nes->read_byte(tempaddress);
	address = MAKEWORD(pcl, pch);
}

INLINE void c_cpu::indirect_x()	//Pre-indexed indirect
{
	//TODO: I think this is ok, but need to verify
	unsigned char hi = nes->read_byte(PC++);
	hi += X;
	address = MAKEWORD(nes->read_byte(hi), nes->read_byte((hi + 1) & 0xFF));
	M = nes->read_byte(address);
}

INLINE void c_cpu::indirect_x_ea()	//Pre-indexed indirect
{
	//TODO: I think this is ok, but need to verify
	unsigned char hi = nes->read_byte(PC++);
	hi += X;
	address = MAKEWORD(nes->read_byte(hi), nes->read_byte((hi + 1) & 0xFF));
}

INLINE void c_cpu::indirect_y()	//Post-indexed indirect
{
	unsigned char temp = nes->read_byte(PC++);
	address = MAKEWORD(nes->read_byte(temp), nes->read_byte((temp + 1) & 0xFF));

	int temp2 = address + Y;
	address = temp2;
	M = nes->read_byte(address);
}

INLINE void c_cpu::indirect_y_pc()	//Post-indexed indirect
{
	unsigned char temp = nes->read_byte(PC++);
	address = MAKEWORD(nes->read_byte(temp), nes->read_byte((temp + 1) & 0xFF));

	int temp2 = address + Y;
	CHECK_PAGE_CROSS2(address, temp2);
	address = temp2;
	M = nes->read_byte(address);
}

INLINE void c_cpu::indirect_y_ea()	//Post-indexed indirect
{
	unsigned char temp = nes->read_byte(PC++);
	address = MAKEWORD(nes->read_byte(temp), nes->read_byte((temp + 1) & 0xFF));

	int temp2 = address + Y;
	address = temp2;
	nes->read_byte(address);
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
	SETN(A);
	SETZ(A);
}
INLINE void c_cpu::AND()
{
	A = A & M;
	SETN(A);
	SETZ(A);
}
INLINE void c_cpu::ALR()
{
	A &= nes->read_byte(PC++);
	SR.C = A & 0x1;
	SR.N = 0;
	A >>= 1;
	SETZ(A);
}
INLINE void c_cpu::ASL()
{
	SR.C = M & 0x80 ? true : false;
	M <<= 1;
	nes->write_byte(address, M);
	SETN(M);
	SETZ(M);
}
INLINE void c_cpu::ASL_R(unsigned char& reg)
{
	SR.C = reg & 0x80 ? true : false;
	reg <<= 1;
	SETN(reg);
	SETZ(reg);
}
INLINE void c_cpu::AXS()
{
	X &= A;
	unsigned short temp = X - nes->read_byte(PC++);
	if (temp > 0xFF)
		SR.C = true;
	X = temp & 0xFF;
	SETN(X);
	SETZ(X);

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
	SETZ(result);
	SR.V = M & 0x40 ? true : false;
	SETN(M);
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
	PUSH(HIBYTE(PC + 1));
	PUSH(LOBYTE(PC + 1));
	PUSH(*S | 0x30);
	SR.B = true;
	SR.I = true;
	PC = MAKEWORD(nes->read_byte(0xFFFE), nes->read_byte(0xFFFF));
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
	SETZ(temp);
	SETN(temp);
	SR.C = M <= A ? true : false;
}
INLINE void c_cpu::CPX()
{
	unsigned char temp = X - M;
	SETZ(temp);
	SETN(temp);
	SR.C = M <= X ? true : false;
}
INLINE void c_cpu::CPY()
{
	unsigned char temp = Y - M;
	SETZ(temp);
	SETN(temp);
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
	SETN(M);
	SETZ(M);
}
INLINE void c_cpu::DEX()
{
	X--;
	SETN(X);
	SETZ(X);
}
INLINE void c_cpu::DEY()
{
	Y--;
	SETN(Y);
	SETZ(Y);
}
INLINE void c_cpu::EOR()
{
	A ^= M;
	SETN(A);
	SETZ(A);
}
INLINE void c_cpu::INC()
{
	nes->write_byte(address, M);
	M++;
	nes->write_byte(address, M);
	SETN(M);
	SETZ(M);
}
INLINE void c_cpu::INX()
{
	X++;
	SETN(X);
	SETZ(X);
}
INLINE void c_cpu::INY()
{
	Y++;
	SETN(Y);
	SETZ(Y);
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
	PUSH(HIBYTE(PC));
	PUSH(LOBYTE(PC));
	PC = address;
}
INLINE void c_cpu::LAX()
{
	LDX();
	TXA();
}
INLINE void c_cpu::LDA()
{
	A = M;
	SETN(A);
	SETZ(A);
}
INLINE void c_cpu::LDX()
{
	X = M;
	SETN(X);
	SETZ(X);
}
INLINE void c_cpu::LDY()
{
	Y = M;
	SETN(Y);
	SETZ(Y);
}
INLINE void c_cpu::LSR()
{
	SR.C = M & 0x01 ? true : false;
	M >>= 1;
	SETZ(M);
	SR.N = false;
	nes->write_byte(address, M);
}
INLINE void c_cpu::LSR_R(unsigned char& reg)
{
	SR.C = reg & 0x01 ? true : false;
	reg >>= 1;
	SETZ(reg);
	SR.N = false;
}
INLINE void c_cpu::ORA()
{
	A |= M;
	SETZ(A);
	SETN(A);
}

INLINE void c_cpu::PHA()
{
	PUSH(A);
}
INLINE void c_cpu::PHP()
{
	PUSH(*S | 0x30);
}
INLINE void c_cpu::PLA()
{
	A = POP;
	SETN(A);
	SETZ(A);
}
INLINE void c_cpu::PLP()
{
	bool prev_i = SR.I;
	*S = POP;
	SR.B = false;
	SR.Unused = true;
	if (do_irq && SR.I == false && SR.I != prev_i)
		irq_delay = 1;
}
INLINE void c_cpu::ROL()
{
	int oldcarry = SR.C ? 1 : 0;
	SR.C = (M & 0x80) ? true : false;
	M <<= 1;
	if (oldcarry)
	{
		M |= 0x01;
	}
	SETN(M);
	SETZ(M);
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
	SETN(reg);
	SETZ(reg);
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
	SETN(M);
	SETZ(M);
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
	SETN(reg);
	SETZ(reg);
}
INLINE void c_cpu::RTI()
{
	bool prev_i = SR.I;
	*S = POP;
	SR.B = false;
	unsigned char pcl = POP;
	unsigned char pch = POP;
	PC = MAKEWORD(pcl, pch);
	if (do_irq && SR.I == false && SR.I != prev_i)
		irq_delay = 1;
}
INLINE void c_cpu::RTS()
{
	unsigned char pcl = POP;
	unsigned char pch = POP;
	PC = MAKEWORD(pcl, pch);
	PC++;
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

INLINE void c_cpu::SKB()
{
	PC++;
}
INLINE void c_cpu::SAX()
{
	nes->write_byte(address, A & X);
}
INLINE void c_cpu::STA()
{
	nes->write_byte(address, A);
}
INLINE void c_cpu::STX()
{
	nes->write_byte(address, X);
}
INLINE void c_cpu::STY()
{
	nes->write_byte(address, Y);
}
INLINE void c_cpu::TAX()
{
	X = A;
	SETN(X);
	SETZ(X);
}
INLINE void c_cpu::TAY()
{
	Y = A;
	SETN(Y);
	SETZ(Y);
}
INLINE void c_cpu::TXA()
{
	A = X;
	SETN(A);
	SETZ(A);
}
INLINE void c_cpu::TYA()
{
	A = Y;
	SETN(A);
	SETZ(A);
}
INLINE void c_cpu::TSX()
{
	X = SP;
	SETN(X);
	SETZ(X);
}
INLINE void c_cpu::TXS()
{
	SP = X;
}
