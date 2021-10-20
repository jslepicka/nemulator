#pragma once
#include "..\mapper.h"

class c_mapper4 :
	public c_mapper
{
public:
	c_mapper4(void);
	~c_mapper4(void);
	unsigned char ppu_read(unsigned short address);
	void ppu_write(unsigned short address, unsigned char value);
	virtual void WriteByte(unsigned short address, unsigned char value);
	virtual unsigned char ReadByte(unsigned short address);
	virtual void reset(void);
	void mmc3_check_a12();
	virtual void clock(int cycles);
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
	virtual void Sync(void);

	int prg_offset;
	int chr_offset;
	int prg_xor;
	int chr_xor;

	int prg_mask;
	int chr_mask;

	int last_prg_page;
	int four_screen;
};