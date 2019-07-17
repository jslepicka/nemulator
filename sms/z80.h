#pragma once
class c_sms;

class c_z80
{
public:
	c_z80(c_sms *sms);
	~c_z80(void);
	int emulate_frame();
	int reset();
	void execute(int cycles);
	int pending_psg_cycles;
private:
	int needed_cycles;
	unsigned short PC;
	int prev_nmi;
	int halted;
	int pre_pc;
	struct s_log_entry
	{
		int inst;
		int pc;
		int a;
		int flags;
		int b;
		int c;
		int d;
		int e;
		int h;
		int l;
		int ix;
		int iy;
		int sp;
	};
	int pending_ei;
	int dd;
	int fd;
	c_sms *sms;
	int available_cycles;
	int fetch_opcode;
	int opcode;
	int prefix;
	int required_cycles;
	int temp;
	int temp2;

	int x, y, z, p, q;
	signed char d;
	unsigned char tchar;

	static const int cc[8];
	unsigned char **r, *r_00[8], *r_dd[8], *r_fd[8];
	unsigned short **rp, *rp_00[4], *rp_dd[4], *rp_fd[8];
	unsigned short **rp2, *rp2_00[4], *rp2_dd[4], *rp2_fd[8];

	static const int cycle_table[262];
	static const int cb_cycle_table[256];
	static const int dd_cycle_table[256];
	static const int ed_cycle_table[256];
	static const int ddcb_cycle_table[256];

	void execute_opcode();
	void push_word(unsigned short value);
	unsigned short pull_word();

	void set_s(int set);
	void set_z(int set);
	void set_h(int set);
	void set_pv(int set);
	void set_n(int set);
	void set_c(int set);
	int get_parity(unsigned char value);

	void OR(unsigned char operand);
	void INC(unsigned char *operand);
	void DEC(unsigned char *operand);
	void RST(unsigned char dest);
	void RLC(unsigned char *operand);
	void RL(unsigned char *operand);
	void RR(unsigned char *operand);
	void BIT(int bit, unsigned char operand);
	void ADD8(unsigned char operand, int carry = 0);
	void ADC8(unsigned char operand);
	void SUB8(unsigned char operand, int carry = 0);
	void NEG();
	void AND(unsigned char operand);
	void CP(unsigned char operand);
	void XOR(unsigned char operand);
	void ADD16(unsigned short *dest, unsigned short operand);
	void SBC16(unsigned short operand);
	void ADC16(unsigned short operand);
	void ROT(int y, unsigned char *operand);
	void inc_r();

	void alu(int op, unsigned char operand);
	int test_flag(int f);

	void swap_register(unsigned short *first, unsigned short *second);

	int IFF1; //user accessible
	//on interrupt, IFF1 is copied to IFF2, then IFF1 is set.
	//after interrupt, IFF2 is copied back to IFF1;
	int IFF2;

	int IM; //interrupt mode
	unsigned short SP;

	union
	{
		struct
		{
			unsigned char lo;
			unsigned char hi;
		} byte;
		unsigned short word;
	} AF, AF2, BC, BC2, DE, DE2, HL, HL2, IX, IY;

	unsigned char I, R;

};
