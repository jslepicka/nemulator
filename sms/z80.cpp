#include "z80.h"
#include "sms.h"
#include <stdio.h>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <iostream>
#include <Windows.h>

#include <crtdbg.h>
#if defined(DEBUG) | defined(_DEBUG)
#define DEBUG_NEW new(_CLIENT_BLOCK, __FILE__, __LINE__)
#define new DEBUG_NEW
#endif

#define BYTETOBINARY(byte)  \
  (byte & 0x80 ? 1 : 0), \
  (byte & 0x40 ? 1 : 0), \
  0, \
  (byte & 0x10 ? 1 : 0), \
  0, \
  (byte & 0x04 ? 1 : 0), \
  (byte & 0x02 ? 1 : 0), \
  (byte & 0x01 ? 1 : 0) 

int DEBUG = 0;
#define dprintf(fmt, ...) do { if (DEBUG) printf(fmt, __VA_ARGS__); } while (0)

using namespace std;

c_z80::c_z80(c_sms *sms)
{
	pending_ei = 0;
	instruction = 0;
	pre_pc = 0;
	int count = 0;
	this->sms = sms;
}


c_z80::~c_z80(void)
{
}

int c_z80::reset()
{
	pending_psg_cycles = 0;
	needed_cycles = 0;
	prev_nmi = 0;
	halted = 0;
	available_cycles = 0;
	required_cycles = 0;
	fetch_opcode = 1;
	opcode = 0;
	prefix = 0;
	PC = 0;
	IFF1 = 0;
	IFF2 = 0;
	IM = 0;
	AF.word = AF2.word = 0x0000;
	BC.word = BC2.word = 0x0000;
	DE.word = DE2.word = 0x0000;
	HL.word = HL2.word = 0x0000;
	IX.word = 0x0000;
	IY.word = 0x0000;
	SP = 0xDFF0;
	I = 0;
	R = 0;

	r_00[0] = r_dd[0] = r_fd[0] = &BC.byte.hi;
	r_00[1] = r_dd[1] = r_fd[1] = &BC.byte.lo;
	r_00[2] = r_dd[2] = r_fd[2] = &DE.byte.hi;
	r_00[3] = r_dd[3] = r_fd[3] = &DE.byte.lo;
	r_00[4] = &HL.byte.hi;
	r_dd[4] = &IX.byte.hi;
	r_fd[4] = &IY.byte.hi;
	r_00[5] = &HL.byte.lo;
	r_dd[5] = &IX.byte.lo;
	r_fd[5] = &IY.byte.lo;
	r_00[6] = r_dd[6] = r_fd[6] = 0;
	r_00[7] = r_dd[7] = r_fd[7] = &AF.byte.hi;
	r = r_00;

	rp_00[0] = rp_dd[0] = rp_fd[0] = &BC.word;
	rp_00[1] = rp_dd[1] = rp_fd[1] = &DE.word;
	rp_00[2] = &HL.word;
	rp_dd[2] = &IX.word;
	rp_fd[2] = &IY.word;
	rp_00[3] = rp_dd[3] = rp_fd[3] = &SP;
	rp = rp_00;

	rp2_00[0] = rp2_dd[0] = rp2_fd[0] = &BC.word;
	rp2_00[1] = rp2_dd[1] = rp2_fd[1] = &DE.word;
	rp2_00[2] = &HL.word;
	rp2_dd[2] = &IX.word;
	rp2_fd[2] = &IY.word;
	rp2_00[3] = rp2_dd[3] = rp2_fd[3] = &AF.word;
	rp2 = rp2_00;


	return 1;
}

int c_z80::emulate_frame()
{
	const int cycles_per_frame = 3579545 / 60; //3.58Mhz/60fps
	execute(cycles_per_frame);
	return 1;
}

const int c_z80::cycle_table[261] = {
	//		0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
	/*0*/	4, 10,  7,  6,  4,  4,  7,  4,  4, 11,  7,  6,  4,  4,  7,  4,
	/*1*/	8, 10,  7,  6,  4,  4,  7,  4, 12, 11,  7,  6,  4,  4,  7,  4, //check 18
	/*2*/	7, 10, 16,  6,  4,  4,  7,  4,  7, 11, 16,  6,  4,  4,  7,  4,
	/*3*/	7, 10, 13,  6, 11, 11, 10,  4,  7, 11, 13,  6,  4,  4,  7,  4,
	/*4*/	4,  4,  4,  4,  4,  4,  7,  4,  4,  4,  4,  4,  4,  4,  7,  4,
	/*5*/	4,  4,  4,  4,  4,  4,  7,  4,  4,  4,  4,  4,  4,  4,  7,  4,
	/*6*/	4,  4,  4,  4,  4,  4,  7,  4,  4,  4,  4,  4,  4,  4,  7,  4,
	/*7*/	7,  7,  7,  7,  7,  7,  4,  7,  4,  4,  4,  4,  4,  4,  7,  4,
	/*8*/	4,  4,  4,  4,  4,  4,  7,  4,  4,  4,  4,  4,  4,  4,  7,  4,
	/*9*/	4,  4,  4,  4,  4,  4,  7,  4,  4,  4,  4,  4,  4,  4,  7,  4,
	/*A*/	4,  4,  4,  4,  4,  4,  7,  4,  4,  4,  4,  4,  4,  4,  7,  4,
	/*B*/	4,  4,  4,  4,  4,  4,  7,  4,  4,  4,  4,  4,  4,  4,  7,  4,
	/*C*/	5, 10, 10, 10, 10, 11,  7, 11,  5, 10, 10, 99, 19, 17,  7, 11, //check if 99 impacts anything
	/*D*/	5, 10, 10, 11, 10, 11,  7, 11,  5,  4, 10, 11, 10, 99,  7, 11,
	/*E*/	5, 10, 10, 19, 10, 11,  7, 11,  5,  4, 10,  4, 10, 99,  7, 11,
	/*F*/	5, 10, 10,  4, 10, 11,  7, 11,  5,  6, 10,  4, 10, 99,  7, 11,
	/*10*/  1, 13,  1,  4, 11
};

const int c_z80::cb_cycle_table[256] = {
	//		0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
	/*0*/	8,  8,  8,  8,  8,  8, 15,  8,  8,  8,  8,  8,  8,  8, 15,  8,
	/*1*/	8,  8,  8,  8,  8,  8, 15,  8,  8,  8,  8,  8,  8,  8, 15,  8,
	/*2*/	8,  8,  8,  8,  8,  8, 15,  8,  8,  8,  8,  8,  8,  8, 15,  8,
	/*3*/	8,  8,  8,  8,  8,  8, 15,  8,  8,  8,  8,  8,  8,  8, 15,  8,
	/*4*/	8,  8,  8,  8,  8,  8, 12,  8,  8,  8,  8,  8,  8,  8, 12,  8,
	/*5*/	8,  8,  8,  8,  8,  8, 12,  8,  8,  8,  8,  8,  8,  8, 12,  8,
	/*6*/	8,  8,  8,  8,  8,  8, 12,  8,  8,  8,  8,  8,  8,  8, 12,  8,
	/*7*/	8,  8,  8,  8,  8,  8, 12,  8,  8,  8,  8,  8,  8,  8, 12,  8,
	/*8*/	8,  8,  8,  8,  8,  8, 15,  8,  8,  8,  8,  8,  8,  8, 15,  8,
	/*9*/	8,  8,  8,  8,  8,  8, 15,  8,  8,  8,  8,  8,  8,  8, 15,  8,
	/*A*/	8,  8,  8,  8,  8,  8, 15,  8,  8,  8,  8,  8,  8,  8, 15,  8,
	/*B*/   8,  8,  8,  8,  8,  8, 15,  8,  8,  8,  8,  8,  8,  8, 15,  8,
	/*C*/	8,  8,  8,  8,  8,  8, 15,  8,  8,  8,  8,  8,  8,  8, 15,  8,
	/*D*/	8,  8,  8,  8,  8,  8, 15,  8,  8,  8,  8,  8,  8,  8, 15,  8,
	/*E*/	8,  8,  8,  8,  8,  8, 15,  8,  8,  8,  8,  8,  8,  8, 15,  8,
	/*F*/	8,  8,  8,  8,  8,  8, 15,  8,  8,  8,  8,  8,  8,  8, 15,  8,
};

const int c_z80::dd_cycle_table[256] = {
	//		0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
	/*0*/	1,  1,  1,  1,  1,  1,  1,  1,  1, 15,  1,  1,  1,  1,  1,  1,
	/*1*/	1,  1,  1,  1,  1,  1,  1,  1,  1, 15,  1,  1,  1,  1,  1,  1,
	/*2*/	1, 14, 20, 10,  8,  8, 11,  1,  1, 15, 20, 10,  8,  8, 11,  1,
	/*3*/	1,  1,  1,  1, 23, 23, 19,  1,  1, 15,  1,  1,  1,  1,  1,  1,
	/*4*/	1,  1,  1,  1,  8,  8, 19,  1,  1,  1,  1,  1,  8,  8, 19,  1,
	/*5*/	1,  1,  1,  1,  8,  8, 19,  1,  1,  1,  1,  1,  8,  8, 19,  1,
	/*6*/	8,  8,  8,  8,  8,  8, 19,  8,  8,  8,  8,  8,  8,  8, 19,  8,
	/*7*/  19, 19, 19, 19, 19, 19,  1, 19,  1,  1,  1,  1,  8,  8, 19,  1,
	/*8*/	1,  1,  1,  1,  8,  8, 19,  1,  1,  1,  1,  1,  8,  8, 19,  1,
	/*9*/	1,  1,  1,  1,  8,  8, 19,  1,  1,  1,  1,  1,  8,  8, 19,  1,
	/*A*/	1,  1,  1,  1,  8,  8, 19,  1,  1,  1,  1,  1,  8,  8, 19,  1,
	/*B*/   1,  1,  1,  1,  8,  8, 19,  1,  1,  1,  1,  1,  8,  8, 19,  1,
	/*C*/	1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1, 99,  1,  1,  1,
	/*D*/	1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
	/*E*/	1, 14,  1, 23,  1, 15,  1,  1,  1,  8,  1,  1,  1,  1,  1,  1,
	/*F*/	1,  1,  1,  1,  1,  1,  1,  1,  1, 10,  1,  1,  1,  1,  1,  1,
};

const int c_z80::ed_cycle_table[256] = {
	//		0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
	/*0*/	1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
	/*1*/	1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
	/*2*/	1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
	/*3*/	1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
	/*4*/  12, 12, 15, 20,  8, 14,  8,  9, 12, 12, 15, 20,  8, 14,  8,  9,
	/*5*/  12, 12, 15, 20,  8, 14,  8,  9, 12, 12, 15, 20,  8, 14,  8,  9,
	/*6*/  12, 12, 15, 20,  8, 14,  8, 18, 12, 12, 15, 20,  8, 14,  8, 18,
	/*7*/  12, 12, 15, 20,  8, 14,  8,  1, 12, 12, 15, 20,  8, 14,  8,  1,
	/*8*/	1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
	/*9*/	1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
	/*A*/  16, 16, 16, 16,  1,  1,  1,  1, 16, 16, 16, 16,  1,  1,  1,  1,
	/*B*/  16, 16, 16, 16,  1,  1,  1,  1, 16, 16, 16, 16,  1,  1,  1,  1,
	/*C*/	1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
	/*D*/	1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
	/*E*/	1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
	/*F*/	1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
};


const int c_z80::fd_cycle_table[256] = {
	//		0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
	/*0*/	1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
	/*1*/	1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
	/*2*/	1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
	/*3*/	1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
	/*4*/	1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
	/*5*/	1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
	/*6*/	1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
	/*7*/	1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
	/*8*/	1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
	/*9*/	1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
	/*A*/	1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
	/*B*/   1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
	/*C*/	1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
	/*D*/	1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
	/*E*/	1, 14,  1,  1,  1, 15,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
	/*F*/	1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
};

const int c_z80::ddcb_cycle_table[256] = {
	//		0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
	/*0*/  23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
	/*1*/  23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
	/*2*/  23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
	/*3*/  23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
	/*4*/  20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
	/*5*/  20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
	/*6*/  20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
	/*7*/  20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
	/*8*/  23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
	/*9*/  23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
	/*A*/  23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
	/*B*/  23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
	/*C*/  23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
	/*D*/  23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
	/*E*/  23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
	/*F*/  23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
};

const int c_z80::cc[8] = {
	1 << 6, 1 << 6, 1 << 0, 1 << 0, 1 << 2, 1 << 2, 1 << 7, 1 << 7
};

void c_z80::inc_r()
{
	int R8 = R & 0x80;
	R = R8 | ((R++) & 0x7F);
}

void c_z80::execute(int cycles)
{
	available_cycles += cycles;
	while (true)
	{
		if (fetch_opcode)
		{
			inc_r();
			pre_pc = PC;
			dd = 0;
			fd = 0;
			r = r_00;
			rp = rp_00;
			rp2 = rp2_00;

			if (sms->nmi && prev_nmi == 0)
			{
				opcode = 0x104;
			}
			else if (sms->irq && IFF1)
			{
				//printf("IRQ at 0x%04X\n", PC);
				IFF1 = IFF2 = 0;
				halted = 0;
				switch (IM)
				{
				case 0:
				case 2:
					printf("invalid interrupt!\n");
					break;
				case 1:
					opcode = 0x101;
					break;
				}
			}
			else if (halted)
			{
				opcode = 0x103;
			}
			else
			{
				dprintf("%04X ", PC);
				if (instruction >= 0)
				{
					//printf("%08d PC:%04X AF:%04X BC:%04X DE:%04X HL:%04X IX:%04X IY:%04X SP:%04X\n", instruction, PC, AF.word & 0xFFD7, BC.word, DE.word, HL.word, IX.word, IY.word, SP);
				}
				instruction++;
				//if ((instruction & 0xFFFF) == 0)
				//{
				//	char t[32];
				//	sprintf(t, "%d\n", instruction);
				//	fputs(t, stderr);
				//}
				opcode = sms->read_byte(PC++);
				if (pending_ei)
				{
					IFF1 = IFF2 = 1;
					pending_ei = 0;
				}
			}

			prev_nmi = sms->nmi;

			switch (opcode)
			{
			case 0xCB:
				inc_r();
				opcode <<= 8;
				opcode |= sms->read_byte(PC++);
				required_cycles += cb_cycle_table[opcode & 0xFF];
				prefix = 0xCB;
				break;
			case 0xDD:
				inc_r();
				dd = 1;
				opcode <<= 8;
				opcode |= sms->read_byte(PC++);
				if ((opcode & 0xFF) == 0xCB)
				{
					opcode <<= 8;
					opcode |= sms->read_byte(PC++);
					opcode <<= 8;
					opcode |= sms->read_byte(PC++);
					required_cycles += ddcb_cycle_table[opcode & 0xFF];
					prefix = 0xDDCB;
				}
				else
				{
					//replace HL, H, and L, with IX, IXH, and IXL
					//replace (HL) with (IX+d)
					required_cycles += dd_cycle_table[opcode & 0xFF];
					prefix = 0x00;
					r = r_dd;
					rp = rp_dd;
					rp2 = rp2_dd;
				}
				break;
			case 0xED:
				inc_r();
				opcode <<= 8;
				opcode |= sms->read_byte(PC++);
				required_cycles += ed_cycle_table[opcode & 0xFF];
				prefix = 0xED;
				break;
			case 0xFD:
				inc_r();
				opcode <<= 8;
				fd = 1;
				opcode |= sms->read_byte(PC++);
				if ((opcode & 0xFF) == 0xCB)
				{
					opcode <<= 8;
					opcode |= sms->read_byte(PC++);
					opcode <<= 8;
					opcode |= sms->read_byte(PC++);
					required_cycles += ddcb_cycle_table[opcode & 0xFF];
					prefix = 0xDDCB;
				}
				else
				{
					//replace HL, H, and L, with IY, IYH, and IYL
					//replace (HL) with (IY+d)
					required_cycles += dd_cycle_table[opcode & 0xFF]; //same as dd_cycle_table
					prefix = 0x00;
					r = r_fd;
					rp = rp_fd;
					rp2 = rp2_fd;
				}
				break;
			default:
				if (opcode == 0x101 || opcode == 0x103 || opcode == 0x104)
					prefix = 0x01;
				else
					prefix = 0;
				required_cycles += cycle_table[opcode];
				break;
			}
			//some instruction counts 
			dprintf("%06X %2d ", opcode, required_cycles);
			fetch_opcode = 0;
		}
		if (required_cycles <= available_cycles)
		{
			available_cycles -= required_cycles;

			fetch_opcode = 1;
			//keep track of how many cycles since psg has run
			//account for cycles that have run without retiring an opcode
			//in the previous execute call
			pending_psg_cycles += needed_cycles ? needed_cycles : required_cycles;
			needed_cycles = 0;
			required_cycles = 0;
			execute_opcode();
		}
		else
		{
			//number of cycles that have run without retiring an opcode
			needed_cycles = required_cycles - available_cycles;
			break;
		}
	}
}

void c_z80::execute_opcode()
{
	//decode per http://www.z80.info/decoding.htm
	int o = opcode & 0xFF;
	z = opcode & 0x7;
	y = (opcode >> 3) & 0x7;
	x = (opcode >> 6) & 0x3;
	q = y & 0x1;
	p = (y >> 1) & 0x3;

	if (fd && (y == 6 || z == 6))
	{
		int x = 1;
	}

	switch (prefix)
	{
	case 0x00:
		switch (x)
		{
		case 0:
			switch (z)
			{
			case 0:
				switch (y)
				{
				case 0:
					dprintf("NOP\n");
					break;
				case 1:
					dprintf("EX AF, AF'\n");
					swap_register(&AF.word, &AF2.word);
					break;
				case 2:
					d = (signed char)sms->read_byte(PC++);
					dprintf("DJNZ %d\n", PC);
					BC.byte.hi--;
					if (BC.byte.hi != 0)
					{
						//available_cycles -= 5;
						required_cycles += 5;
						PC += d;
					}
					break;
				case 3:
					d = (signed char)sms->read_byte(PC++);
					dprintf("JR %d\n", d);
					PC += d;
					break;
				case 4:
				case 5:
				case 6:
				case 7:
					d = (signed char)sms->read_byte(PC++);
					if (test_flag(y - 4))
						PC += d;
					break;
				}
				break;

			case 1:
				switch (q)
				{
				case 0:
					//LD rp[p],nn
					temp = sms->read_word(PC);
					PC += 2;
					*rp[p] = temp;
					break;
				case 1:
					//ADD HL, rp[p]
					//ADD16(&HL.word, *rp[p]);
					ADD16(rp[2], *rp[p]);
					break;
				}
				break;
			case 2:
				switch (q)
				{
				case 0:
					switch (p)
					{
					case 0:
						//LD (BC), A
						sms->write_byte(BC.word, AF.byte.hi);
						break;
					case 1:
						//LD (DE), A
						sms->write_byte(DE.word, AF.byte.hi);
						break;
					case 2:
						//LD (nn), HL
						temp = sms->read_word(PC);
						PC += 2;
						//sms->write_word(temp, HL.word);
						temp2 = *rp[2];
						sms->write_word(temp, *rp[2]);
						break;
					case 3:
						//LD (nn), A
						temp = sms->read_word(PC);
						PC += 2;
						sms->write_byte(temp, AF.byte.hi);
						break;
					}
					break;
				case 1:
					switch (p)
					{
					case 0:
						//LD A, (BC)
						AF.byte.hi = sms->read_byte(BC.word);
						break;
					case 1:
						//LD A, (DE)
						AF.byte.hi = sms->read_byte(DE.word);
						break;
					case 2:
						//LD HL, (nn)
						temp = sms->read_word(PC);
						PC += 2;
						*rp[2] = sms->read_word(temp);
						break;
					case 3:
						//LD A, (nn)
						temp = sms->read_word(PC);
						PC += 2;
						AF.byte.hi = sms->read_byte(temp);
						break;
					}
					break;
				}
				break;
			case 3:
				switch (q)
				{
				case 0:
					//INC rp[p]
					*rp[p] = (*rp[p]) + 1;
					break;
				case 1:
					//DEC rp[p]
					*rp[p] = (*rp[p]) - 1;
					break;
				}
				break;
			case 4:
				//INC r[y]
				if (y == 6)
				{
					unsigned char temp;
					if (dd)
					{
						d = (signed char)sms->read_byte(PC++);
						temp = sms->read_byte(IX.word + d);
						INC(&temp);
						sms->write_byte(IX.word + d, temp);
					}
					else if (fd)
					{
						d = (signed char)sms->read_byte(PC++);
						temp = sms->read_byte(IY.word + d);
						INC(&temp);
						sms->write_byte(IY.word + d, temp);
					}
					else
					{
						temp = sms->read_byte(*rp[2]);
						INC(&temp);
						sms->write_byte(*rp[2], temp);
					}
				}
				else
				{
					INC(r[y]);
				}
				break;
			case 5:
				//DEC r[y]
				if (y == 6)
				{
					unsigned char temp;
					if (dd)
					{
						d = (signed char)sms->read_byte(PC++);
						temp = sms->read_byte(IX.word + d);
						DEC(&temp);
						sms->write_byte(IX.word + d, temp);
					}
					else if (fd)
					{
						d = (signed char)sms->read_byte(PC++);
						temp = sms->read_byte(IY.word + d);
						DEC(&temp);
						sms->write_byte(IY.word + d, temp);
					}
					else
					{
						temp = sms->read_byte(*rp[2]);
						DEC(&temp);
						sms->write_byte(*rp[2], temp);
					}
				}
				else
				{
					DEC(r[y]);
				}
				break;
			case 6:
				//LD r[y], n

				if (y == 6)
				{
					if (dd)
					{

						d = (signed char)sms->read_byte(PC++);
						temp = sms->read_byte(PC++);
						sms->write_byte(IX.word + d, temp);
					}
					else if (fd)
					{

						d = (signed char)sms->read_byte(PC++);
						temp = sms->read_byte(PC++);
						sms->write_byte(IY.word + d, temp);
					}
					else
					{
						temp = sms->read_byte(PC++);
						sms->write_byte(*rp[2], temp);
					}
				}
				else
				{
					temp = sms->read_byte(PC++);
					*r[y] = temp;
				}
				break;
			case 7:
				switch (y)
				{
				case 0:
					//RLCA
					temp = (AF.byte.hi & 0x80) >> 7;
					AF.byte.hi <<= 1;
					AF.byte.hi |= temp;
					set_c(temp);
					set_n(0);
					set_h(0);
					break;
				case 1:
					//RRCA
					temp = AF.byte.hi & 0x1;
					set_c(temp);
					set_h(0);
					set_n(0);
					AF.byte.hi >>= 1;
					AF.byte.hi |= (temp << 7);
					break;
				case 2:
					//RLA
					temp = (AF.byte.hi & 0x80) >> 7;
					AF.byte.hi <<= 1;
					AF.byte.hi |= (AF.byte.lo & 0x1);
					set_c(temp);
					set_n(0);
					set_h(0);
					break;
				case 3:
					//RRA
					temp = AF.byte.hi & 0x1;
					AF.byte.hi >>= 1;
					AF.byte.hi |= ((AF.byte.lo & 0x1) << 7);
					set_c(temp);
					set_n(0);
					set_h(0);
					break;
				case 4:
					//DAA
				{
					int a_before = AF.byte.hi;
					int correction = 0;
					int carry = AF.byte.lo & 0x1;
					int n_flag = AF.byte.lo & 0x2;
					int half_carry = AF.byte.lo & 0x10;
					if (AF.byte.hi > 0x99 || carry)
					{
						correction = 0x60;
						set_c(1);
					}
					else
					{
						set_c(0);
					}
					if ((AF.byte.hi & 0xF) > 0x9 || half_carry)
					{
						correction |= 0x06;
					}
					if (n_flag)
					{
						AF.byte.hi -= correction;
					}
					else
					{
						AF.byte.hi += correction;
					}
					set_h((a_before ^ AF.byte.hi) & 0x10);
					set_s(AF.byte.hi & 0x80);
					set_z(AF.byte.hi == 0);
					set_pv(get_parity(AF.byte.hi));
				}
					break;
				case 5:
					//CPL
					AF.byte.hi = ~AF.byte.hi;
					set_n(1);
					set_h(1);
					break;
				case 6:
					//SCF
					set_c(1);
					set_n(0);
					set_h(0);
					break;
				case 7:
					//CCF
					set_n(0);
					set_h(AF.byte.lo & 1);
					set_c(!(AF.byte.lo & 0x1));
					break;
				}
				break;
			}
			break;
		case 1: //x=1
			if (z == 6 && y == 6)
			{
				//HALT
				halted = 1;
			}
			else
			{
				//LD r[y], r[z]
				//unsigned char src = z == 6 ? sms->read_byte(HL.word) : *r[z];
				unsigned char src = 0;

				int unmap_z = 0;
				int unmap_y = 0;

				if (dd || fd)
				{
					//if (z == 6 || y == 6)
					//{
					//	if (z == 4 || z == 5 || y == 4 || y == 5)
					//	{
					//		printf("(HL) + H/L at %x\n", PC);
					//		//exit(1);
					//	}
					//}
					if (z == 6)
					{
						unmap_y = 1;
					}
					else if (y == 6)
					{
						unmap_z = 1;
					}
				}
				if (z == 6)
				{
					if (dd)
					{
						d = (signed char)sms->read_byte(PC++);
						src = sms->read_byte(IX.word + d);
					}
					else if (fd)
					{
						d = (signed char)sms->read_byte(PC++);
						src = sms->read_byte(IY.word + d);
					}
					else
					{
						src = sms->read_byte(HL.word);
					}
				}
				else
				{
					if (unmap_z)
					{
						if (z == 4)
						{
							src = HL.byte.hi;
						}
						else if (z == 5)
						{
							src = HL.byte.lo;
						}
						else
						{
							src = *r[z];
						}
					}
					else
					{
						src = *r[z];
					}
				}

				if (y == 6) //HL
				{
					if (dd)
					{
						d = (signed char)sms->read_byte(PC++);
						sms->write_byte(IX.word + d, src);
					}
					else if (fd)
					{
						d = (signed char)sms->read_byte(PC++);
						sms->write_byte(IY.word + d, src);
					}
					else
					{
						sms->write_byte(*rp[2], src);
					}
				}
				else
				{
					if (unmap_y)
					{
						if (y == 4)
						{
							HL.byte.hi = src;
						}
						else if (y == 5)
						{
							HL.byte.lo = src;
						}
						else
						{
							*r[y] = src;
						}
					}
					else
					{
						*r[y] = src;
					}

				}
			}
			break;
		case 2: //x=2
			//alu[y] r[z]
			if (z == 6)
			{
				if (dd)
				{
					d = (signed char)sms->read_byte(PC++);
					alu(y, sms->read_byte(IX.word + d));
				}
				else if (fd)
				{
					d = (signed char)sms->read_byte(PC++);
					alu(y, sms->read_byte(IY.word + d));
				}
				else
				{
					alu(y, sms->read_byte(*rp[2]));
				}
			}
			else
			{
				alu(y, *r[z]);
			}
			break;
		case 3: //x=3
			switch (z)
			{
			case 0:
				//RET cc[y]
				if (test_flag(y))
					PC = pull_word();
				break;
			case 1:
				switch (q)
				{
				case 0:
					//POP rp2[p]
					*rp2[p] = pull_word();
					break;
				case 1:
					switch (p)
					{
					case 0:
						//RET
						PC = pull_word();
						break;
					case 1:
						//EXX
						swap_register(&BC.word, &BC2.word);
						swap_register(&DE.word, &DE2.word);
						swap_register(&HL.word, &HL2.word);
						//todo: should this be affected by dd?
						if (dd || fd)
						{
							int x = 1;
						}
						break;
					case 2:
						//JP HL
						//PC = sms->read_word(*rp[2]);
						PC = *rp[2];
						if (dd || fd)
						{
							int x = 1;
						}
						break;
					case 3:
						//LD SP, HL
						SP = *rp[2];
						break;
					}
					break;
				}
				break;
			case 2:
				//JP cc[y], nn
				temp = sms->read_word(PC);
				PC += 2;
				if (test_flag(y))
					PC = temp;
				break;
			case 3:
				switch (y)
				{
				case 0:
					//JP nn
					temp = sms->read_word(PC);
					PC += 2;
					PC = temp;
					break;
				case 1:
					//(CB)
					printf("How'd I reach CB?\n");
					//getchar();
					//exit(1);
					break;
				case 2:
					//OUT (n), A
					temp = sms->read_byte(PC++);
					sms->write_port(temp, AF.byte.hi);
					break;
				case 3:
					//IN A, (n)
					temp = sms->read_byte(PC++);
					AF.byte.hi = sms->read_port(temp);
					break;
				case 4:
					//EX (SP), HL
				{

					if (dd || fd)
					{
						int x = 1;
					}
					int sp0 = sms->read_byte(SP);
					int sp1 = sms->read_byte(SP + 1);
					sms->write_byte(SP, *r[5]);
					sms->write_byte(SP + 1, *r[4]);
					*r[5] = sp0;
					*r[4] = sp1;
				}
				break;
				case 5:
					//EX DE, HL
					if (fd || dd)
					{
						int x = 1;
					}
					temp = DE.word;
					DE.word = *rp[2];
					*rp[2] = temp;
					break;
				case 6:
					//DI
					dprintf("DI\n");
					//printf("DI\n");
					IFF1 = IFF2 = 0;
					pending_ei = 0;
					break;
				case 7:
					//EI
					//printf("EI at %x\n", PC);
					//printf("EI\n");
					pending_ei = 1;
					//IFF1 = IFF2 = 1;
					break;
				}
				break;
			case 4:
				//CALL cc[y], nn
				temp = sms->read_word(PC);
				PC += 2;
				if (test_flag(y))
				{
					push_word(PC);
					//available_cycles -= 7;
					required_cycles += 7;
					PC = temp;
				}
				break;
			case 5:
				switch (q)
				{
				case 0:
					//PUSH rp2[p]
					push_word(*rp2[p]);
					break;
				case 1:
					switch (p)
					{
					case 0:
						//CALL nn
						temp = sms->read_word(PC);
						PC += 2;
						push_word(PC);
						PC = temp;
						break;
					case 1:
						//DD
						printf("How'd I reach DD?\n");
						//getchar();
						//exit(1);
						break;
					case 2:
						//ED
						printf("How'd I reach ED?\n");
						//getchar();
						//exit(1);
						break;
					case 3:
						//FD
						printf("How'd I reach FD?\n");
						//getchar();
						//exit(1);
						break;
					}
					break;
				}
				break;
			case 6:
				//alu[y], n
				alu(y, sms->read_byte(PC++));
				break;
			case 7:
				//RST y*8
				push_word(PC);
				PC = y * 8;
				break;
			}
			break;
		}
		break; //end of prefix 0x00

	case 0x01:
		switch (z)
		{
		case 1:
			push_word(PC);
			PC = 0x38;
			break;
		case 3: //halt
		{
			int x = 1;
		}
			break;
		case 4: //nmi
			push_word(PC);
			PC = 0x66;
			break;
		default:
			//printf("invalid interrupt\n");
			//exit(1);
			break;
		}
		break;

	case 0xCB:
		if (z == 6)
		{
			if (dd || fd)
			{
				int x = 1;
			}
			tchar = sms->read_byte(HL.word);
		}
		else
		{
			tchar = *r[z];
		}

		switch (x)
		{
		case 0:
			//rot[y] r[z]
			ROT(y, &tchar);
			break;
		case 1:
			//BIT y, r[z]
			BIT(y, tchar);
			break;
		case 2:
			//RES y, r[z]
			tchar = tchar & (~(1 << y));
			break;
		case 3:
			tchar = tchar | (1 << y);
			break;
		}
		if (z == 6)
		{
			if (dd || fd)
			{
				int x = 1;
			}
			sms->write_byte(HL.word, tchar);
		}
		else
		{
			//todo: is this correct?
			*r[z] = tchar;
		}
		break;

	case 0xDD:
		//ugh
		//printf("DD\n");
		//exit(1);
	{
		int x = 1;
	}
		break;

	case 0xED:
		switch (x)
		{
		case 0:
		case 3:
			//NOP
			break;
		case 1:
			switch (z)
			{
			case 0:
				if (y == 6)
				{
					//IN (C)
					temp = sms->read_port(BC.byte.lo);
					set_n(0);
					set_pv(get_parity(temp));
					set_h(0);
					set_z(temp == 0);
					set_s(temp & 0x80);
				}
				else
				{
					if (dd || fd)
					{
						int x = 1;
					}
					//IN r[y], (C)
					*r[y] = sms->read_port(BC.byte.lo);
					set_n(0);
					set_pv(get_parity(*r[y]));
					set_h(0);
					set_z(*r[y] == 0);
					set_s(*r[y] & 0x80);
				}
				break;
			case 1:
				if (y == 6)
				{
					//OUT (C), 0
					sms->write_port(BC.byte.lo, 0);
				}
				else
				{
					if (dd || fd)
					{
						int x = 1;
					}
					//OUT (C), r[y]
					sms->write_port(BC.byte.lo, *r[y]);
				}
				break;
			case 2:
				switch (q)
				{
				case 0:
					//SBC HL, rp[p]
					//printf("SBC16\n");
					SBC16(*rp[p]);
					//exit(1);
					break;
				case 1:
					//ADC HL, rp[p]
					//printf("ADC16\n");
					ADC16(*rp[p]);
					break;
				}
				break;
			case 3:
				switch (q)
				{
				case 0:
					//LD (nn), rp[p]
					temp = sms->read_word(PC);
					PC += 2;
					sms->write_word(temp, *rp[p]);
					break;
				case 1:
					//LD rp[p], (nn)
					temp = sms->read_word(PC);
					PC += 2;
					*rp[p] = sms->read_word(temp);
					break;
				}
				break;
			case 4:
				//NEG
				NEG();
				break;
			case 5:
				if (y == 1)
				{
					//RETI
					//printf("RETI\n");
					PC = pull_word();
					//exit(1);
				}
				else
				{
					//RETN
					PC = pull_word();
					IFF1 = IFF2;
				}
				break;
			case 6:
				//IM im[y]
				switch (y & 0x3)
				{
				case 0:
					IM = 0;
					break;
				case 1:
					IM = 0;
					//printf("Undefined interrupt mode\n");
					//exit(1);
					break;
				case 2:
					IM = 1;
					break;
				case 3:
					IM = 2;
					break;
				}
				break;
			case 7:
				switch (y)
				{
				case 0:
					//LD I, A
					I = AF.byte.hi;
					break;
				case 1:
					//LD R, A
					R = AF.byte.hi;
					printf("LD R, A\n");
					break;
				case 2:
					//LD A, I
					AF.byte.hi = I;
					break;
				case 3:
					//LD A, R
					AF.byte.hi = R;
					break;
				case 4:
					//RRD
					//printf("RRD\n");
					//exit(1);
				{
					int x = sms->read_byte(HL.word);
					int x_lo = x & 0xF;
					x >>= 4;
					x |= ((AF.byte.hi & 0xF) << 4);
					sms->write_byte(HL.word, x);
					AF.byte.hi &= 0xF0;
					AF.byte.hi |= x_lo;
					set_s(AF.byte.hi & 0x80);
					set_z(AF.byte.hi == 0);
					set_h(0);
					set_pv(get_parity(AF.byte.hi));
					set_n(0);
				}
					break;
				case 5:
					//RLD
					//printf("RLD\n");
					//exit(1);
				{
					int x = sms->read_byte(HL.word);
					int x_hi = x & 0xF0;
					x <<= 4;
					x |= (AF.byte.hi & 0xF);
					sms->write_byte(HL.word, x);
					AF.byte.hi &= 0xF0;
					AF.byte.hi |= (x_hi >> 4);
					set_s(AF.byte.hi & 0x80);
					set_z(AF.byte.hi == 0);
					set_h(0);
					set_pv(get_parity(AF.byte.hi));
					set_n(0);
				}
					break;
				case 6:
				case 7:
					//NOP
					break;
				}
				break;
			}
			break;
		case 2:
			if (z <= 3 && y >= 4)
			{
				//BLI[y,z]
				switch (y)
				{
				case 4:
					switch (z)
					{
					case 0:
						//LDI
						sms->write_byte(DE.word, sms->read_byte(HL.word));
						HL.word++;
						DE.word++;
						BC.word--;
						set_n(0);
						set_h(0);
						set_pv(BC.word != 0);
						break;
					case 1:
						//CPI
					{
						unsigned short s = sms->read_byte(HL.word);
						int z = AF.byte.hi == s;
						int c = AF.byte.lo & 0x1;
						CP(s);
						HL.word++;
						BC.word--;
						set_z(z);
						set_c(c);
						set_pv(BC.word != 0);
					}
					break;
					case 2:
						//INI
						sms->write_byte(HL.word, sms->read_port(BC.byte.lo));
						HL.word++;
						BC.byte.hi--;
						set_n(0);
						set_z(BC.byte.hi == 0);
						break;
					case 3:
						//OUTI
						sms->write_port(BC.byte.lo, sms->read_byte(HL.word));
						HL.word++;
						BC.byte.hi--;
						set_n(1);
						set_z(BC.byte.hi == 0);
						break;
					}
					break;
				case 5:
					switch (z)
					{
					case 0:
						//LDD
						sms->write_byte(DE.word, sms->read_byte(HL.word));
						HL.word--;
						DE.word--;
						BC.word--;
						set_n(0);
						set_h(0); //todo: check
						set_pv(BC.word != 0);
						break;
					case 1:
						//CPD
					{
						unsigned short s = sms->read_byte(HL.word);
						int z = AF.byte.hi == s;
						int c = AF.byte.lo & 0x1;
						CP(s);
						HL.word--;
						BC.word--;
						set_z(z);
						set_c(c);
						set_pv(BC.word != 0);
					}
					break;
					case 2:
						//IND
						sms->write_byte(HL.word, sms->read_port(BC.byte.lo));
						HL.word--;
						BC.byte.hi--;
						set_n(1);
						set_z(BC.byte.hi == 0);
						break;
					case 3:
						//OUTD
						sms->write_port(BC.byte.lo, sms->read_byte(HL.word));
						HL.word--;
						BC.byte.hi--;
						set_n(1);
						set_z(BC.byte.hi == 0);
						break;
					}
					break;
				case 6:
					switch (z)
					{
					case 0:
						//LDIR
						sms->write_byte(DE.word, sms->read_byte(HL.word));
						HL.word++;
						DE.word++;
						BC.word--;
						if (BC.word != 0)
						{
							//available_cycles -= 5;
							required_cycles += 5;
							PC -= 2;
							set_pv(1);
						}
						else
						{
							set_pv(0);
						}
						set_n(0);

						set_h(0);
						break;
					case 1:
						//CPIR
						if (dd || fd)
						{
							//printf("CPIR dd/fd\n");
							//exit(1);
						}
						tchar = sms->read_byte(HL.word);
						CP(tchar);
						HL.word++;
						BC.word--;
						if (BC.word != 0 && AF.byte.hi != tchar)
						{
							PC -= 2;
							//available_cycles -= 5;
							required_cycles += 5;
							set_pv(1);
						}
						else
						{
							set_pv(0);
						}
						break;
					case 2:
						//INIR
						sms->write_byte(HL.word, sms->read_port(BC.byte.lo));
						HL.word++;
						BC.byte.hi--;
						set_z(BC.byte.hi == 0);
						set_n(1);
						if (BC.byte.hi != 0)
						{
							PC -= 2;
							//available_cycles -= 5;
							required_cycles += 5;
						}
						break;
					case 3:
						//OTIR
						sms->write_port(BC.byte.lo, sms->read_byte(HL.word));
						HL.word++;
						BC.byte.hi--;
						if (BC.byte.hi != 0)
						{
							//available_cycles -= 5;
							required_cycles += 5;
							PC -= 2;
						}
						set_n(1);
						set_z(BC.byte.hi == 0);
						//set_z(1);
						break;
					}
					break;
				case 7:
					switch (z)
					{
					case 0:
						//LDDR
						if (dd || fd)
						{
							int x = 1;
						}
						sms->write_byte(DE.word, sms->read_byte(HL.word));
						DE.word--;
						HL.word--;
						BC.word--;
						if (BC.word != 0)
						{
							//available_cycles -= 5;
							required_cycles += 5;
							PC -= 2;
							set_pv(1);
						}
						else
						{
							set_pv(0);
						}
						set_n(0);
						set_h(0);
						break;
					case 1:
						//CPDR
						if (dd || fd)
						{
							//printf("CPDR dd/fd\n");
							//exit(1);
						}
						tchar = sms->read_byte(HL.word);
						CP(tchar);
						HL.word--;
						BC.word--;
						if (BC.word != 0 && AF.byte.hi != tchar)
						{
							PC -= 2;
							//available_cycles -= 5;
							required_cycles += 5;
							set_pv(1);
						}
						else
						{
							set_pv(0);
						}

						break;
					case 2:
						//INDR
						sms->write_byte(HL.word, BC.byte.lo);
						HL.word--;
						BC.byte.hi--;
						set_z(BC.byte.hi == 0);
						set_n(1);
						if (BC.byte.hi != 0)
						{
							PC -= 2;
							//available_cycles -= 5;
							required_cycles += 5;
						}
						break;
					case 3:
						//OTDR
						sms->write_port(BC.byte.lo, sms->read_byte(HL.word));
						HL.word--;
						BC.byte.hi--;
						set_z(BC.byte.hi == 0);
						set_n(1);
						if (BC.byte.hi != 0)
						{
							PC -= 2;
							//available_cycles -= 5;
							required_cycles += 5;
						}
						break;
					}
					break;
				}
			}
			else
			{
				//NOP
				//printf("NOP\n");
				//getchar();
				//exit(1);
			}
			break;
		}
		break;

	case 0xDDCB:
		d = (signed char)((opcode >> 8) & 0xFF);
		switch (x)
		{
		case 0:
			if (z != 6)
			{
				//LD r[z], rot[y] (IX+d)
				printf("LD r[z], rot[y] (IX+d)\n");
				//exit(1);
			}
			else
			{
				//rot[y] (IX+d)
				temp = dd ? IX.word : IY.word;
				tchar = sms->read_byte(temp + d);
				ROT(y, &tchar);
				sms->write_byte(temp + d, tchar);
			}
			break;
		case 1:
			//BIT y, r[z]
			temp = dd ? IX.word : IY.word;
			temp2 = sms->read_byte(temp + d);
			BIT(y, temp2);
			break;
		case 2:
			if (z != 6)
			{
				//LD r[z], RES y, (IX+d)
				printf("LD r[z], RES y, (IX+d)\n");
				//exit(1);
			}
			else
			{
				//RES y, (IX+d)
				temp = dd ? IX.word : IY.word;
				temp2 = sms->read_byte(temp + d);
				temp2 &= ~(1 << y);
				sms->write_byte(temp + d, temp2);
			}
			break;
		case 3:
			if (z != 6)
			{
				//LD r[z], SET y, (IX+d)
				printf("LD r[z], SET y, (IX+d)\n");
				//exit(1);
			}
			else
			{
				temp = dd ? IX.word : IY.word;
				tchar = sms->read_byte(temp + d);
				tchar = tchar | (1 << y);
				sms->write_byte(temp + d, tchar);
			}
			break;
		default:
			break;
		}
		break;

	default:
		printf("crap\n");
		//exit(1);
		break;

	}

	dprintf(" PC: %04X SP: %04X I: %02X R: %02X\n", PC, SP, I, R);
	//dprintf(" A: %02X B: %02X C: %02X D: %02X E: %02X F: %02X H: %02X L: %02X I: %02X R: %02X IXH: %02X IXL: %02X IYH: %02X IYL: %02X\n",
	//	AF.byte.hi, BC.byte.hi, BC.byte.lo, DE.byte.hi, DE.byte.lo, AF.byte.lo, HL.byte.hi, HL.byte.lo, I, R, IX.byte.hi, IX.byte.lo,
	//	IY.byte.hi, IY.byte.lo);
	dprintf(" AF: %04X BC: %04X DE: %04X HL: %04X IX: %04X IY: %04X\n", AF.word, BC.word, DE.word, HL.word, IX.word, IY.word);
	dprintf("\n");
}

void c_z80::push_word(unsigned short value)
{
	SP -= 2;
	sms->write_word(SP, value);
}

unsigned short c_z80::pull_word()
{
	unsigned short ret = sms->read_word(SP);
	SP += 2;
	return ret;
}


void c_z80::set_s(int set)
{
	int bit = 1 << 7;
	if (set)
		AF.byte.lo |= bit;
	else
		AF.byte.lo &= (~bit);
}
void c_z80::set_z(int set)
{
	int bit = 1 << 6;
	if (set)
		AF.byte.lo |= bit;
	else
		AF.byte.lo &= (~bit);
}
void c_z80::set_h(int set)
{
	int bit = 1 << 4;
	if (set)
		AF.byte.lo |= bit;
	else
		AF.byte.lo &= (~bit);
}
void c_z80::set_pv(int set)
{
	int bit = 1 << 2;
	if (set)
		AF.byte.lo |= bit;
	else
		AF.byte.lo &= (~bit);
}
void c_z80::set_n(int set)
{
	int bit = 1 << 1;
	if (set)
		AF.byte.lo |= bit;
	else
		AF.byte.lo &= (~bit);
}
void c_z80::set_c(int set)
{
	int bit = 1 << 0;
	if (set)
		AF.byte.lo |= bit;
	else
		AF.byte.lo &= (~bit);
}

int c_z80::get_parity(unsigned char value)
{
	int sum = 0;
	for (int i = 0; i < 8; i++)
	{
		sum += value & 0x1;
		value >>= 1;
	}
	return !(sum & 0x1);
}

void c_z80::OR(unsigned char operand)
{
	AF.byte.hi |= operand;
	set_c(0);
	set_n(0);
	set_pv(get_parity(AF.byte.hi));
	set_h(0);
	set_z(AF.byte.hi == 0);
	set_s(AF.byte.hi & 0x80);
}

void c_z80::INC(unsigned char *operand)
{
	int o = *operand;
	int result = o + 1;
	set_pv(o == 0x7F);
	set_s(result & 0x80);
	set_z((result & 0xFF) == 0);
	set_h((o & 0xF) + 1 > 0xF);
	set_n(0);
	*operand = result & 0xFF;
}

void c_z80::DEC(unsigned char *operand)
{
	temp = 1;
	unsigned short result = *operand - temp;
	set_s(result & 0x80);
	set_z((result & 0xFF) == 0);
	set_h(((unsigned char)temp & 0xF) > (*operand & 0xF));
	set_pv((*operand ^ result) & (temp ^ *operand) & 0x80);
	signed char s = *operand;
	signed char s1 = *operand - 1;
	//set_pv((s ^ s1) & 0x80);
	set_n(1);
	*operand = result & 0xFF;
}

void c_z80::ROT(int y, unsigned char *operand)
{
	int temp3;
	switch (y)
	{
	case 0: //RLC
		RLC(operand);
		break;
	case 1: //RRC
		tchar = *operand;
		set_c(tchar & 0x1);
		tchar >>= 1;
		tchar |= ((AF.byte.lo & 0x1) << 7);
		set_s(tchar & 0x80);
		set_z(tchar == 0);
		set_h(0);
		set_pv(get_parity(tchar));
		set_n(0);
		*operand = tchar;
		break;
	case 2: //RL
		RL(operand);
		break;
	case 3: //RR
		RR(operand);
		break;
	case 4: //SLA
		tchar = *operand;
		set_c(tchar & 0x80);
		tchar <<= 1;
		set_s(tchar & 0x80);
		set_z(tchar == 0);
		set_h(0);
		set_pv(get_parity(tchar));
		set_n(0);
		*operand = tchar;
		break;
	case 5: //SRA
		tchar = *operand;
		temp3 = tchar & 0x80;
		set_c(tchar & 0x1);
		tchar >>= 1;
		tchar |= temp3;
		set_s(tchar & 0x80);
		set_z(tchar == 0);
		set_h(0);
		set_pv(get_parity(tchar));
		set_n(0);
		*operand = tchar;
		break;
	case 6: //SLL
		tchar = *operand;
		set_c(tchar & 0x80);
		tchar <<= 1;
		tchar |= 0x1;
		set_s(tchar & 0x80);
		set_z(tchar == 0);
		set_h(0);
		set_pv(get_parity(tchar));
		set_n(0);
		*operand = tchar;
		break;
	case 7: //SRL
		tchar = *operand;
		set_c(tchar & 0x1);
		tchar >>= 1;
		set_s(tchar & 0x80);
		set_z(tchar == 0);
		set_h(0);
		set_pv(get_parity(tchar));
		set_n(0);
		*operand = tchar;
		break;
	default:
		break;
	}
}

void c_z80::RST(unsigned char dest)
{
	push_word(PC);
	PC = dest;
}

void c_z80::swap_register(unsigned short *first, unsigned short *second)
{
	unsigned short temp = *first;
	*first = *second;
	*second = temp;
}

void c_z80::RLC(unsigned char *operand)
{
	int carry = (*operand & 0x80) >> 7;
	set_c(carry);
	set_n(0);
	set_h(0);
	*operand <<= 1;
	*operand |= carry;
	set_pv(get_parity(*operand));
	set_z(*operand == 0);
	set_s(*operand & 0x80);
}

void c_z80::RL(unsigned char *operand)
{
	int carry = (*operand & 0x80);
	*operand <<= 1;
	if (AF.byte.lo & 0x1)
		*operand |= 0x1;
	set_c(carry);
	set_n(0);
	set_h(0);
	set_pv(get_parity(*operand));
	set_z(*operand == 0);
	set_s(*operand & 0x80);
}

void c_z80::RR(unsigned char *operand)
{
	int carry = AF.byte.lo & 0x1;
	set_c(*operand & 0x1);
	*operand >>= 1;
	if (carry)
		*operand |= 0x80;
	set_n(0);
	set_h(0);
	set_pv(get_parity(*operand));
	set_z(*operand == 0);
	set_s(*operand & 0x80);
}

void c_z80::BIT(int bit, unsigned char operand)
{
	set_n(0);
	set_h(1);
	set_z((operand & (1 << bit)) == 0);
	set_pv(AF.byte.lo & 0x40);
	set_s(bit == 7 ? !(AF.byte.lo & 0x40) : 0);

}

void c_z80::ADD8(unsigned char operand, int carry)
{
	//unsigned short result = AF.byte.hi + operand + carry;
	//signed char c_result = AF.byte.hi + operand + carry;
	//set_c(result > 0xFF);
	//set_s(result & 0x80);
	//set_z(result == 0);
	//set_h((AF.byte.hi & 0xF) + (operand & 0xF) > 0xF);
	//set_pv((AF.byte.hi ^ c_result) & (operand ^ c_result) & 0x80);
	//set_n(0);
	//AF.byte.hi = result & 0xFF;
	int c = operand + carry;
	int result = AF.byte.hi + operand + carry;
	unsigned int uresult = (unsigned int) result;
	//set_pv(result < -128 || result > 127);
	set_pv((AF.byte.hi ^ result) & (operand ^ result) & 0x80);
	//set_pv((AF.byte.hi ^ result) & (operand ^ result) & 0x80);
	set_c(uresult < 0 || uresult > 255);
	set_s(result & 0x80);
	set_n(0);
	set_h(((AF.byte.hi & 0xF) + (operand & 0xF) + (carry & 0xF)) > 0xF);
	AF.byte.hi = uresult & 0xFF;
	set_z(AF.byte.hi == 0);
}

void c_z80::ADC8(unsigned char operand)
{
	ADD8(operand, AF.byte.lo & 0x1);
}

void c_z80::AND(unsigned char operand)
{
	AF.byte.hi = AF.byte.hi & operand;
	set_c(0);
	set_n(0);
	set_pv(get_parity(AF.byte.hi));
	set_h(1);
	set_z(AF.byte.hi == 0);
	set_s(AF.byte.hi & 0x80);
}

void c_z80::CP(unsigned char operand)
{
	//unsigned short result = AF.byte.hi - operand;
	//signed char c_result = AF.byte.hi - operand;
	//set_s(result & 0x80);
	//set_z(result == 0);
	//set_h(((unsigned char)operand & 0xF) > (AF.byte.hi & 0xF));
	//set_pv((AF.byte.hi ^ c_result) & (operand ^ AF.byte.hi) & 0x80);
	//set_n(1);
	//set_c(operand > AF.byte.hi);

	int c = operand;
	int result = (int)AF.byte.hi - (int)c;
	unsigned int uresult = (unsigned int)result;
	set_pv((AF.byte.hi ^ result) & (operand ^ AF.byte.hi) & 0x80);
	set_c(uresult < 0 || uresult > 255);
	set_n(1);
	set_z((result & 0xFF) == 0);
	set_s(result & 0x80);
	set_h((AF.byte.hi & 0xF) - (c & 0xF) < 0);
	//AF.byte.hi = uresult & 0xFF;
}

void c_z80::SBC16(unsigned short operand)
{
	//int x = operand + (AF.byte.lo & 1);
	//set_h((x & 0xFFF) > (HL.word & 0xFFF));
	//set_c(x > HL.word);
	//set_n(1);
	////TODO:handle p/v flag

	//HL.word -= x;
	//set_s(HL.word & 0x8000);
	//set_z(HL.word == 0);

	if (HL.word != 1 || operand != 1)
	{
		int xdsfsd = 1;
	}
	int x = operand + (AF.byte.lo & 1);
	int result = HL.word - x;
	unsigned int uresult = (unsigned int)result;
	set_h(((HL.word & 0xFFF) - (x & 0xFFF) - (AF.byte.lo & 0x1)) < 0);
	//set_pv(result < -32768 || result > 32767);
	set_pv((HL.word ^ result) & (x ^ HL.word) & 0x8000);
	set_c(result < 0);
	set_n(1);
	//TODO:handle p/v flag
	set_s(result & 0x8000);
	HL.word = result & 0xFFFF;
	set_z(HL.word == 0);
	if (result != 0)
	{
		int skjdhfkds = 1;
	}
}

void c_z80::SUB8(unsigned char operand, int carry)
{

	int c = operand + carry;
	int result = (int)AF.byte.hi - (int)c;
	unsigned int uresult = (unsigned int)result;
	set_pv((AF.byte.hi ^ result) & (operand ^ AF.byte.hi) & 0x80);
	set_c(result < 0);
	set_n(1);

	set_s(result & 0x80);
	set_h(((AF.byte.hi & 0xF) - (operand & 0xF) - (carry & 0xF)) < 0);
	AF.byte.hi = result & 0xFF;
	set_z(AF.byte.hi == 0);

}

void c_z80::NEG()
{
	unsigned short temp = 0 - AF.byte.hi;
	signed char s_temp = 0 - AF.byte.hi;
	set_c(AF.byte.hi > 0);
	set_h((AF.byte.hi & 0xF) > 0);
	set_pv((0 ^ s_temp) & (0 ^ AF.byte.hi) & 0x80);
	AF.byte.hi = temp & 0xFF;
	set_n(1);
	set_z(AF.byte.hi == 0);
	set_s(AF.byte.hi & 0x80);

}

void c_z80::XOR(unsigned char operand)
{
	AF.byte.hi ^= operand;
	set_c(0);
	set_n(0);
	set_h(0);
	set_z(AF.byte.hi == 0);
	set_s(AF.byte.hi & 0x80);
	set_pv(get_parity(AF.byte.hi));
}

void c_z80::ADD16(unsigned short *dest, unsigned short operand)
{
	set_h((*dest & 0xFFF) + (operand & 0xFFF) > 0xFFF);
	int result = *dest + operand;
	set_c(result > 0xFFFF);
	set_n(0);
	*dest = result;
}

void c_z80::ADC16(unsigned short operand)
{
	int x = operand + (AF.byte.lo & 1);
	int result = HL.word + x;
	set_h(((operand & 0xFFF) + (HL.word & 0xFFF) + (AF.byte.lo & 0x1)) > 0xFFF);
	set_c((HL.word + x) > 0xFFFF);
	set_n(0);
	//TODO:handle p/v flag
	//set_pv(result < -32768 || result > 32767);
	set_pv((HL.word ^ result) & (operand ^ result) & 0x8000);
	set_s(result & 0x8000);
	HL.word = result & 0xFFFF;
	set_z(HL.word == 0);
}


void c_z80::alu(int op, unsigned char operand)
{
	switch (op)
	{
	case 0: //ADD
		ADD8(operand);
		break;
	case 1: //ADC
		ADD8(operand, AF.byte.lo & 0x1);
		break;
	case 2: //SUB
		SUB8(operand);
		break;
	case 3: //SBC
		SUB8(operand, AF.byte.lo & 0x1);
		break;
	case 4: //AND
		AND(operand);
		break;
	case 5: //XOR
		XOR(operand);
		break;
	case 6: //OR
		OR(operand);
		break;
	case 7: //CP
		CP(operand);
		break;
	}
}

int c_z80::test_flag(int f)
{
	int t = AF.byte.lo & cc[f];
	//if bit 0 of c is set, check for positive condition
	if (f & 1)
	{
		if (t)
			return 1;
	}
	else
	{
		if (!t)
			return 1;
	}
	return 0;
}