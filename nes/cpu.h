#pragma once
class c_nes;

class c_cpu
{
public:
	c_cpu();
	~c_cpu();
	int reset();
	void execute_nmi();
	void execute_irq();
	int available_cycles;
	void do_sprite_dma(unsigned char* dst, int source_address);
	void execute_apu_dma();
	c_nes* nes;
	void clear_irq();
	int executed_cycles;
	void clear_nmi();
	void execute();
	int odd_cycle;
	void add_cycle();

private:
    int cycles;
	unsigned char A;				//Accumulator
	unsigned char X;				//X Index Register
	unsigned char Y;				//Y Index Register
	struct {
		bool C : 1;		//Carry
		bool Z : 1;		//Zero
		bool I : 1;		//Interrupt enable
		bool D : 1;		//Decimal
		bool B : 1;		//BRK
		bool Unused : 1;
		bool V : 1;		//Overflow
		bool N : 1;		//Sign
	} SR;
	unsigned char* S;	//Status Register
	unsigned char SP;			//Stack Pointer
	unsigned char M;				//Memory
	unsigned short PC;			//Program Counter
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
	unsigned char* dma_dst;
	int dma_src;

	int opcode;
	void execute_opcode();
	

	static const int cycle_table[260];

	void branch(int Condition);

	//Addressing modes
	void immediate();
	void zeropage();
	void zeropage_x();
	void zeropage_y();
	void absolute();
	void absolute_x();
	void absolute_y();
	void indirect();
	void indirect_x();
	void indirect_y();

	//with page crossing check
	void absolute_x_pc();
	void absolute_y_pc();
	void indirect_y_pc();

	//for rmw instructions
	void absolute_x_rmw();

	//load effective address for store ops
	void zeropage_ea();
	void zeropage_x_ea();
	void zeropage_y_ea();
	void absolute_ea();
	void absolute_x_ea();
	void absolute_y_ea();
	void indirect_x_ea();
	void indirect_y_ea();

	//Group one instructions
	void ADC();
	void AND();
	void CMP();
	void EOR();
	void LDA();
	void ORA();
	void SBC();
	void STA();

	//Group two instructions
	void LSR();
	void LSR_R(unsigned char& reg);
	void ASL();
	void ASL_R(unsigned char& reg);
	void ROL();
	void ROL_R(unsigned char& reg);
	void ROR();
	void ROR_R(unsigned char& reg);
	void INC();
	void DEC();
	void LDX();
	void STX();
	void TAX();
	void DEX();
	void DEY();
	void TXS();
	void TSX();

	//Group three instructions
	void LDY();
	void STY();
	void CPY();
	void CPX();
	void INX();
	void INY();
	void BCC();
	void BCS();
	void BEQ();
	void BMI();
	void BNE();
	void BPL();
	void BVC();
	void BVS();
	void CLC();
	void SEC();
	void CLD();
	void SED();
	void CLI();
	void SEI();
	void CLV();
	void BRK();
	void JSR();
	void PHA();
	void PHP();
	void PLA();
	void PLP();
	void JMP();
	void BIT();
	void RTI();
	void TAY();
	void TYA();
	void TXA();
	void RTS();
	void SKB();

	//illegal
	void ALR();
	void ANC();
	void ARR();
	void AXS();
	void DCP();
	void ISC();
	void LAX();
	void RLA();
	void RRA();
	void SAX();
	void SHY();
	void SHX();
	void SLO();
	void SRE();
	void STP();

	//untested
	void LAS();
	void SHA();
	void TAS();
	void XAA();

	int irq_checked();
};