#pragma once
class c_nes;

class c_cpu
{

public:
	c_cpu(void);
	~c_cpu(void);
	int reset(void);
	void execute_cycles(int numPpuCycles);
	void execute3();
	void execute_nmi();
	void execute_irq(void);
	int availableCycles;
	void DoSpriteDMA(unsigned char *dst, int source_address);
	c_nes *nes;
	void clear_irq();
	void ExecuteApuDMA();
	int executed_cycles;
	void clear_nmi();
	void execute();

private:
	bool check_page_cross;
	int nmi_delay;
	int irq_delay;
	int doApuDMA;
	bool doIrq;
	bool doNmi;
	bool irqPending;
	bool nmiPending;
	int dmaPos;
	unsigned char *dmaDst;
	int dmaSrc;

	int opcode;
	int requiredCycles;
	void ExecuteOpcode(void);
	bool fetchOpcode;


	static const int cycleTable[260];

	unsigned char A;				//Accumulator
	unsigned char X;				//X Index Register
	unsigned char Y;				//Y Index Register
	struct {
		bool C:1;		//Carry
		bool Z:1;		//Zero
		bool I:1;		//Interrupt enable
		bool D:1;		//Decimal
		bool B:1;		//BRK
		bool Unused:1;
		bool V:1;		//Overflow
		bool N:1;		//Sign
	} SR;
	unsigned char *S;	//Status Register
	unsigned short PC;			//Program Counter
	unsigned char SP;			//Stack Pointer
	long executecount;
	unsigned char M;				//Memory
	unsigned short Address;

	void Branch(int Condition);
	int cycles;

	//Addressing modes
	void Immediate(void);
	void ZeroPage(bool bReadMem = true);
	void ZeroPageX(bool bReadMem = true);
	void ZeroPageY(bool bReadMem = true);
	void Absolute(bool bReadMem = true);
	void AbsoluteX(bool bReadMem = true);
	void AbsoluteY(bool bReadMem = true);
	void Indirect(void);
	void IndirectX(bool bReadMem = true);
	void IndirectY(bool bReadMem = true);

	//Group one instructions
	void ADC(void);
	void AND(void);
	void CMP(void);
	void EOR(void);
	void LDA(void);
	void ORA(void);
	void SBC(void);
	void STA(void);

	//Group two instructions
	void LSR(unsigned char & Operand, bool bRegister = false);
	void ASL(unsigned char & Operand, bool bRegister = false);
	void ROL(unsigned char & Operand, bool bRegister = false);
	void ROR(unsigned char & Operand, bool bRegister = false);
	void INC(void);
	void DEC(void);
	void LDX(void);
	void STX(void);
	void TAX(void);
	void DEX(void);
	void DEY(void);
	void TXS(void);
	void TSX(void);

	//Group three instructions
	void LDY(void);
	void STY(void);
	void CPY(void);
	void CPX(void);
	void INX(void);
	void INY(void);
	void BCC(void);
	void BCS(void);
	void BEQ(void);
	void BMI(void);
	void BNE(void);
	void BPL(void);
	void BVC(void);
	void BVS(void);
	void CLC(void);
	void SEC(void);
	void CLD(void);
	void SED(void);
	void CLI(void);
	void SEI(void);
	void CLV(void);
	void BRK(void);
	void JSR(void);
	void PHA(void);
	void PHP(void);
	void PLA(void);
	void PLP(void);
	void JMP(void);
	void BIT(void);
	void RTI(void);
	void TAY(void);
	void TYA(void);
	void TXA(void);
	void RTS(void);
	void SKB(void);

	//illegal
	void ALR();
	void AXS();
	void DCP();
	void ISC();
	void LAX();
	void SAX();


	int irq_checked();
};