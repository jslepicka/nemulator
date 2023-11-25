#pragma once
#include <cstdint>
#include <functional>

class c_gb;

class c_lr35902
{
public:
	c_lr35902(c_gb* gb);
	void reset(uint16_t PC = 0x100);
	void execute(int cycles);

private:
	void execute_opcode();

	uint8_t o8;
	uint16_t o16;
	int o;

	int instruction_offset;

	void ADD8(uint8_t d, int carry = 0);
	void SUB8(uint8_t d, int carry = 0);
	void SWAP(uint8_t& reg);
	void ADD16(uint16_t& reg, uint16_t d);
	void ADD16r(uint16_t& reg, int8_t d);

	void ADC8(uint8_t d);
	void SBC8(uint8_t d);
	void DEC(uint8_t& reg);
	void INC(uint8_t& reg);
	void LD(uint8_t& reg, uint8_t d);
	void LD(uint16_t& reg, uint16_t d);
	void LD(uint16_t address, uint8_t d);
	void CP(uint8_t d);

	void AND(uint8_t d);
	void OR(uint8_t d);
	void XOR(uint8_t d);

	void SRL(uint8_t& reg);
	void RR(uint8_t& reg);
	void RL(uint8_t& reg);
	void RLC(uint8_t& reg);
	void RRC(uint8_t& reg);
	void SLA(uint8_t& reg);
	void SRA(uint8_t& reg);
	void BIT(int bit, uint8_t d);
	void RES(int bit, uint8_t& d);
	void SET(int bit, uint8_t& d);

	void CALL(uint16_t addr);
	void DAA();
    void STOP();

	void push_word(uint16_t d);
	uint16_t pop_word();

	void update_f();
	void update_flags();

	c_gb* gb;
	int available_cycles;
	int required_cycles;
	int fetch_opcode;
	int opcode;
	int ins_start;
	int IME;
	int pending_ei;
	int prefix;
	int instruction_len;

	uint16_t PC;
	uint16_t SP;

	union
	{
		struct
		{
			unsigned char l;
			unsigned char h;
		} b;
		unsigned short w;
	} AF, BC, DE, HL;

	//flags;
	int fz, fn, fh, fc;

	struct s_ins
	{
		uint8_t len;
		uint8_t cycles;
	};

	int halted;

	static const s_ins ins_info[517];
};

