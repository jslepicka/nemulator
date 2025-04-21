module;
export module m68k;

import std;
using int8_t = std::int8_t;
using int16_t = std::int16_t;
using int32_t = std::int32_t;
using int64_t = std::int64_t;

using uint8_t = std::uint8_t;
using uint16_t = std::uint16_t;
using uint32_t = std::uint32_t;
using uint64_t = std::uint64_t;

export class c_m68k
{
    typedef std::function<uint8_t(uint32_t)> read_byte_t;
    typedef std::function<void(uint32_t, uint8_t)> write_byte_t;
    typedef std::function<uint16_t(uint32_t)> read_word_t;
    typedef std::function<void(uint32_t, uint16_t)> write_word_t;

    typedef std::function<void(c_m68k*)> opcode_t;

public:
    c_m68k(
        read_word_t read_word,
        write_word_t write_word,
        read_byte_t read_byte,
        write_byte_t write_byte,
        uint8_t *ipl
    );
    void reset();
    bool test();
    void decode();
    void set_flags();
    void update_status();
    void execute(int cycles);

  private:
    uint8_t *ipl;
    int available_cycles;
    int fetch_opcode;
    int required_cycles;
    read_word_t read_word_cb;
    write_word_t write_word_cb;
    read_byte_t read_byte_cb;
    write_byte_t write_byte_cb;

    uint16_t read_word(uint32_t address)
    {
        return read_word_cb(address & 0xFFFFFF);
    }
    uint8_t read_byte(uint32_t address)
    {
        return read_byte_cb(address & 0xFFFFFF);
    }
    void write_word(uint32_t address, uint16_t data)
    {
        write_word_cb(address & 0xFFFFFF, data);
    }
    void write_byte(uint32_t address, uint8_t data)
    {
        write_byte_cb(address & 0xFFFFFF, data);
    }

    opcode_t opcode_fn;

    enum INSTRUCTION
    {
        INVALID, ORItoCCR, ORItoSR, ORI, ANDItoCCR, ANDItoSR, ANDI, SUBI,
        ADDI, EORItoCCR, EORItoSR, EORI, CMPI, BTST, BCHG, BCLR,
        BSET, BTST2, BCHG2, BCLR2, BSET2, MOVEP, MOVEA, MOVE,
        MOVEfromSR, MOVEtoCCR, MOVEtoSR, NEGX, CLR, NEG, NOT, EXT,
        NBCD, SWAP, PEA, ILLEGAL, TAS, TST, TRAP, LINK,
        UNLK, MOVE_USP, RESET, NOP, STOP, RTE, RTS, TRAPV,
        RTR, JSR, JMP, MOVEM, LEA, CHK, ADDQ, SUBQ, Scc,
        DBcc, BRA, BSR, Bcc, MOVEQ, DIVU, DIVS, SBCD,
        OR, SUB, SUBX, SUBA, EOR, CMPM, CMP, CMPA,
        MULU, MULS, ABCD, EXGd, EXGa, EXGda, AND, ADD,
        ADDX, ADDA, ASd, LSd, ROXd, ROd, ASd2, LSd2,
        ROXd2, ROd2, INTERRUPT_4, INTERRUPT_6
    };

    enum INSTRUCTION_SIZE
    {
        SIZE_BYTE,
        SIZE_WORD,
        SIZE_LONG
    };

    enum class ADDRESS_MODE
    {
        DATA_REGISTER,
        ADDRESS_REGISTER,
        ADDRESS,
        ADDRESS_POSTINCREMENT,
        ADDRESS_PREINCREMENT,
        ADDRESS_DISPLACEMENT,
        ADDRESS_INDEX,
        PC_DISPLACEMENT,
        PC_INDEX,
        ABS_SHORT,
        ABS_LONG,
        IMMEDIATE
    };

    enum class CONDITION
    {
        T,
        F,
        HI,
        LS,
        CC,
        CS,
        NE,
        EQ,
        VC,
        VS,
        PL,
        MI,
        GE,
        LT,
        GT,
        LE
    };

    uint8_t instruction;
    uint8_t op_size;
    uint16_t op_word;
    uint32_t ea;
    int32_t address_increment;
    uint32_t *address_increment_target;
    uint32_t* register_target;

    ADDRESS_MODE address_mode;

    static const uint8_t instructions[65536];

    uint16_t sr;
    uint32_t d0;
    uint32_t d1;
    uint32_t d2;
    uint32_t d3;
    uint32_t d4;
    uint32_t d5;
    uint32_t d6;
    uint32_t d7;
    uint32_t* d[8];

    uint32_t a0;
    uint32_t a1;
    uint32_t a2;
    uint32_t a3;
    uint32_t a4;
    uint32_t a5;
    uint32_t a6;
    uint32_t* a[8];

    uint32_t usp; //a7 or usp
    uint32_t ssp;

    uint32_t pc;

    uint32_t flag_c;
    uint32_t flag_v;
    uint32_t flag_z;
    uint32_t flag_n;
    uint32_t flag_x;

    uint32_t mask;
    uint32_t highest_bit;
    uint32_t ea_indirect;

    union {
        struct {
#pragma pack(push, 1)
            int8_t d8 : 8;
            uint8_t : 3;
            uint8_t size : 1;
            uint8_t reg : 3;
            uint8_t d_a : 1;
#pragma pack(pop)
        };
        uint16_t value;
    } extension_word;

    void ASd_();
    void ASd2_();
    void AND_();
    void ANDI_();
    void LEA_();
    void OR_();
    void ORI_();
    void EOR_();
    void EORI_();
    void NOT_();
    void NEG_();
    void NEGX_();
    void BTST_();
    void BTST2_();
    void BITOP_(std::function<uint32_t(uint32_t,uint32_t)>);
    void BITOP2_(std::function<uint32_t(uint32_t, uint32_t)>);
    void CLR_();
    void EXG_();
    void TST_();
    void LSd_();
    void LSd2_();
    void JMP_();
    void JSR_();
    void SWAP_();
    void ADDX_();
    void SUBX_();
    void ADD_();
    void SUB_();
    void ADDQ_();
    void SUBQ_();
    void ADDI_();
    void SUBI_();
    void ADDA_();
    void SUBA_();
    void MOVE_();
    void MOVEQ_();
    void MOVE_USP_();
    void MOVEtoCCR_();
    void MOVEfromSR_();
    void MOVEtoSR_();
    void MOVEA_();
    void ROd_();
    void ROd2_();
    void ROXd_();
    void ROXd2_();
    void NOP_();
    void CMP_();
    void CMPI_();
    void CMPM_();
    void ANDItoCCR_();
    void ANDItoSR_();
    void ORItoCCR_();
    void ORItoSR_();
    void EORItoCCR_();
    void EORItoSR_();
    void CMPA_();
    void PEA_();
    void TAS_();
    void Bcc_();
    void BRA_();
    void BSR_();
    void CHK_();
    void TRAP_();
    void TRAPV_();
    void RESET_();
    void RTE_();
    void Scc_();
    void RTS_();
    void RTR_();
    void LINK_();
    void UNLK_();
    void EXT_();
    void DBcc_();
    void MOVEM_();
    void MOVEP_();
    void ABCD_();
    void SBCD_();
    void NBCD_();
    void MULU_();
    void MULS_();
    void DIVU_();
    void DIVS_();
    void INTERRUPT_(int number);

    void do_trap(uint32_t vector);

    void set_size(int size);

    void get_size1();
    void get_size3();
    void get_address_mode();
    uint32_t compute_ea();
    uint32_t read_ea();
    void write_ea(uint32_t value);
    void preincrement_ea();
    void postincrement_ea();
    void writeback_ea();
    uint32_t get_immediate();
    uint8_t bcd_adc(uint8_t x, uint8_t y);
    uint8_t bcd_sbc(uint8_t dst, uint8_t src);

    bool test_condition(CONDITION condition);

    uint32_t M;
    uint32_t Xn;
    uint32_t immediate_size;

    uint8_t interrupt;

};
