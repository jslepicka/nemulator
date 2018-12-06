///////////////////////////////////////////////////////////////////////////////////
//                                                                               //
//   nemulator (an NES emulator)                                                 //
//                                                                               //
//   Copyright (C) 2003-2009 James Slepicka <james@nemulator.com>                //
//                                                                               //
//   This program is free software; you can redistribute it and/or modify        //
//   it under the terms of the GNU General Public License as published by        //
//   the Free Software Foundation; either version 2 of the License, or           //
//   (at your option) any later version.                                         //
//                                                                               //
//   This program is distributed in the hope that it will be useful,             //
//   but WITHOUT ANY WARRANTY; without even the implied warranty of              //
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the               //
//   GNU General Public License for more details.                                //
//                                                                               //
//   You should have received a copy of the GNU General Public License           //
//   along with this program; if not, write to the Free Software                 //
//   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA   //
//                                                                               //
///////////////////////////////////////////////////////////////////////////////////

#include "cpu.h"
#include "mapper.h"
#include "nes.h"

#if defined(DEBUG) | defined(_DEBUG)
	#define DEBUG_NEW new(_CLIENT_BLOCK, __FILE__, __LINE__)
	#define new DEBUG_NEW
#endif

#define INLINE __forceinline
//#define INLINE
//#define MAKEWORD(a,b) (((a) & 0xFF) | (((b) & 0xFF) << 8))
#define PUSH(x) nes->WriteByte(SP-- | 0x100, x)
#define POP nes->ReadByte(++SP | 0x100)
#define SETN(x) SR.N = (x & 0x80) == 0x80 ? true : false
#define SETZ(x) SR.Z = x == 0 ? true : false
#define CHECK_PAGE_CROSS(a, x) if (check_page_cross && (x & 0xFF) > (a & 0xFF)) requiredCycles += 3
#define CHECK_PAGE_CROSS2(a, b) if (check_page_cross && ((a^b)&0x100)) requiredCycles += 3
//#define HIBYTE(a) (((a) >> 8) & 0xFF)
//#define LOBYTE(a) ((a) & 0xFF)

//Cycle counts.  Note: multiplied by 3 to measure in PPU-cycle time
//const int c_cpu::cycleTable[] = {
//        21, 18,  0,  0,  0,  9, 15,  0,  9,  6,  6,  0,  0, 12, 18,  0, //0F
//         6, 15,  0,  0,  0, 12, 18,  0,  6, 12,  0,  0,  0, 12, 21,  0, //1F
//        18, 18,  0,  0,  9,  9, 15,  0, 12,  6,  6,  0, 12, 12, 18,  0, //2F
//         6, 15,  0,  0,  0, 12, 18,  0,  6, 12,  0,  0,  0, 12, 21,  0, //3F
//        18, 18,  0,  0,  0,  9, 15,  0,  9,  6,  6,  0,  9, 12, 18,  0, //4F
//         6, 15,  0,  0,  0, 12, 18,  0,  6, 12,  0,  0,  0, 12, 21,  0, //5F
//        18, 18,  0,  0,  0,  9, 15,  0, 12,  6,  6,  0, 15, 12, 18,  0, //6F
//         6, 15,  0,  0,  0, 12, 18,  0,  6, 12,  0,  0,  0, 12, 21,  0, //7F
//         0, 18,  0,  0,  9,  9,  9,  0,  6,  6,  6,  0, 12, 12, 12,  0, //8F
//         6, 18,  0,  0, 12, 12, 12,  0,  6, 15,  6,  0,  0, 15,  0,  0, //9F
//         6, 18,  6,  0,  9,  9,  9,  0,  6,  6,  6,  0, 12, 12, 12,  0, //AF
//         6, 15,  0, 15, 12, 12, 12,  0,  6, 12,  6,  0, 12, 12, 12,  0, //BF
//         6, 18,  0,  0,  9,  9, 15,  0,  6,  6,  6,  0, 12, 12, 18,  0, //CF
//         6, 15,  0,  0,  0, 12, 18,  0,  6, 12,  0,  0,  0, 12, 21,  0, //DF
//         6, 18,  0,  0,  9,  9, 15,  0,  6,  6,  6,  0, 12, 12, 18,  0, //EF
//         6, 15,  0,  0,  0, 12, 18,  0,  6, 12,  0,  0,  0, 12, 21,  0, //FF
//        21, 21,  6, 12
//};

//replaced all zeros with 3 so that invalid opcodes don't cause cpu loop to spin
const int c_cpu::cycleTable[] = {
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


c_cpu::c_cpu(void)
{
	nes = 0;
}

c_cpu::~c_cpu(void)
{
}

int c_cpu::reset(void)
{
	cycles = 0;
	nmi_delay = 0;
	irq_delay = 0;
	S = (unsigned char*) &SR;
	*S = 0;
	SR.Unused = 1;
	SR.I = 1;
	doNmi = false;
	doIrq = false;
	availableCycles = 0;
	fetchOpcode = true;
	opcode = 0;
	requiredCycles = 0;
	irqPending = false;
	nmiPending = false;
	PC = MAKEWORD(nes->ReadByte(0xFFFC), nes->ReadByte(0xFFFD));
	SP = 0xFF;
	executecount = 0;
	dmaDst = 0;
	dmaSrc = 0;
	dmaPos = 0;
	doApuDMA = 0;
	return 1;
}

__forceinline void c_cpu::execute()
{
	for (;;)
	{
		if (fetchOpcode)
		{
			if (doApuDMA)
			{
				opcode = 0x103;
				doApuDMA--;
			}
			else if (dmaPos)
			{
				opcode = 0x102;
			}
			else if (doNmi && nmi_delay-- == 0)
			{
				opcode = 0x100;
				doNmi = false;
				nmi_delay = 0;
			}
			else if (doIrq && irq_delay-- == 0 && !SR.I)
			{
				opcode = 0x101;
				irq_delay = 0;
			}
			else
				opcode = nes->ReadByte(PC++);

			requiredCycles += cycleTable[opcode];
			fetchOpcode = false;
		}
		if (requiredCycles <= availableCycles)
		{
			availableCycles -= requiredCycles;
			if (nes->emulation_mode == c_nes::modes::EMULATION_MODE_FAST)
				nes->mapper->clock(requiredCycles);

			requiredCycles = 0;
			fetchOpcode = true;
			ExecuteOpcode();
		}
		else
			break;
	}
}

void c_cpu::execute3()
{
	availableCycles += 3;
	execute();
}

void c_cpu::execute_cycles(int numPpuCycles)
{
	availableCycles += numPpuCycles;
	execute();
}

void c_cpu::DoSpriteDMA(unsigned char *dst, int source_address)
{
	dmaDst = dst;
	dmaSrc = source_address;
	dmaPos = 256;
}

void c_cpu::ExecuteOpcode(void)
{
	check_page_cross = false;
	switch (opcode)
	{
	case 0x00: BRK(); break;
	case 0x01: IndirectX(); ORA(); break;
	case 0x05: ZeroPage(); ORA(); break;
	case 0x06: ZeroPage(); ASL(M); break;
	case 0x08: PHP(); break;
	case 0x09: Immediate(); ORA(); break;
	case 0x0A: ASL(A, true); break;
	case 0x0D: Absolute(); ORA(); break;
	case 0x0E: Absolute(); ASL(M); break;
	case 0x10: BPL(); break;
	case 0x11: check_page_cross = true; IndirectY(); ORA(); break;
	case 0x15: ZeroPageX(); ORA(); break;
	case 0x16: ZeroPageX(); ASL(M); break;
	case 0x18: CLC(); break;
	case 0x19: check_page_cross = true; AbsoluteY(); ORA(); break;
	case 0x1D: check_page_cross = true; AbsoluteX(); ORA(); break;
	case 0x1E: AbsoluteX(); ASL(M); break;
	case 0x20: Absolute(); JSR(); break;
	case 0x21: IndirectX(); AND(); break;
	case 0x24: ZeroPage(); BIT(); break;
	case 0x25: ZeroPage(); AND(); break;
	case 0x26: ZeroPage(); ROL(M); break;
	case 0x28: PLP(); break;
	case 0x29: Immediate(); AND(); break;
	case 0x2A: ROL(A, true); break;
	case 0x2C: Absolute(); BIT(); break;
	case 0x2D: Absolute(); AND(); break;
	case 0x2E: Absolute(); ROL(M); break;
	case 0x30: BMI(); break;
	case 0x31: check_page_cross = true; IndirectY(); AND(); break;
	case 0x35: ZeroPageX(); AND(); break;
	case 0x36: ZeroPageX(); ROL(M); break;
	case 0x38: SEC(); break;
	case 0x39: check_page_cross = true; AbsoluteY(); AND(); break;
	case 0x3D: check_page_cross = true; AbsoluteX(); AND(); break;
	case 0x3E: AbsoluteX(); ROL(M); break;
	case 0x40: RTI(); break;
	case 0x41: IndirectX(); EOR(); break;
	case 0x45: ZeroPage(); EOR(); break;
	case 0x46: ZeroPage(); LSR(M); break;
	case 0x48: PHA(); break;
	case 0x49: Immediate(); EOR(); break;
	case 0x4A: LSR(A, true); break;
	case 0x4B: ALR(); break;
	case 0x4C: Absolute(); JMP(); break;
	case 0x4D: Absolute(); EOR(); break;
	case 0x4E: Absolute(); LSR(M); break;
	case 0x50: BVC(); break;
	case 0x51: check_page_cross = true; IndirectY(); EOR(); break;
	case 0x55: ZeroPageX(); EOR(); break;
	case 0x56: ZeroPageX(); LSR(M); break;
	case 0x58: CLI(); break;
	case 0x59: check_page_cross = true; AbsoluteY(); EOR(); break;
	case 0x5D: check_page_cross = true; AbsoluteX(); EOR(); break;
	case 0x5E: AbsoluteX(); LSR(M); break;
	case 0x60: RTS(); break;
	case 0x61: IndirectX(); ADC(); break;
	case 0x65: ZeroPage(); ADC(); break;
	case 0x66: ZeroPage(); ROR(M); break;
	case 0x68: PLA(); break;
	case 0x69: Immediate(); ADC(); break;
	case 0x6A: ROR(A, true); break;
	case 0x6C: Indirect(); JMP(); break;
	case 0x6D: Absolute(); ADC(); break;
	case 0x6E: Absolute(); ROR(M); break;
	case 0x70: BVS(); break;
	case 0x71: check_page_cross = true; IndirectY(); ADC(); break;
	case 0x75: ZeroPageX(); ADC(); break;
	case 0x76: ZeroPageX(); ROR(M); break;
	case 0x78: SEI(); break;
	case 0x79: check_page_cross = true; AbsoluteY(); ADC(); break;
	case 0x7D: check_page_cross = true; AbsoluteX(); ADC(); break;
	case 0x7E: AbsoluteX(); ROR(M); break;
	case 0x81: IndirectX(false); STA(); break;
	case 0x84: ZeroPage(false); STY(); break;
	case 0x85: ZeroPage(false); STA(); break;
	case 0x86: ZeroPage(false); STX(); break;
	case 0x88: DEY(); break;
	case 0x89: SKB(); break;
	case 0x8A: TXA(); break;
	case 0x8C: Absolute(false); STY(); break;
	case 0x8D: Absolute(false); STA(); break;
	case 0x8E: Absolute(false); STX(); break;
	case 0x8F: Absolute(false); SAX(); break;
	case 0x90: BCC(); break;
	case 0x91: IndirectY(false); STA(); break;
	case 0x94: ZeroPageX(false); STY(); break;
	case 0x95: ZeroPageX(false); STA(); break;
	case 0x96: ZeroPageY(false); STX(); break;
	case 0x98: TYA(); break;
	case 0x99: AbsoluteY(false); STA(); break;
	case 0x9A: TXS(); break;
	case 0x9D: AbsoluteX(false); STA(); break;
	case 0xA0: Immediate(); LDY(); break;
	case 0xA1: IndirectX(); LDA(); break;
	case 0xA2: Immediate(); LDX(); break;
	case 0xA4: ZeroPage(); LDY(); break;
	case 0xA5: ZeroPage(); LDA(); break;
	case 0xA6: ZeroPage(); LDX(); break;
	case 0xA7: ZeroPage(); LAX(); break;
	case 0xA8: TAY(); break;
	case 0xA9: Immediate(); LDA(); break;
	case 0xAA: TAX(); break;
	case 0xAC: Absolute(); LDY(); break;
	case 0xAD: Absolute(); LDA(); break;
	case 0xAE: Absolute(); LDX(); break;
	case 0xB0: BCS(); break;
	case 0xB1: check_page_cross = true; IndirectY(); LDA(); break;
	case 0xB3: check_page_cross = true; IndirectY(); LAX(); break;
	case 0xB4: ZeroPageX(); LDY(); break;
	case 0xB5: ZeroPageX(); LDA(); break;
	case 0xB6: ZeroPageY(); LDX(); break;
	case 0xB8: CLV(); break;
	case 0xB9: check_page_cross = true; AbsoluteY(); LDA(); break;
	case 0xBA: TSX(); break;
	case 0xBC: check_page_cross = true; AbsoluteX(); LDY(); break;
	case 0xBD: check_page_cross = true; AbsoluteX(); LDA(); break;
	case 0xBE: check_page_cross = true; AbsoluteY(); LDX(); break;
	case 0xC0: Immediate(); CPY(); break;
	case 0xC1: IndirectX(); CMP(); break;
	case 0xC4: ZeroPage(); CPY(); break;
	case 0xC5: ZeroPage(); CMP(); break;
	case 0xC6: ZeroPage(); DEC(); break;
	case 0xC8: INY(); break;
	case 0xC9: Immediate(); CMP(); break;
	case 0xCA: DEX(); break;
	case 0xCB: AXS(); break;
	case 0xCC: Absolute(); CPY(); break;
	case 0xCD: Absolute(); CMP(); break;
	case 0xCE: Absolute(); DEC(); break;
	case 0xD0: BNE(); break;
	case 0xD1: check_page_cross = true; IndirectY(); CMP(); break;
	case 0xD5: ZeroPageX(); CMP(); break;
	case 0xD6: ZeroPageX(); DEC(); break;
	case 0xD8: CLD(); break;
	case 0xD9: check_page_cross = true; AbsoluteY(); CMP(); break;
	case 0xDB: AbsoluteY(); DCP(); break;
	case 0xDD: check_page_cross = true; AbsoluteX(); CMP(); break;
	case 0xDE: AbsoluteX(); DEC(); break;
	case 0xE0: Immediate(); CPX(); break;
	case 0xE1: IndirectX(); SBC(); break;
	case 0xE4: ZeroPage(); CPX(); break;
	case 0xE5: ZeroPage(); SBC(); break;
	case 0xE6: ZeroPage(); INC(); break;
	case 0xE8: INX(); break;
	case 0xE9: Immediate(); SBC(); break;
	case 0xEA: break;
	case 0xEC: Absolute(); CPX(); break;
	case 0xED: Absolute(); SBC(); break;
	case 0xEE: Absolute(); INC(); break;
	case 0xF0: BEQ(); break;
	case 0xF1: check_page_cross = true; IndirectY(); SBC(); break;
	case 0xF5: ZeroPageX(); SBC(); break;
	case 0xF6: ZeroPageX(); INC(); break;
	case 0xF8: SED(); break;
	case 0xF9: check_page_cross = true; AbsoluteY(); SBC(); break;
	case 0xFB: AbsoluteY(); ISC(); break;
	case 0xFD: check_page_cross = true; AbsoluteX(); SBC(); break;
	case 0xFE: AbsoluteX(); INC(); break;
	case 0x100: //NMI
		PUSH(HIBYTE(PC));
		PUSH(LOBYTE(PC));
		SR.B = false;
		PUSH(*S);
		SR.I = true;
		PC = MAKEWORD(nes->ReadByte(0xFFFA), nes->ReadByte(0xFFFB));
		break;
	case 0x101: //IRQ
		PUSH(HIBYTE(PC));
		PUSH(LOBYTE(PC));
		SR.B = false;
		PUSH(*S);
		SR.I = true;
		PC = MAKEWORD(nes->ReadByte(0xFFFE), nes->ReadByte(0xFFFF));
		break;
	case 0x102: //Sprite DMA
		*(dmaDst++) = nes->ReadByte(dmaSrc++);
		dmaPos--;
		//Sprite DMA burns 513 CPU cycles, so after the last DMA, burn another one.
		if (dmaPos == 0)
			requiredCycles += 3;
		break;
	case 0x103: //APU DMA
		{int a = 1;}
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

void c_cpu::ExecuteApuDMA()
{
	//doApuDMA++;
	//if (doApuDMA > 1)
	//	int x = 1;
	requiredCycles += 12;
}

int c_cpu::irq_checked()
{
	//irqs are checked 2 cycles before an opcode completes
	//if this check has already occured, return 1
	//return (fetchOpcode || ((requiredCycles > availableCycles) && ((requiredCycles - availableCycles) < 6)));
	if (fetchOpcode || availableCycles > (requiredCycles - 3))
		return 1;
	else
		return 0;
}

void c_cpu::execute_nmi()
{
	nmi_delay = irq_checked();
	doNmi = true;
}

void c_cpu::execute_irq(void)
{
	irq_delay = irq_checked();
	doIrq = true;
}

void c_cpu::clear_irq()
{
	doIrq = false;
}

INLINE void c_cpu::Immediate(void)
{
	M = nes->ReadByte(PC++);
}

INLINE void c_cpu::ZeroPage(bool bReadMem)
{
	Address = nes->ReadByte(PC++);
	if (bReadMem) M = nes->ReadByte(Address);
}

INLINE void c_cpu::ZeroPageX(bool bReadMem)
{
	Address = nes->ReadByte(PC++);
	Address += X;
	Address &= 0xFF;
	if (bReadMem) M = nes->ReadByte(Address);
}

INLINE void c_cpu::ZeroPageY(bool bReadMem)
{
	Address = nes->ReadByte(PC++);
	Address += Y;
	Address &= 0xFF;
	if (bReadMem) M = nes->ReadByte(Address);
}

INLINE void c_cpu::Absolute(bool bReadMem)
{
	unsigned char lo = nes->ReadByte(PC++);
	unsigned char hi = nes->ReadByte(PC++);
	Address = MAKEWORD(lo, hi);
	if (bReadMem) M = nes->ReadByte(Address);
}

INLINE void c_cpu::AbsoluteX(bool bReadMem)
{
	unsigned char lo = nes->ReadByte(PC++);
	unsigned char hi = nes->ReadByte(PC++);
	Address = MAKEWORD(lo, hi);
	int temp = Address + X;
	CHECK_PAGE_CROSS2(Address, temp);
	Address = temp;
	//Address += X;
	//CHECK_PAGE_CROSS(Address, X);
	
	if (bReadMem) M = nes->ReadByte(Address);
}

INLINE void c_cpu::AbsoluteY(bool bReadMem)
{
	unsigned char lo = nes->ReadByte(PC++);
	unsigned char hi = nes->ReadByte(PC++);
	Address = MAKEWORD(lo, hi);
	int temp = Address + Y;
	CHECK_PAGE_CROSS2(Address, temp);
	Address = temp;
	//Address += Y;
	//CHECK_PAGE_CROSS(Address, Y);
	if (bReadMem) M = nes->ReadByte(Address);
}

INLINE void c_cpu::Indirect(void)	//For indirect jmp
{
	unsigned char lo = nes->ReadByte(PC++);
	unsigned char hi = nes->ReadByte(PC++);
	unsigned short tempaddress = MAKEWORD(lo, hi);
	unsigned char pcl = nes->ReadByte(tempaddress);
	lo++;
	tempaddress = MAKEWORD(lo, hi);
	unsigned char pch = nes->ReadByte(tempaddress);
	Address = MAKEWORD(pcl, pch);
}

INLINE void c_cpu::IndirectX(bool bReadMem)	//Pre-indexed indirect
{
	/*unsigned short temp = nes->ReadByte(PC++);
	unsigned char hi = (temp & 0xFF00) >> 2;
	temp += X;
	temp &= 0xFF;
	temp |= (hi << 4);
	Address = MAKEWORD(nes->ReadByte(temp), nes->ReadByte(temp + 1));
	if (bReadMem) M = nes->ReadByte(Address);
	*/

	//TODO: I think this is ok, but need to verify
	unsigned char hi = nes->ReadByte(PC++);
	hi += X;
	Address = MAKEWORD(nes->ReadByte(hi), nes->ReadByte((hi + 1) & 0xFF));
	//CHECK_PAGE_CROSS(Address, X);
	if (bReadMem) M = nes->ReadByte(Address);
}

INLINE void c_cpu::IndirectY(bool bReadMem)	//Post-indexed indirect
{
	unsigned char temp = nes->ReadByte(PC++);
	Address = MAKEWORD(nes->ReadByte(temp), nes->ReadByte((temp + 1) & 0xFF));
	
	int temp2 = Address + Y;
	CHECK_PAGE_CROSS2(Address, temp2);
	Address = temp2;
	
	//Address += Y;

	//if ((Y > (unsigned char)Address) && ((Address & 0xFF00) == 0x0100))
	//	Address &= 0xFF;

	//CHECK_PAGE_CROSS(Address, Y);
	if (bReadMem) M = nes->ReadByte(Address);
}

INLINE void c_cpu::Branch(int Condition)
{
	if (Condition)
	{
		requiredCycles += 3;
		signed char offset = nes->ReadByte(PC++);
		int temp = PC;
		PC += offset;
		if ((PC & 0xFF00) != (temp & 0xFF00))
			requiredCycles += 3;
	}
	else PC++;
}

INLINE void c_cpu::ADC(void)
{
	unsigned int temp = A + M + (SR.C ? 1 : 0);
	SR.C = temp > 0xFF ? true : false;
	//SR.V = (((~(A^M)) & 0x80) & ((A^temp) & 0x80)) ? true : false;
	//if A and result have different sign and M and result have different sign, set overflow
	//127 + 1 = -128 or -1 - -3 = 2 will overflow
	//127 + -128
	SR.V = (A^temp) & (M^temp) & 0x80 ? true : false;
	A = temp & 0xFF;
	SETN(A);
	SETZ(A);
}
INLINE void c_cpu::AND(void)
{
	A = A & M;
	SETN(A);
	SETZ(A);
}
INLINE void c_cpu::ALR()
{
	A &= nes->ReadByte(PC++);
	SR.C = A & 0x1;
	SR.N = 0;
	A >>= 1;
	SETZ(A);
}
INLINE void c_cpu::ASL(unsigned char & Operand, bool bRegister)
{
	SR.C = Operand & 0x80 ? true : false;
	Operand <<= 1;
	if (!bRegister) nes->WriteByte(Address, Operand);
	SETN(Operand);
	SETZ(Operand);
}
INLINE void c_cpu::AXS()
{
	X &= A;
	unsigned short temp = X - nes->ReadByte(PC++);
	if (temp > 0xFF)
		SR.C = true;
	X = temp & 0xFF;
	SETN(X);
	SETZ(X);
	
}
INLINE void c_cpu::BCC(void)
{
	Branch(SR.C == false);
}
INLINE void c_cpu::BCS(void)
{
	Branch(SR.C == true);
}
INLINE void c_cpu::BEQ(void)
{
	Branch(SR.Z == true);
}
INLINE void c_cpu::BIT(void)
{
	unsigned char result = A & M;
	SETZ(result);
	SR.V = M & 0x40 ? true : false;
	SETN(M);
}
INLINE void c_cpu::BMI(void)
{
	Branch(SR.N == true);
}
INLINE void c_cpu::BNE(void)
{
	Branch(SR.Z == false);
}
INLINE void c_cpu::BPL(void)
{
	Branch(SR.N == false);
}
INLINE void c_cpu::BRK(void)
{
	PUSH(HIBYTE(PC+1));
	PUSH(LOBYTE(PC+1));
	PUSH(*S | 0x30);
	SR.B = true;
	SR.I = true;
	PC = MAKEWORD(nes->ReadByte(0xFFFE), nes->ReadByte(0xFFFF));
}
INLINE void c_cpu::BVC(void)
{
	Branch(SR.V == false);
}
INLINE void c_cpu::BVS(void)
{
	Branch(SR.V == true);
}
INLINE void c_cpu::CLC(void)
{
	SR.C = false;
}
INLINE void c_cpu::CLD(void)
{
	SR.D = false;
}
INLINE void c_cpu::CLI(void)
{
	SR.I = false;
	if (doIrq)
		irq_delay = 1;
}
INLINE void c_cpu::CLV(void)
{
	SR.V = false;
}
INLINE void c_cpu::CMP(void)
{
	unsigned char temp = A - M;
	SETZ(temp);
	SETN(temp);
	SR.C = M <= A ? true : false;
}
INLINE void c_cpu::CPX(void)
{
	unsigned char temp = X - M;
	SETZ(temp);
	SETN(temp);
	SR.C = M <= X ? true : false;
}
INLINE void c_cpu::CPY(void)
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
INLINE void c_cpu::DEC(void)
{
	M--;
	nes->WriteByte(Address, M);
	SETN(M);
	SETZ(M);
}
INLINE void c_cpu::DEX(void)
{
	X--;
	SETN(X);
	SETZ(X);
}
INLINE void c_cpu::DEY(void)
{
	Y--;
	SETN(Y);
	SETZ(Y);
}
INLINE void c_cpu::EOR(void)
{
	A ^= M;
	SETN(A);
	SETZ(A);
}
INLINE void c_cpu::INC(void)
{
	nes->WriteByte(Address, M);
	M++;
	nes->WriteByte(Address, M);
	SETN(M);
	SETZ(M);
}
INLINE void c_cpu::INX(void)
{
	X++;
	SETN(X);
	SETZ(X);
}
INLINE void c_cpu::INY(void)
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
INLINE void c_cpu::JMP(void)
{
	PC = Address;
}
INLINE void c_cpu::JSR(void)
{
	PC--;
	PUSH(HIBYTE(PC));
	PUSH(LOBYTE(PC));
	PC = Address;
}
INLINE void c_cpu::LAX()
{
	LDX();
	TXA();
}
INLINE void c_cpu::LDA(void)
{
	A = M;
	SETN(A);
	SETZ(A);
}
INLINE void c_cpu::LDX(void)
{
	X = M;
	SETN(X);
	SETZ(X);
}
INLINE void c_cpu::LDY(void)
{
	Y = M;
	SETN(Y);
	SETZ(Y);
}
INLINE void c_cpu::LSR(unsigned char & Operand, bool bRegister)
{
	SR.C = Operand & 0x01 ? true : false;
	Operand >>= 1;
	SETZ(Operand);
	SR.N = false;
	if (!bRegister) nes->WriteByte(Address, Operand);
}
INLINE void c_cpu::ORA(void)
{
	A |= M;
	SETZ(A);
	SETN(A);
}

INLINE void c_cpu::PHA(void)
{
	PUSH(A);
}
INLINE void c_cpu::PHP(void)
{
	PUSH(*S | 0x30);
}
INLINE void c_cpu::PLA(void)
{
	A = POP;
	SETN(A);
	SETZ(A);
}
INLINE void c_cpu::PLP(void)
{
	bool prev_i = SR.I;
	*S = POP;
	SR.B = false;
	SR.Unused = true;
	if (doIrq && SR.I == false && SR.I != prev_i)
		irq_delay = 1;
}
INLINE void c_cpu::ROL(unsigned char & Operand, bool bRegister)
{
	int oldcarry = SR.C ? 1 : 0;
	SR.C = (Operand & 0x80) ? true : false;
	Operand <<= 1;
	if (oldcarry)
	{
		Operand |= 0x01;
	}
	SETN(Operand);
	SETZ(Operand);
	if (!bRegister) nes->WriteByte(Address, Operand);
}
INLINE void c_cpu::ROR(unsigned char & Operand, bool bRegister)
{
	int oldcarry = SR.C ? 1 : 0;
	SR.C = (Operand & 0x01) ? true : false;
	Operand >>= 1;
	if (oldcarry)
	{
		Operand |= 0x80;
	}
	SETN(Operand);
	SETZ(Operand);
	if (!bRegister) nes->WriteByte(Address, Operand);
}
INLINE void c_cpu::RTI(void)
{
	bool prev_i = SR.I;
	*S = POP;
	SR.B = false;
	unsigned char pcl = POP;
	unsigned char pch = POP;
	PC = MAKEWORD(pcl, pch);
		if (doIrq && SR.I == false && SR.I != prev_i)
		irq_delay = 1;
}
INLINE void c_cpu::RTS(void)
{
	unsigned char pcl = POP;
	unsigned char pch = POP;
	PC = MAKEWORD(pcl, pch);
	PC++;
}
INLINE void c_cpu::SBC(void)
{
	/*
	unsigned short temp = A - M - (SR.C ? 0 : 1);
	SR.C = temp < 0x100 ? true : false;
	//SR.V = (((A^temp) & 0x80) & ((A^M) & 0x80)) ? true : false;
	SR.V = (A^temp) & (M^A) & 0x80 ? true : false;
	A = temp & 0xFF;
	SETN(A);
	SETZ(A);
	*/
	M ^= 0xFF;
	ADC();
}
INLINE void c_cpu::SEC(void)
{
	SR.C = true;
}
INLINE void c_cpu::SED(void)
{
	SR.D = true;
}
INLINE void c_cpu::SEI(void)
{
	SR.I = true;
}

INLINE void c_cpu::SKB(void)
{
	PC++;
}
INLINE void c_cpu::SAX()
{
	nes->WriteByte(Address, A&X);
}
INLINE void c_cpu::STA(void)
{
	nes->WriteByte(Address, A);
}
INLINE void c_cpu::STX(void)
{
	nes->WriteByte(Address, X);
}
INLINE void c_cpu::STY(void)
{
	nes->WriteByte(Address, Y);
}
INLINE void c_cpu::TAX(void)
{
	X = A;
	SETN(X);
	SETZ(X);
}
INLINE void c_cpu::TAY(void)
{
	Y = A;
	SETN(Y);
	SETZ(Y);
}
INLINE void c_cpu::TXA(void)
{
	A = X;
	SETN(A);
	SETZ(A);
}
INLINE void c_cpu::TYA(void)
{
	A = Y;
	SETN(A);
	SETZ(A);
}
INLINE void c_cpu::TSX(void)
{
	X = SP;
	SETN(X);
	SETZ(X);
}
INLINE void c_cpu::TXS(void)
{
	SP = X;
}
