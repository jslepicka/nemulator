#pragma once
#include <functional>

class c_z80
{
    typedef std::function<uint8_t(uint16_t)> read_byte_t;
    typedef std::function<void(uint16_t, uint8_t)> write_byte_t;
    typedef std::function<uint8_t(uint8_t)> read_port_t;
    typedef std::function<void(uint8_t, uint8_t)> write_port_t;

  public:
    c_z80(read_byte_t read_byte, write_byte_t write_byte, read_port_t read_port, write_port_t write_port, int *nmi, int *irq, uint8_t *data_bus = 0);
    ~c_z80();
    int emulate_frame();
    int reset();
    void execute(int cycles);
    uint64_t retired_cycles;

  private:
    unsigned short PC;
    int prev_nmi;
    int halted;
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
    int available_cycles;
    int fetch_opcode;
    int opcode;
    int prefix;
    int required_cycles;
    int temp;

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

    int flag_s, flag_z, flag_h, flag_pv, flag_n, flag_c;

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

    enum class ALU_OP
    {
        ADD,
        ADC,
        SUB,
        SBC,
        AND,
        XOR,
        OR,
        CP
    };

    void alu(ALU_OP op, unsigned char operand);
    int test_flag(int f);

    int IFF1; //user accessible
    //on interrupt, IFF1 is copied to IFF2, then IFF1 is set.
    //after interrupt, IFF2 is copied back to IFF1;
    int IFF2;

    int IM; //interrupt mode

    int *nmi;
    int *irq;

    //latch for data bus value.  Used for IM2 (also needed for IM0 which is not currently implemented)
    uint8_t *data_bus;

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
    unsigned short* ddfd_ptr;
    void update_f();
    void update_flags();
    int* flag_ptrs[8] = { &flag_z, &flag_z, &flag_c, &flag_c, &flag_pv, &flag_pv, &flag_s, &flag_s };

    read_byte_t read_byte;
    write_byte_t write_byte;
    read_port_t read_port;
    write_port_t write_port;

    uint16_t read_word(uint16_t address);
    void write_word(uint16_t address, uint16_t data);
};
