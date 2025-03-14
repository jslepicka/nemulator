module;

export module z80;
import nemulator.std;
import random;

export class c_z80
{

    typedef std::function<uint8_t(uint16_t)> read_byte_t;
    typedef std::function<void(uint16_t, uint8_t)> write_byte_t;
    typedef std::function<uint8_t(uint8_t)> read_port_t;
    typedef std::function<void(uint8_t, uint8_t)> write_port_t;
    typedef std::function<void()> int_ack_t; //interrupt acknowledge callback


  public:
    c_z80(read_byte_t read_byte, write_byte_t write_byte, read_port_t read_port, write_port_t write_port, int_ack_t int_ack, int *nmi,
          int *irq, uint8_t *data_bus)
    {
        int count = 0;
        this->read_byte = read_byte;
        this->write_byte = write_byte;
        this->read_port = read_port;
        this->write_port = write_port;
        this->nmi = nmi;
        this->irq = irq;
        this->data_bus = data_bus;
        this->int_ack_callback = int_ack;
    }
    ~c_z80()
    {
    }
    int reset()
    {
        pending_ei = 0;
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
        //Setting R to 0 causes Impossible Mission to use the same map every time
        //R = 0;
        R = random::get_rand() & 0xFF;

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

        ddfd_ptr = 0;
        flag_s = flag_z = flag_h = flag_pv = flag_n = flag_c = 0;
        retired_cycles = 0;
        return 1;
    }
    void execute(int cycles)
    {
        available_cycles += cycles;
        while (true) {
            if (fetch_opcode) {
                inc_r();
                ddfd_ptr = 0;
                //dd = 0;
                //fd = 0;
                r = r_00;
                rp = rp_00;
                rp2 = rp2_00;

                if (*nmi && prev_nmi == 0) {
                    opcode = 0x104;
                    halted = 0;
                }
                else if (*irq && IFF1) {
                    IFF1 = IFF2 = 0;
                    halted = 0;
                    switch (IM) {
                        case 0:
                            opcode = 0x105;
                            break;
                        case 2:
                            //Invalid interrupt mode for sms
                            opcode = 0x102;
                            break;
                        case 1:
                            opcode = 0x101;
                            break;
                    }
                }
                else if (halted) {
                    opcode = 0x103;
                }
                else {
                    opcode = read_byte(PC++);
                    if (pending_ei) {
                        IFF1 = IFF2 = 1;
                        pending_ei = 0;
                    }
                }

                prev_nmi = *nmi;

                switch (opcode) {
                    case 0xCB:
                        inc_r();
                        opcode <<= 8;
                        opcode |= read_byte(PC++);
                        required_cycles += cb_cycle_table[opcode & 0xFF];
                        prefix = 0xCB;
                        break;
                    case 0xDD:
                        inc_r();
                        ddfd_ptr = &IX.word;
                        opcode <<= 8;
                        opcode |= read_byte(PC++);
                        if ((opcode & 0xFF) == 0xCB) {
                            opcode <<= 8;
                            opcode |= read_byte(PC++);
                            opcode <<= 8;
                            opcode |= read_byte(PC++);
                            required_cycles += ddcb_cycle_table[opcode & 0xFF];
                            prefix = 0xDDCB;
                        }
                        else {
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
                        opcode |= read_byte(PC++);
                        required_cycles += ed_cycle_table[opcode & 0xFF];
                        prefix = 0xED;
                        break;
                    case 0xFD:
                        inc_r();
                        opcode <<= 8;
                        ddfd_ptr = &IY.word;
                        opcode |= read_byte(PC++);
                        if ((opcode & 0xFF) == 0xCB) {
                            opcode <<= 8;
                            opcode |= read_byte(PC++);
                            opcode <<= 8;
                            opcode |= read_byte(PC++);
                            required_cycles += ddcb_cycle_table[opcode & 0xFF];
                            prefix = 0xDDCB;
                        }
                        else {
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
                        if (opcode == 0x101 || opcode == 0x103 || opcode == 0x104 || opcode == 0x105 || opcode == 0x102)
                            prefix = 0x01;
                        else
                            prefix = 0;
                        required_cycles += cycle_table[opcode];
                        break;
                }
                fetch_opcode = 0;
            }
            if (required_cycles <= available_cycles) {
                available_cycles -= required_cycles;
                retired_cycles += required_cycles;

                fetch_opcode = 1;

                required_cycles = 0;
                execute_opcode();
            }
            else {
                break;
            }
        }
    }
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

    unsigned char **r, *r_00[8], *r_dd[8], *r_fd[8];
    unsigned short **rp, *rp_00[4], *rp_dd[4], *rp_fd[8];
    unsigned short **rp2, *rp2_00[4], *rp2_dd[4], *rp2_fd[8];

    static const int cycle_table[262];
    static const int cb_cycle_table[256];
    static const int dd_cycle_table[256];
    static const int ed_cycle_table[256];
    static const int ddcb_cycle_table[256];

    int flag_s, flag_z, flag_h, flag_pv, flag_n, flag_c;

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

    union {
        struct
        {
            unsigned char lo;
            unsigned char hi;
        } byte;
        unsigned short word;
    } AF, AF2, BC, BC2, DE, DE2, HL, HL2, IX, IY;

    unsigned char I, R;
    unsigned short *ddfd_ptr;

    int *flag_ptrs[8] = {&flag_z, &flag_z, &flag_c, &flag_c, &flag_pv, &flag_pv, &flag_s, &flag_s};

    read_byte_t read_byte;
    write_byte_t write_byte;
    read_port_t read_port;
    write_port_t write_port;
    int_ack_t int_ack_callback;

    void int_ack()
    {
        if (int_ack_callback != nullptr) {
            int_ack_callback();
        }
    }

    void alu(ALU_OP op, unsigned char operand)
    {
        switch (op) {
            case ALU_OP::ADD: //ADD
                ADD8(operand);
                break;
            case ALU_OP::ADC: //ADC
                ADD8(operand, flag_c ? 1 : 0);
                break;
            case ALU_OP::SUB: //SUB
                SUB8(operand);
                break;
            case ALU_OP::SBC: //SBC
                SUB8(operand, flag_c ? 1 : 0);
                break;
            case ALU_OP::AND: //AND
                AND(operand);
                break;
            case ALU_OP::XOR: //XOR
                XOR(operand);
                break;
            case ALU_OP::OR: //OR
                OR(operand);
                break;
            case ALU_OP::CP: //CP
                CP(operand);
                break;
        }
    }
    int test_flag(int f)
    {
        int t = *flag_ptrs[f];
        //if bit 0 of c is set, check for positive condition
        if (f & 1) {
            if (t)
                return 1;
        }
        else {
            if (!t)
                return 1;
        }
        return 0;
    }

    int get_parity(unsigned char value)
    {
        int sum = 0;
        for (; value; value >>= 1) {
            sum += value & 0x1;
        }
        return !(sum & 0x1);
    }

    void push_word(unsigned short value)
    {
        SP -= 2;
        write_word(SP, value);
    }
    unsigned short pull_word()
    {
        unsigned short ret = read_word(SP);
        SP += 2;
        return ret;
    }

    void set_s(int set)
    {
        flag_s = set;
    }
    void set_z(int set)
    {
        flag_z = set;
    }
    void set_h(int set)
    {
        flag_h = set;
    }
    void set_pv(int set)
    {
        flag_pv = set;
    }
    void set_n(int set)
    {
        flag_n = set;
    }
    void set_c(int set)
    {
        flag_c = set;
    }
    void OR(unsigned char operand)
    {
        AF.byte.hi |= operand;
        set_c(0);
        set_n(0);
        set_pv(get_parity(AF.byte.hi));
        set_h(0);
        set_z(AF.byte.hi == 0);
        set_s(AF.byte.hi & 0x80);
    }

    void INC(unsigned char *operand)
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

    void DEC(unsigned char *operand)
    {
        temp = 1;
        unsigned short result = *operand - temp;
        set_s(result & 0x80);
        set_z((result & 0xFF) == 0);
        set_h(((unsigned char)temp & 0xF) > (*operand & 0xF));
        set_pv((*operand ^ result) & (temp ^ *operand) & 0x80);
        signed char s = *operand;
        signed char s1 = *operand - 1;
        set_n(1);
        *operand = result & 0xFF;
    }

    void ROT(int y, unsigned char *operand)
    {
        int temp3;
        switch (y) {
            case 0: //RLC
                RLC(operand);
                break;
            case 1: //RRC
                tchar = *operand;
                set_c(tchar & 0x1);
                tchar >>= 1;
                tchar |= ((flag_c ? 1 : 0) << 7);
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

    void RST(unsigned char dest)
    {
        push_word(PC);
        PC = dest;
    }

    void RLC(unsigned char *operand)
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

    void RL(unsigned char *operand)
    {
        int carry = (*operand & 0x80);
        *operand <<= 1;
        if (flag_c)
            *operand |= 0x1;
        set_c(carry);
        set_n(0);
        set_h(0);
        set_pv(get_parity(*operand));
        set_z(*operand == 0);
        set_s(*operand & 0x80);
    }

    void RR(unsigned char *operand)
    {
        int carry = flag_c;
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

    void BIT(int bit, unsigned char operand)
    {
        set_n(0);
        set_h(1);
        set_z((operand & (1 << bit)) == 0);
        set_pv(flag_z);
        set_s(bit == 7 ? !(flag_z) : 0);
    }

    void ADD8(unsigned char operand, int carry = 0)
    {
         //int c = operand + carry;
        int result = AF.byte.hi + operand + carry;
        unsigned int uresult = (unsigned int)result;
        set_pv((AF.byte.hi ^ result) & (operand ^ result) & 0x80);
        set_c(uresult < 0 || uresult > 255);
        set_s(result & 0x80);
        set_n(0);
        set_h(((AF.byte.hi & 0xF) + (operand & 0xF) + (carry & 0xF)) > 0xF);
        AF.byte.hi = uresult & 0xFF;
        set_z(AF.byte.hi == 0);
    }

    void ADC8(unsigned char operand)
    {
        ADD8(operand, flag_c ? 1 : 0);
    }

    void AND(unsigned char operand)
    {
        AF.byte.hi = AF.byte.hi & operand;
        set_c(0);
        set_n(0);
        set_pv(get_parity(AF.byte.hi));
        set_h(1);
        set_z(AF.byte.hi == 0);
        set_s(AF.byte.hi & 0x80);
    }

    void CP(unsigned char operand)
    {
        int c = operand;
        int result = (int)AF.byte.hi - (int)c;
        unsigned int uresult = (unsigned int)result;
        set_pv((AF.byte.hi ^ result) & (operand ^ AF.byte.hi) & 0x80);
        set_c(uresult < 0 || uresult > 255);
        set_n(1);
        set_z((result & 0xFF) == 0);
        set_s(result & 0x80);
        set_h((AF.byte.hi & 0xF) - (c & 0xF) < 0);
    }

    void SBC16(unsigned short operand)
    {
        int x = operand + (flag_c ? 1 : 0);
        int result = HL.word - x;
        unsigned int uresult = (unsigned int)result;
        set_h(((HL.word & 0xFFF) - (x & 0xFFF) - (flag_c ? 1 : 0)) < 0);
        set_pv((HL.word ^ result) & (x ^ HL.word) & 0x8000);
        set_c(result < 0);
        set_n(1);
        set_s(result & 0x8000);
        HL.word = result & 0xFFFF;
        set_z(HL.word == 0);
    }

    void SUB8(unsigned char operand, int carry = 0)
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

    void NEG()
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

    void XOR(unsigned char operand)
    {
        AF.byte.hi ^= operand;
        set_c(0);
        set_n(0);
        set_h(0);
        set_z(AF.byte.hi == 0);
        set_s(AF.byte.hi & 0x80);
        set_pv(get_parity(AF.byte.hi));
    }

    void ADD16(unsigned short *dest, unsigned short operand)
    {
        set_h((*dest & 0xFFF) + (operand & 0xFFF) > 0xFFF);
        int result = *dest + operand;
        set_c(result > 0xFFFF);
        set_n(0);
        *dest = result;
    }

    void ADC16(unsigned short operand)
    {
        int x = operand + (flag_c ? 1 : 0);
        int result = HL.word + x;
        set_h(((operand & 0xFFF) + (HL.word & 0xFFF) + (flag_c ? 1 : 0)) > 0xFFF);
        set_c((HL.word + x) > 0xFFFF);
        set_n(0);
        set_pv((HL.word ^ result) & (operand ^ result) & 0x8000);
        set_s(result & 0x8000);
        HL.word = result & 0xFFFF;
        set_z(HL.word == 0);
    }

    void inc_r()
    {
        R = (R & 0x80) | ((R + 1) & 0x7F);
    }
    __forceinline void update_f()
    {
        AF.byte.lo = (flag_s ? 0x80 : 0) | (flag_z ? 0x40 : 0) | (flag_h ? 0x10 : 0) | (flag_pv ? 0x4 : 0) |
                     (flag_n ? 0x2 : 0) | (flag_c ? 0x1 : 0);
    }
    void update_flags()
    {
        flag_s = AF.byte.lo & 0x80;
        flag_z = AF.byte.lo & 0x40;
        flag_h = AF.byte.lo & 0x10;
        flag_pv = AF.byte.lo & 0x4;
        flag_n = AF.byte.lo & 0x2;
        flag_c = AF.byte.lo & 0x1;
    }
    uint16_t read_word(uint16_t address)
    {
        uint16_t lo = read_byte(address);
        uint16_t hi = read_byte(address + 1);
        return lo | (hi << 8);
    }
    void write_word(uint16_t address, uint16_t data)
    {
        write_byte(address, data & 0xFF);
        write_byte(address + 1, data >> 8);
    }

    void execute_opcode()
    {
        //decode per http://www.z80.info/decoding.htm
        int o = opcode & 0xFF;
        z = opcode & 0x7;
        y = (opcode >> 3) & 0x7;
        x = (opcode >> 6) & 0x3;
        q = y & 0x1;
        p = (y >> 1) & 0x3;

        switch (prefix) {
            case 0x00:
                switch (x) {
                    case 0:
                        switch (z) {
                            case 0:
                                switch (y) {
                                    case 0:
                                        //NOP
                                        break;
                                    case 1:
                                        //EX AF, AF'
                                        update_f();
                                        std::swap(AF.word, AF2.word);
                                        update_flags();
                                        break;
                                    case 2:
                                        //DJNZ
                                        d = (signed char)read_byte(PC++);
                                        BC.byte.hi--;
                                        if (BC.byte.hi != 0) {
                                            required_cycles += 5;
                                            PC += d;
                                        }
                                        break;
                                    case 3:
                                        //JR
                                        d = (signed char)read_byte(PC++);
                                        PC += d;
                                        break;
                                    case 4:
                                    case 5:
                                    case 6:
                                    case 7:
                                        //JR cc[y-4], d
                                        d = (signed char)read_byte(PC++);
                                        if (test_flag(y - 4)) {
                                            PC += d;
                                            required_cycles += 5;
                                        }
                                        break;
                                }
                                break;

                            case 1:
                                switch (q) {
                                    case 0:
                                        //LD rp[p],nn
                                        temp = read_word(PC);
                                        PC += 2;
                                        *rp[p] = temp;
                                        break;
                                    case 1:
                                        //ADD HL, rp[p]
                                        ADD16(rp[2], *rp[p]);
                                        break;
                                }
                                break;
                            case 2:
                                switch (q) {
                                    case 0:
                                        switch (p) {
                                            case 0:
                                                //LD (BC), A
                                                write_byte(BC.word, AF.byte.hi);
                                                break;
                                            case 1:
                                                //LD (DE), A
                                                write_byte(DE.word, AF.byte.hi);
                                                break;
                                            case 2:
                                                //LD (nn), HL
                                                temp = read_word(PC);
                                                PC += 2;
                                                write_word(temp, *rp[2]);
                                                break;
                                            case 3:
                                                //LD (nn), A
                                                temp = read_word(PC);
                                                PC += 2;
                                                write_byte(temp, AF.byte.hi);
                                                break;
                                        }
                                        break;
                                    case 1:
                                        switch (p) {
                                            case 0:
                                                //LD A, (BC)
                                                AF.byte.hi = read_byte(BC.word);
                                                break;
                                            case 1:
                                                //LD A, (DE)
                                                AF.byte.hi = read_byte(DE.word);
                                                break;
                                            case 2:
                                                 //LD HL, (nn)
                                                temp = read_word(PC);
                                                PC += 2;
                                                *rp[2] = read_word(temp);
                                                break;
                                            case 3:
                                                //LD A, (nn)
                                                temp = read_word(PC);
                                                PC += 2;
                                                AF.byte.hi = read_byte(temp);
                                                break;
                                        }
                                        break;
                                }
                                break;
                            case 3:
                                switch (q) {
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
                                if (y == 6) {
                                    unsigned char temp;
                                    if (ddfd_ptr) {
                                        d = (signed char)read_byte(PC++);
                                        unsigned short loc = *ddfd_ptr + d;
                                        temp = read_byte(loc);
                                        INC(&temp);
                                        write_byte(loc, temp);
                                    }
                                    else {
                                        temp = read_byte(*rp[2]);
                                        INC(&temp);
                                        write_byte(*rp[2], temp);
                                    }
                                }
                                else {
                                    INC(r[y]);
                                }
                                break;
                            case 5:
                                //DEC r[y]
                                if (y == 6) {
                                    unsigned char temp;
                                    if (ddfd_ptr) {
                                        d = (signed char)read_byte(PC++);
                                        unsigned short loc = *ddfd_ptr + d;
                                        temp = read_byte(loc);
                                        DEC(&temp);
                                        write_byte(loc, temp);
                                    }
                                    else {
                                        temp = read_byte(*rp[2]);
                                        DEC(&temp);
                                        write_byte(*rp[2], temp);
                                    }
                                }
                                else {
                                    DEC(r[y]);
                                }
                                break;
                            case 6:
                                //LD r[y], n
                                if (y == 6) {
                                    if (ddfd_ptr) {
                                        d = (signed char)read_byte(PC++);
                                        temp = read_byte(PC++);
                                        write_byte(*ddfd_ptr + d, temp);
                                    }
                                    else {
                                        temp = read_byte(PC++);
                                        write_byte(*rp[2], temp);
                                    }
                                }
                                else {
                                    temp = read_byte(PC++);
                                    *r[y] = temp;
                                }
                                break;
                            case 7:
                                switch (y) {
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
                                        AF.byte.hi |= (flag_c ? 1 : 0);
                                        set_c(temp);
                                        set_n(0);
                                        set_h(0);
                                        break;
                                    case 3:
                                        //RRA
                                        temp = AF.byte.hi & 0x1;
                                        AF.byte.hi >>= 1;
                                        AF.byte.hi |= ((flag_c ? 1 : 0) << 7);
                                        set_c(temp);
                                        set_n(0);
                                        set_h(0);
                                        break;
                                    case 4:
                                        //DAA
                                        {
                                            int a_before = AF.byte.hi;
                                            int correction = 0;
                                            if (AF.byte.hi > 0x99 || flag_c) {
                                                correction = 0x60;
                                                set_c(1);
                                            }
                                            else {
                                                set_c(0);
                                            }
                                            if ((AF.byte.hi & 0xF) > 0x9 || flag_h) {
                                                correction |= 0x06;
                                            }
                                            if (flag_n) {
                                                AF.byte.hi -= correction;
                                            }
                                            else {
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
                                        set_h(flag_c);
                                        set_c(!flag_c);
                                        break;
                                }
                                break;
                        }
                        break;
                    case 1: //x=1
                        if (z == 6 && y == 6) {
                            //HALT
                            halted = 1;
                        }
                        else {
                            //LD r[y], r[z]
                            unsigned char src = 0;

                            if (z == 6) {
                                if (ddfd_ptr) {
                                    d = (signed char)read_byte(PC++);
                                    src = read_byte(*ddfd_ptr + d);
                                }
                                else {
                                    src = read_byte(HL.word);
                                }
                            }
                            else {
                                if (ddfd_ptr && y == 6) {
                                    if (z == 4) {
                                        src = HL.byte.hi;
                                    }
                                    else if (z == 5) {
                                        src = HL.byte.lo;
                                    }
                                    else {
                                        src = *r[z];
                                    }
                                }
                                else {
                                    src = *r[z];
                                }
                            }

                            if (y == 6) //HL
                            {
                                if (ddfd_ptr) {
                                    d = (signed char)read_byte(PC++);
                                    write_byte(*ddfd_ptr + d, src);
                                }
                                else {
                                    write_byte(*rp[2], src);
                                }
                            }
                            else {
                                if (ddfd_ptr && z == 6) {
                                    if (y == 4) {
                                        HL.byte.hi = src;
                                    }
                                    else if (y == 5) {
                                        HL.byte.lo = src;
                                    }
                                    else {
                                        *r[y] = src;
                                    }
                                }
                                else {
                                    *r[y] = src;
                                }
                            }
                        }
                        break;
                    case 2: //x=2
                        //alu[y] r[z]
                        if (z == 6) {
                            if (ddfd_ptr) {
                                d = (signed char)read_byte(PC++);
                                alu((ALU_OP)y, read_byte(*ddfd_ptr + d));
                            }
                            else {
                                alu((ALU_OP)y, read_byte(*rp[2]));
                            }
                        }
                        else {
                            alu((ALU_OP)y, *r[z]);
                        }
                        break;
                    case 3: //x=3
                        switch (z) {
                            case 0:
                                //RET cc[y]
                                if (test_flag(y))
                                    PC = pull_word();
                                break;
                            case 1:
                                switch (q) {
                                    case 0:
                                        //POP rp2[p]
                                        *rp2[p] = pull_word();
                                        if (p == 3)
                                            update_flags();
                                        break;
                                    case 1:
                                        switch (p) {
                                            case 0:
                                                //RET
                                                PC = pull_word();
                                                break;
                                            case 1:
                                                //EXX
                                                std::swap(BC.word, BC2.word);
                                                std::swap(DE.word, DE2.word);
                                                std::swap(HL.word, HL2.word);
                                                break;
                                            case 2:
                                                //JP HL
                                                PC = *rp[2];
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
                                temp = read_word(PC);
                                PC += 2;
                                if (test_flag(y))
                                    PC = temp;
                                break;
                            case 3:
                                switch (y) {
                                    case 0:
                                        //JP nn
                                        temp = read_word(PC);
                                        PC += 2;
                                        PC = temp;
                                        break;
                                    case 1:
                                        //CB - should never get here?
                                        break;
                                    case 2:
                                        //OUT (n), A
                                        temp = read_byte(PC++);
                                        write_port(temp, AF.byte.hi);
                                        break;
                                    case 3:
                                        //IN A, (n)
                                        temp = read_byte(PC++);
                                        AF.byte.hi = read_port(temp);
                                        break;
                                    case 4:
                                        //EX (SP), HL
                                        {
                                            int sp0 = read_byte(SP);
                                            int sp1 = read_byte(SP + 1);
                                            write_byte(SP, *r[5]);
                                            write_byte(SP + 1, *r[4]);
                                            *r[5] = sp0;
                                            *r[4] = sp1;
                                        }
                                        break;
                                    case 5:
                                        //EX DE, HL
                                        if (ddfd_ptr) {
                                            //EX DE, IX and EX DE, IY are invalid
                                            int x = 1;
                                        }
                                        temp = DE.word;
                                        DE.word = *rp[2];
                                        *rp[2] = temp;
                                        break;
                                    case 6:
                                        //DI
                                        IFF1 = IFF2 = 0;
                                        if (pending_ei != 0) {
                                            int x = 1;
                                        }
                                        pending_ei = 0;
                                        break;
                                    case 7:
                                        //EI
                                        pending_ei = 1;
                                        break;
                                }
                                break;
                            case 4:
                                //CALL cc[y], nn
                                temp = read_word(PC);
                                PC += 2;
                                if (test_flag(y)) {
                                    push_word(PC);
                                    required_cycles += 7;
                                    PC = temp;
                                }
                                break;
                            case 5:
                                switch (q) {
                                    case 0:
                                        //PUSH rp2[p]
                                        if (p == 3)
                                            update_f();
                                        push_word(*rp2[p]);
                                        break;
                                    case 1:
                                        switch (p) {
                                            case 0:
                                                //CALL nn
                                                temp = read_word(PC);
                                                PC += 2;
                                                push_word(PC);
                                                PC = temp;
                                                break;
                                            case 1:
                                                //DD - should not get here
                                                break;
                                            case 2:
                                                //ED - should not get here
                                                break;
                                            case 3:
                                                //FD - should not get here
                                                break;
                                        }
                                        break;
                                }
                                break;
                            case 6:
                                //alu[y], n
                                alu((ALU_OP)y, read_byte(PC++));
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
                switch (z) {
                    case 1:
                        //IM1
                        push_word(PC);
                        PC = 0x38;
                        int_ack();
                        break;
                    case 2: {
                        //IM2
                        uint16_t v = 0;
                        push_word(PC);
                        v = *data_bus | (I << 8);
                        PC = read_word(v & 0xFFFE); //is this mask correct?
                        int_ack();
                    } break;
                    case 3: //halt
                    {
                        int x = 1;
                    } break;
                    case 4: //nmi
                        push_word(PC);
                        PC = 0x66;
                        break;
                    case 5: //IM0
                        opcode = *data_bus;
                        prefix = 0;
                        required_cycles += cycle_table[opcode];
                        fetch_opcode = 0;
                        int_ack();
                        break;
                    default:
                        break;
                }
                break;

            case 0xCB:
                if (z == 6) {
                    tchar = read_byte(HL.word);
                }
                else {
                    tchar = *r[z];
                }

                switch (x) {
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
                if (z == 6) {
                    write_byte(HL.word, tchar);
                }
                else {
                    //todo: is this correct?
                    *r[z] = tchar;
                }
                break;

            case 0xED:
                switch (x) {
                    case 0:
                    case 3:
                        //NOP
                        break;
                    case 1:
                        switch (z) {
                            case 0:
                                if (y == 6) {
                                    //IN (C)
                                    temp = read_port(BC.byte.lo);
                                    set_n(0);
                                    set_pv(get_parity(temp));
                                    set_h(0);
                                    set_z(temp == 0);
                                    set_s(temp & 0x80);
                                }
                                else {
                                    //IN r[y], (C)
                                    *r[y] = read_port(BC.byte.lo);
                                    set_n(0);
                                    set_pv(get_parity(*r[y]));
                                    set_h(0);
                                    set_z(*r[y] == 0);
                                    set_s(*r[y] & 0x80);
                                }
                                break;
                            case 1:
                                if (y == 6) {
                                    //OUT (C), 0
                                    write_port(BC.byte.lo, 0);
                                }
                                else {
                                    //OUT (C), r[y]
                                    write_port(BC.byte.lo, *r[y]);
                                }
                                break;
                            case 2:
                                switch (q) {
                                    case 0:
                                        //SBC HL, rp[p]
                                        SBC16(*rp[p]);
                                        break;
                                    case 1:
                                        //ADC HL, rp[p]
                                        ADC16(*rp[p]);
                                        break;
                                }
                                break;
                            case 3:
                                switch (q) {
                                    case 0:
                                        //LD (nn), rp[p]
                                        temp = read_word(PC);
                                        PC += 2;
                                        write_word(temp, *rp[p]);
                                        break;
                                    case 1:
                                        //LD rp[p], (nn)
                                        temp = read_word(PC);
                                        PC += 2;
                                        *rp[p] = read_word(temp);
                                        break;
                                }
                                break;
                            case 4:
                                //NEG
                                NEG();
                                break;
                            case 5:
                                if (y == 1) {
                                    //RETI
                                    PC = pull_word();
                                }
                                else {
                                    //RETN
                                    PC = pull_word();
                                    IFF1 = IFF2;
                                }
                                break;
                            case 6:
                                //IM im[y]
                                switch (y & 0x3) {
                                    case 0:
                                        IM = 0;
                                        break;
                                    case 1:
                                        //Undefined mode
                                        IM = 0;
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
                                switch (y) {
                                    case 0:
                                        //LD I, A
                                        I = AF.byte.hi;
                                        break;
                                    case 1:
                                        //LD R, A
                                        R = AF.byte.hi;
                                        break;
                                    case 2:
                                        //LD A, I
                                        AF.byte.hi = I;
                                        set_z(AF.byte.hi == 0);
                                        set_s(AF.byte.hi & 0x80);
                                        set_h(0);
                                        set_n(0);
                                        set_pv(IFF2);
                                        break;
                                    case 3:
                                        //LD A, R
                                        AF.byte.hi = R;
                                        set_z(AF.byte.hi == 0);
                                        set_s(AF.byte.hi & 0x80);
                                        set_h(0);
                                        set_n(0);
                                        set_pv(IFF2);
                                        break;
                                    case 4:
                                        //RRD
                                        {
                                            int x = read_byte(HL.word);
                                            int x_lo = x & 0xF;
                                            x >>= 4;
                                            x |= ((AF.byte.hi & 0xF) << 4);
                                            write_byte(HL.word, x);
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
                                        {
                                            int x = read_byte(HL.word);
                                            int x_hi = x & 0xF0;
                                            x <<= 4;
                                            x |= (AF.byte.hi & 0xF);
                                            write_byte(HL.word, x);
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
                        if (z <= 3 && y >= 4) {
                            //BLI[y,z]
                            switch (y) {
                                case 4:
                                    switch (z) {
                                        case 0:
                                            //LDI
                                            write_byte(DE.word, read_byte(HL.word));
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
                                                unsigned char s = read_byte(HL.word);
                                                int z = AF.byte.hi == s;
                                                int c = flag_c;
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
                                            write_byte(HL.word, read_port(BC.byte.lo));
                                            HL.word++;
                                            BC.byte.hi--;
                                            set_n(0);
                                            set_z(BC.byte.hi == 0);
                                            break;
                                        case 3:
                                            //OUTI
                                            write_port(BC.byte.lo, read_byte(HL.word));
                                            HL.word++;
                                            BC.byte.hi--;
                                            set_n(1);
                                            set_z(BC.byte.hi == 0);
                                            break;
                                    }
                                    break;
                                case 5:
                                    switch (z) {
                                        case 0:
                                            //LDD
                                            write_byte(DE.word, read_byte(HL.word));
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
                                                unsigned char s = read_byte(HL.word);
                                                int z = AF.byte.hi == s;
                                                int c = flag_c;
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
                                            write_byte(HL.word, read_port(BC.byte.lo));
                                            HL.word--;
                                            BC.byte.hi--;
                                            set_n(1);
                                            set_z(BC.byte.hi == 0);
                                            break;
                                        case 3:
                                            //OUTD
                                            write_port(BC.byte.lo, read_byte(HL.word));
                                            HL.word--;
                                            BC.byte.hi--;
                                            set_n(1);
                                            set_z(BC.byte.hi == 0);
                                            break;
                                    }
                                    break;
                                case 6:
                                    switch (z) {
                                        case 0:
                                            //LDIR
                                            write_byte(DE.word, read_byte(HL.word));
                                            HL.word++;
                                            DE.word++;
                                            BC.word--;
                                            if (BC.word != 0) {
                                                required_cycles += 5;
                                                PC -= 2;
                                                set_pv(1);
                                            }
                                            else {
                                                set_pv(0);
                                            }
                                            set_n(0);
                                            set_h(0);
                                            break;
                                        case 1:
                                            //CPIR
                                            {
                                                tchar = read_byte(HL.word);
                                                int z = AF.byte.hi == tchar;
                                                int c = flag_c;
                                                CP(tchar);
                                                HL.word++;
                                                BC.word--;
                                                set_pv(BC.word != 0);
                                                set_z(z);
                                                set_c(c);
                                                if (BC.word != 0 && AF.byte.hi != tchar) {
                                                    PC -= 2;
                                                    required_cycles += 5;
                                                }
                                            }
                                            break;
                                        case 2:
                                            //INIR
                                            write_byte(HL.word, read_port(BC.byte.lo));
                                            HL.word++;
                                            BC.byte.hi--;
                                            set_z(BC.byte.hi == 0);
                                            set_n(1);
                                            if (BC.byte.hi != 0) {
                                                PC -= 2;
                                                required_cycles += 5;
                                            }
                                            break;
                                        case 3:
                                            //OTIR
                                            write_port(BC.byte.lo, read_byte(HL.word));
                                            HL.word++;
                                            BC.byte.hi--;
                                            if (BC.byte.hi != 0) {
                                                required_cycles += 5;
                                                PC -= 2;
                                            }
                                            set_n(1);
                                            set_z(BC.byte.hi == 0);
                                            break;
                                    }
                                    break;
                                case 7:
                                    switch (z) {
                                        case 0:
                                            //LDDR
                                            write_byte(DE.word, read_byte(HL.word));
                                            DE.word--;
                                            HL.word--;
                                            BC.word--;
                                            if (BC.word != 0) {
                                                required_cycles += 5;
                                                PC -= 2;
                                                set_pv(1);
                                            }
                                            else {
                                                set_pv(0);
                                            }
                                            set_n(0);
                                            set_h(0);
                                            break;
                                        case 1:
                                            //CPDR
                                            {
                                                tchar = read_byte(HL.word);
                                                int z = AF.byte.hi == tchar;
                                                int c = flag_c;
                                                CP(tchar);
                                                HL.word--;
                                                BC.word--;
                                                set_pv(BC.word != 0);
                                                set_z(z);
                                                set_c(c);
                                                if (BC.word != 0 && AF.byte.hi != tchar) {
                                                    PC -= 2;
                                                    required_cycles += 5;
                                                }
                                            }
                                            break;
                                        case 2:
                                            //INDR
                                            write_byte(HL.word, BC.byte.lo);
                                            HL.word--;
                                            BC.byte.hi--;
                                            set_z(BC.byte.hi == 0);
                                            set_n(1);
                                            if (BC.byte.hi != 0) {
                                                PC -= 2;
                                                required_cycles += 5;
                                            }
                                            break;
                                        case 3:
                                            //OTDR
                                            write_port(BC.byte.lo, read_byte(HL.word));
                                            HL.word--;
                                            BC.byte.hi--;
                                            set_z(BC.byte.hi == 0);
                                            set_n(1);
                                            if (BC.byte.hi != 0) {
                                                PC -= 2;
                                                required_cycles += 5;
                                            }
                                            break;
                                    }
                                    break;
                            }
                        }
                        else {
                            //NOP
                        }
                        break;
                }
                break;

            case 0xDDCB:
                d = (signed char)((opcode >> 8) & 0xFF);
                switch (x) {
                    case 0:
                        if (z != 6) {
                            //undocumented
                            //LD r[z], rot[y] (IX+d)
                        }
                        else {
                            //rot[y] (IX+d)
                            //temp = dd ? IX.word : IY.word;
                            unsigned short loc = *ddfd_ptr + d;
                            tchar = read_byte(loc);
                            ROT(y, &tchar);
                            write_byte(loc, tchar);
                        }
                        break;
                    case 1:
                        //BIT y, r[z]
                        BIT(y, read_byte(*ddfd_ptr + d));
                        break;
                    case 2:
                        if (z != 6) {
                            //undocumented
                            //LD r[z], RES y, (IX+d)
                        }
                        else {
                            //RES y, (IX+d)
                            //temp = dd ? IX.word : IY.word;
                            unsigned short loc = *ddfd_ptr + d;
                            temp = read_byte(loc);
                            temp &= ~(1 << y);
                            write_byte(loc, temp);
                        }
                        break;
                    case 3:
                        if (z != 6) {
                            //undocumented
                            //LD r[z], SET y, (IX+d)
                        }
                        else {
                            //temp = dd ? IX.word : IY.word;
                            unsigned short loc = *ddfd_ptr + d;
                            tchar = read_byte(loc);
                            tchar = tchar | (1 << y);
                            write_byte(loc, tchar);
                        }
                        break;
                    default:
                        break;
                }
                break;

            default:
                //shouldn't reach here
                break;
        }
    }
};

// clang-format off
const int c_z80::cycle_table[262] = {
    //     0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
    /*0*/  4, 10,  7,  6,  4,  4,  7,  4,  4, 11,  7,  6,  4,  4,  7,  4,
    /*1*/  8, 10,  7,  6,  4,  4,  7,  4, 12, 11,  7,  6,  4,  4,  7,  4, //check 18
    /*2*/  7, 10, 16,  6,  4,  4,  7,  4,  7, 11, 16,  6,  4,  4,  7,  4,
    /*3*/  7, 10, 13,  6, 11, 11, 10,  4,  7, 11, 13,  6,  4,  4,  7,  4,
    /*4*/  4,  4,  4,  4,  4,  4,  7,  4,  4,  4,  4,  4,  4,  4,  7,  4,
    /*5*/  4,  4,  4,  4,  4,  4,  7,  4,  4,  4,  4,  4,  4,  4,  7,  4,
    /*6*/  4,  4,  4,  4,  4,  4,  7,  4,  4,  4,  4,  4,  4,  4,  7,  4,
    /*7*/  7,  7,  7,  7,  7,  7,  4,  7,  4,  4,  4,  4,  4,  4,  7,  4,
    /*8*/  4,  4,  4,  4,  4,  4,  7,  4,  4,  4,  4,  4,  4,  4,  7,  4,
    /*9*/  4,  4,  4,  4,  4,  4,  7,  4,  4,  4,  4,  4,  4,  4,  7,  4,
    /*A*/  4,  4,  4,  4,  4,  4,  7,  4,  4,  4,  4,  4,  4,  4,  7,  4,
    /*B*/  4,  4,  4,  4,  4,  4,  7,  4,  4,  4,  4,  4,  4,  4,  7,  4,
    /*C*/  5, 10, 10, 10, 10, 11,  7, 11,  5, 10, 10, 99, 19, 17,  7, 11, //check if 99 impacts anything
    /*D*/  5, 10, 10, 11, 10, 11,  7, 11,  5,  4, 10, 11, 10, 99,  7, 11,
    /*E*/  5, 10, 10, 19, 10, 11,  7, 11,  5,  4, 10,  4, 10, 99,  7, 11,
    /*F*/  5, 10, 10,  4, 10, 11,  7, 11,  5,  6, 10,  4, 10, 99,  7, 11,
    /*10*/ 1, 13, 13,  4, 11, 13 //check value
    //check value for im2 (0x102).  setting to 13 for now, but it likely should be higher
};

const int c_z80::cb_cycle_table[256] = {
    //     0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
    /*0*/  8,  8,  8,  8,  8,  8, 15,  8,  8,  8,  8,  8,  8,  8, 15,  8,
    /*1*/  8,  8,  8,  8,  8,  8, 15,  8,  8,  8,  8,  8,  8,  8, 15,  8,
    /*2*/  8,  8,  8,  8,  8,  8, 15,  8,  8,  8,  8,  8,  8,  8, 15,  8,
    /*3*/  8,  8,  8,  8,  8,  8, 15,  8,  8,  8,  8,  8,  8,  8, 15,  8,
    /*4*/  8,  8,  8,  8,  8,  8, 12,  8,  8,  8,  8,  8,  8,  8, 12,  8,
    /*5*/  8,  8,  8,  8,  8,  8, 12,  8,  8,  8,  8,  8,  8,  8, 12,  8,
    /*6*/  8,  8,  8,  8,  8,  8, 12,  8,  8,  8,  8,  8,  8,  8, 12,  8,
    /*7*/  8,  8,  8,  8,  8,  8, 12,  8,  8,  8,  8,  8,  8,  8, 12,  8,
    /*8*/  8,  8,  8,  8,  8,  8, 15,  8,  8,  8,  8,  8,  8,  8, 15,  8,
    /*9*/  8,  8,  8,  8,  8,  8, 15,  8,  8,  8,  8,  8,  8,  8, 15,  8,
    /*A*/  8,  8,  8,  8,  8,  8, 15,  8,  8,  8,  8,  8,  8,  8, 15,  8,
    /*B*/  8,  8,  8,  8,  8,  8, 15,  8,  8,  8,  8,  8,  8,  8, 15,  8,
    /*C*/  8,  8,  8,  8,  8,  8, 15,  8,  8,  8,  8,  8,  8,  8, 15,  8,
    /*D*/  8,  8,  8,  8,  8,  8, 15,  8,  8,  8,  8,  8,  8,  8, 15,  8,
    /*E*/  8,  8,  8,  8,  8,  8, 15,  8,  8,  8,  8,  8,  8,  8, 15,  8,
    /*F*/  8,  8,  8,  8,  8,  8, 15,  8,  8,  8,  8,  8,  8,  8, 15,  8,
};

const int c_z80::dd_cycle_table[256] = {
    //     0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
    /*0*/  1,  1,  1,  1,  1,  1,  1,  1,  1, 15,  1,  1,  1,  1,  1,  1,
    /*1*/  1,  1,  1,  1,  1,  1,  1,  1,  1, 15,  1,  1,  1,  1,  1,  1,
    /*2*/  1, 14, 20, 10,  8,  8, 11,  1,  1, 15, 20, 10,  8,  8, 11,  1,
    /*3*/  1,  1,  1,  1, 23, 23, 19,  1,  1, 15,  1,  1,  1,  1,  1,  1,
    /*4*/  1,  1,  1,  1,  8,  8, 19,  1,  1,  1,  1,  1,  8,  8, 19,  1,
    /*5*/  1,  1,  1,  1,  8,  8, 19,  1,  1,  1,  1,  1,  8,  8, 19,  1,
    /*6*/  8,  8,  8,  8,  8,  8, 19,  8,  8,  8,  8,  8,  8,  8, 19,  8,
    /*7*/ 19, 19, 19, 19, 19, 19,  1, 19,  1,  1,  1,  1,  8,  8, 19,  1,
    /*8*/  1,  1,  1,  1,  8,  8, 19,  1,  1,  1,  1,  1,  8,  8, 19,  1,
    /*9*/  1,  1,  1,  1,  8,  8, 19,  1,  1,  1,  1,  1,  8,  8, 19,  1,
    /*A*/  1,  1,  1,  1,  8,  8, 19,  1,  1,  1,  1,  1,  8,  8, 19,  1,
    /*B*/  1,  1,  1,  1,  8,  8, 19,  1,  1,  1,  1,  1,  8,  8, 19,  1,
    /*C*/  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1, 99,  1,  1,  1,
    /*D*/  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
    /*E*/  1, 14,  1, 23,  1, 15,  1,  1,  1,  8,  1,  1,  1,  1,  1,  1,
    /*F*/  1,  1,  1,  1,  1,  1,  1,  1,  1, 10,  1,  1,  1,  1,  1,  1,
};

const int c_z80::ed_cycle_table[256] = {
    //     0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
    /*0*/  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
    /*1*/  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
    /*2*/  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
    /*3*/  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
    /*4*/ 12, 12, 15, 20,  8, 14,  8,  9, 12, 12, 15, 20,  8, 14,  8,  9,
    /*5*/ 12, 12, 15, 20,  8, 14,  8,  9, 12, 12, 15, 20,  8, 14,  8,  9,
    /*6*/ 12, 12, 15, 20,  8, 14,  8, 18, 12, 12, 15, 20,  8, 14,  8, 18,
    /*7*/ 12, 12, 15, 20,  8, 14,  8,  1, 12, 12, 15, 20,  8, 14,  8,  1,
    /*8*/  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
    /*9*/  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
    /*A*/ 16, 16, 16, 16,  1,  1,  1,  1, 16, 16, 16, 16,  1,  1,  1,  1,
    /*B*/ 16, 16, 16, 16,  1,  1,  1,  1, 16, 16, 16, 16,  1,  1,  1,  1,
    /*C*/  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
    /*D*/  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
    /*E*/  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
    /*F*/  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
};

const int c_z80::ddcb_cycle_table[256] = {
    //     0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
    /*0*/ 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
    /*1*/ 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
    /*2*/ 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
    /*3*/ 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
    /*4*/ 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
    /*5*/ 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
    /*6*/ 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
    /*7*/ 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
    /*8*/ 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
    /*9*/ 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
    /*A*/ 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
    /*B*/ 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
    /*C*/ 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
    /*D*/ 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
    /*E*/ 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
    /*F*/ 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
};

// clang-format on