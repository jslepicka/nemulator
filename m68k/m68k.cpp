module;
#include <assert.h>
#include <Windows.h>
#include <cstdio>
module m68k;

c_m68k::c_m68k(
    read_word_t read_word,
    write_word_t write_word,
    read_byte_t read_byte,
    write_byte_t write_byte,
    ack_irq_t ack_irq,
    uint8_t *ipl,
    uint32_t *stalled
)
{
    this->ack_irq = ack_irq;
    this->stalled = stalled;
    this->ipl = ipl;
    this->read_word_cb = read_word;
    this->write_word_cb = write_word;
    this->read_byte_cb = read_byte;
    this->write_byte_cb = write_byte;
    d[0] = &d0;
    d[1] = &d1;
    d[2] = &d2;
    d[3] = &d3;
    d[4] = &d4;
    d[5] = &d5;
    d[6] = &d6;
    d[7] = &d7;

    a[0] = &a0;
    a[1] = &a1;
    a[2] = &a2;
    a[3] = &a3;
    a[4] = &a4;
    a[5] = &a5;
    a[6] = &a6;
    a[7] = &usp;
}

void c_m68k::reset()
{
    sr = 0x2000;
    d0 = 0;
    d1 = 0;
    d2 = 0;
    d3 = 0;
    d4 = 0;
    d5 = 0;
    d6 = 0;
    d7 = 0;

    a0 = 0;
    a1 = 0;
    a2 = 0;
    a3 = 0;
    a4 = 0;
    a5 = 0;
    a6 = 0;

    usp = 0;
    ssp = 0;
    pc = 0;

    /*flag_v = 0;
    flag_z = 0;
    flag_n = 0;
    flag_x = 0;*/

    set_flags();
    a[7] = &ssp;
    ssp = (read_word(0) << 16);
    ssp |= read_word(2);
    pc = (read_word(4) << 16);
    pc |= read_word(6);
    int x = 1;
    fetch_opcode = 1;
    available_cycles = 0;
    required_cycles = 0;
    interrupt = 0;
    stopped = false;
}

void c_m68k::execute(int cycles)
{
    available_cycles += cycles;
    while (true) {
        if (pc == 0x113A) {
            int x = 1;
        }
        if (*stalled) {
            available_cycles--;
            (*stalled)--;
            continue;
        }
        if (fetch_opcode) {
            uint8_t i = *ipl & 0x7;
            uint8_t mask = (sr >> 8) & 0x7;
            if (i && i > mask) {
                stopped = false;
                interrupt = i;
                required_cycles = 44;
                fetch_opcode = 0;
            }
            else if (!stopped) {
                op_word = read_word(pc);
                pc += 2;
                instruction = instructions3[op_word];
                required_cycles = 0;
                op_size = SIZE_UNKNOWN;
                address_mode = ADDRESS_MODE::UNKNOWN;
                decode();
                //assert(opcode_fn != nullptr);
                if (required_cycles == 0) {
                    required_cycles = 4;
                }
                //required_cycles = 8;
                fetch_opcode = 0;
            }
            //fetch_opcode = 0;
        }
        if (required_cycles <= available_cycles) {
            available_cycles -= required_cycles;
            if (!stopped) {
                fetch_opcode = 1;
                if (interrupt) {
                    do_trap(24 + interrupt);
                    //do_trap already calls update_status, so no need to do that again
                    sr &= 0xF8FF;
                    sr |= (interrupt << 8);
                    interrupt = 0;
                    ack_irq();
                }
                else {
                    if (opcode_fn == nullptr) {
                        do_trap(4);
                    }
                    else {
                        opcode_fn(this);
                    }
                    //shouldn't be required unless I've missed somewhere else
                    update_status();
                }
            }
        }
        else {
            return;
        }
    }
}

bool c_m68k::test()
{
    set_flags();
    update_status();
    op_word = read_word(pc);
    pc += 2;
    instruction = instructions3[op_word];
    decode();
    if (opcode_fn == nullptr) {
        return false;
    }
    opcode_fn(this);
    update_status();
    //pc += 4; //new tests expect pc to be advanced by prefetch?
    if (pc & 1) {
        //do_trap(3);
    }
    return true;
}

void c_m68k::set_flags() {
    flag_c = sr & 0x01;
    flag_v = sr & 0x02;
    flag_z = sr & 0x04;
    flag_n = sr & 0x08;
    flag_x = sr & 0x10;
}

void c_m68k::update_status() {
    uint16_t new_sr = sr & 0xFFE0;
    new_sr |= flag_c ? 0x01 : 0;
    new_sr |= flag_v ? 0x02 : 0;
    new_sr |= flag_z ? 0x04 : 0;
    new_sr |= flag_n ? 0x08 : 0;
    new_sr |= flag_x ? 0x10 : 0;

    //set a7 depending on supervisor bit
    if (sr & 0x2000) {
        a[7] = &ssp;
    }
    else {
        a[7] = &usp;
    }
    sr = new_sr;
}

void c_m68k::decode()
{
    //this function should figure out everything required to execute the opcode when ready (enough cycles available)
    //and then set a function pointer to the opcode handler
    //that includes:
    // size
    // addresisng mode
    // required number of cycles
    //
    
    uint16_t size_temp;
    switch (instruction) {
    case ASd:
        opcode_fn = std::mem_fn(&c_m68k::ASd_);
        //memory
        get_size1();
        get_address_mode();
        required_cycles = 8;
        required_cycles += get_ea_cycles();
        break;
    case ASd2:
        opcode_fn = std::mem_fn(&c_m68k::ASd2_);
        {
            get_size1();
            uint32_t shift_count = (op_word >> 9) & 0x7;
            required_cycles = op_size == SIZE_LONG ? 8 : 6;
            required_cycles += shift_count * 2;
        }
        break;
    case AND:
        opcode_fn = std::mem_fn(&c_m68k::AND_);
        get_size1();
        get_address_mode();
        if (op_word & 0x100) {
            //to ea
            required_cycles = op_size == SIZE_LONG ? 12 : 8;
        }
        else {
            if (op_size == SIZE_LONG) {
                required_cycles = 6;
                if (address_mode == ADDRESS_MODE::DATA_REGISTER || address_mode == ADDRESS_MODE::ADDRESS_REGISTER ||
                    address_mode == ADDRESS_MODE::IMMEDIATE) {
                    required_cycles += 2;
                }
            }
            else {
                required_cycles = 4;
            }
        }
        required_cycles += get_ea_cycles();
        break;
    case LEA:
        opcode_fn = std::mem_fn(&c_m68k::LEA_);
        get_address_mode();
        switch (address_mode) {
            case ADDRESS_MODE::ADDRESS_REGISTER:
                required_cycles = 4;
                break;
            case ADDRESS_MODE::ADDRESS_DISPLACEMENT:
                required_cycles = 8;
                break;
            case ADDRESS_MODE::ADDRESS_INDEX:
                required_cycles = 12;
                break;
            case ADDRESS_MODE::ABS_SHORT:
                required_cycles = 8;
                break;
            case ADDRESS_MODE::ABS_LONG:
                required_cycles = 12;
                break;
            case ADDRESS_MODE::PC_DISPLACEMENT:
                required_cycles = 8;
                break;
            case ADDRESS_MODE::PC_INDEX:
                required_cycles = 12;
                break;
            default:
                assert(0);
        }
        break;
    case ANDI:
        opcode_fn = std::mem_fn(&c_m68k::ANDI_);
        get_size1();
        get_address_mode();
        if (address_mode == ADDRESS_MODE::DATA_REGISTER) {
            required_cycles = op_size == SIZE_LONG ? 16 : 8;
        }
        else {
            required_cycles = op_size == SIZE_LONG ? 20 : 12;
            required_cycles += get_ea_cycles();
        }
        break;
    case OR:
        opcode_fn = std::mem_fn(&c_m68k::OR_);
        get_size1();
        get_address_mode();
        if (op_word & 0x100) {
            //to ea
            required_cycles = op_size == SIZE_LONG ? 12 : 8;
        }
        else {
            if (op_size == SIZE_LONG) {
                required_cycles = 6;
                if (address_mode == ADDRESS_MODE::DATA_REGISTER || address_mode == ADDRESS_MODE::ADDRESS_REGISTER ||
                    address_mode == ADDRESS_MODE::IMMEDIATE) {
                    required_cycles += 2;
                }
            }
            else {
                required_cycles = 4;
            }
        }
        required_cycles += get_ea_cycles();
        break;
    case ORI:
        opcode_fn = std::mem_fn(&c_m68k::ORI_);
        get_size1();
        get_address_mode();
        if (address_mode == ADDRESS_MODE::DATA_REGISTER) {
            required_cycles = op_size == SIZE_LONG ? 16 : 8;
        }
        else {
            required_cycles = op_size == SIZE_LONG ? 20 : 12;
            required_cycles += get_ea_cycles();
        }
        break;
    case EOR:
        opcode_fn = std::mem_fn(&c_m68k::EOR_);
        get_size1();
        get_address_mode();
        if (address_mode == ADDRESS_MODE::DATA_REGISTER || address_mode == ADDRESS_MODE::ADDRESS_REGISTER) {
            required_cycles = op_size == SIZE_LONG ? 8 : 4;
        }
        else {
            required_cycles = op_size == SIZE_LONG ? 12 : 8;
            required_cycles += get_ea_cycles();
        }
        break;
    case EORI:
        opcode_fn = std::mem_fn(&c_m68k::EORI_);
        get_size1();
        get_address_mode();
        if (address_mode == ADDRESS_MODE::DATA_REGISTER) {
            required_cycles = op_size == SIZE_LONG ? 16 : 8;
        }
        else {
            required_cycles = op_size == SIZE_LONG ? 20 : 12;
            required_cycles += get_ea_cycles();
        }
        break;
    case NOT:
        opcode_fn = std::mem_fn(&c_m68k::NOT_);
        get_size1();
        get_address_mode();
        if (address_mode == ADDRESS_MODE::DATA_REGISTER || address_mode == ADDRESS_MODE::ADDRESS_REGISTER) {
            required_cycles = op_size == SIZE_LONG ? 6 : 4;
        }
        else {
            required_cycles = op_size == SIZE_LONG ? 12 : 8;
            required_cycles += get_ea_cycles();
        }
        break;
    case NEG:
        opcode_fn = std::mem_fn(&c_m68k::NEG_);
        get_size1();
        get_address_mode();
        if (address_mode == ADDRESS_MODE::DATA_REGISTER || address_mode == ADDRESS_MODE::ADDRESS_REGISTER) {
            required_cycles = op_size == SIZE_LONG ? 6 : 4;
        }
        else {
            required_cycles = op_size == SIZE_LONG ? 12 : 8;
            required_cycles += get_ea_cycles();
        }
        break;
    case NEGX:
        opcode_fn = std::mem_fn(&c_m68k::NEGX_);
        get_size1();
        get_address_mode();
        if (address_mode == ADDRESS_MODE::DATA_REGISTER || address_mode == ADDRESS_MODE::ADDRESS_REGISTER) {
            required_cycles = op_size == SIZE_LONG ? 6 : 4;
        }
        else {
            required_cycles = op_size == SIZE_LONG ? 12 : 8;
            required_cycles += get_ea_cycles();
        }
        break;
    case BSET:
        opcode_fn = std::bind(&c_m68k::BITOP_, this, [](uint32_t a, uint32_t b) { return a | b; });
        get_address_mode();
        if (ea_indirect) {
            required_cycles = 12;
            required_cycles += get_ea_cycles();
        }
        else {
            required_cycles = 12; //todo - max value
        }
        break;
    case BSET2:
        opcode_fn = std::bind(&c_m68k::BITOP2_, this, [](uint32_t a, uint32_t b) { return a | b; });
        get_address_mode();
        if (ea_indirect) {
            required_cycles = 8;
            required_cycles += get_ea_cycles();
        }
        else {
            required_cycles = 8; //todo - max value
        }
        break;
    case BCLR:
        opcode_fn = std::bind(&c_m68k::BITOP_, this, [](uint32_t a, uint32_t b) { return a & ~b; });
        get_address_mode();
        if (ea_indirect) {
            required_cycles = 12;
            required_cycles += get_ea_cycles();
        }
        else {
            required_cycles = 14; //todo - max value
        }
        break;
    case BCLR2:
        opcode_fn = std::bind(&c_m68k::BITOP2_, this, [](uint32_t a, uint32_t b) { return a & ~b; });
        get_address_mode();
        if (ea_indirect) {
            required_cycles = 8;
            required_cycles += get_ea_cycles();
        }
        else {
            required_cycles = 10; //todo - max value
        }
        break;
        break;
    case BCHG:
        opcode_fn = std::bind(&c_m68k::BITOP_, this, [](uint32_t a, uint32_t b) { return a ^ b; });
        get_address_mode();
        if (ea_indirect) {
            required_cycles = 12;
            required_cycles += get_ea_cycles();
        }
        else {
            required_cycles = 12; //todo - max value
        }
        break;
    case BCHG2:
        opcode_fn = std::bind(&c_m68k::BITOP2_, this, [](uint32_t a, uint32_t b) { return a ^ b; });
        get_address_mode();
        if (ea_indirect) {
            required_cycles = 8;
            required_cycles += get_ea_cycles();
        }
        else {
            required_cycles = 8; //todo - max value
        }
        break;
    case BTST:
        opcode_fn = std::mem_fn(&c_m68k::BTST_);
        get_address_mode();
        if (ea_indirect) {
            required_cycles = 8;
            required_cycles += get_ea_cycles();
        }
        else {
            required_cycles = 10; //todo - max value
        }
        break;
        break;
    case BTST2:
        opcode_fn = std::mem_fn(&c_m68k::BTST2_);
        get_address_mode();
        if (ea_indirect) {
            required_cycles = 4;
            required_cycles += get_ea_cycles();
        }
        else {
            required_cycles = 6; //todo - max value
        }
        break;
        break;
    case CLR:
        opcode_fn = std::mem_fn(&c_m68k::CLR_);
        get_size1();
        get_address_mode();
        if (address_mode == ADDRESS_MODE::DATA_REGISTER || address_mode == ADDRESS_MODE::ADDRESS_REGISTER) {
            required_cycles = op_size == SIZE_LONG ? 6 : 4;
        }
        else {
            required_cycles = op_size == SIZE_LONG ? 12 : 8;
            required_cycles += get_ea_cycles();
        }
        break;
    case EXGd:
    case EXGa:
    case EXGda:
        opcode_fn = std::mem_fn(&c_m68k::EXG_);
        required_cycles = 6;
        break;
    case TST:
        opcode_fn = std::mem_fn(&c_m68k::TST_);
        get_size1();
        get_address_mode();
        if (address_mode == ADDRESS_MODE::DATA_REGISTER || address_mode == ADDRESS_MODE::ADDRESS_REGISTER) {
            required_cycles = 4;
        }
        else {
            required_cycles = 4;
            required_cycles += get_ea_cycles();
        }
        break;
    case LSd:
        opcode_fn = std::mem_fn(&c_m68k::LSd_);
        get_size1();
        get_address_mode();
        required_cycles = 8;
        required_cycles += get_ea_cycles();
        break;
    case LSd2:
        opcode_fn = std::mem_fn(&c_m68k::LSd2_);
        {
            get_size1();
            uint32_t shift_count = (op_word >> 9) & 0x7;
            required_cycles = op_size == SIZE_LONG ? 8 : 6;
            required_cycles += shift_count * 2;
        }
        break;
    case JMP:
        opcode_fn = std::mem_fn(&c_m68k::JMP_);
        get_address_mode();
        switch (address_mode) {
            case ADDRESS_MODE::ADDRESS_REGISTER:
                required_cycles = 8;
                break;
            case ADDRESS_MODE::ADDRESS_DISPLACEMENT:
                required_cycles = 10;
                break;
            case ADDRESS_MODE::ADDRESS_INDEX:
                required_cycles = 14;
                break;
            case ADDRESS_MODE::ABS_SHORT:
                required_cycles = 10;
                break;
            case ADDRESS_MODE::ABS_LONG:
                required_cycles = 12;
                break;
            case ADDRESS_MODE::PC_DISPLACEMENT:
                required_cycles = 10;
                break;
            case ADDRESS_MODE::PC_INDEX:
                required_cycles = 14;
                break;
            default:
                assert(0);
        }
        break;
    case JSR:
        opcode_fn = std::mem_fn(&c_m68k::JSR_);
        get_address_mode();
        switch (address_mode) {
            case ADDRESS_MODE::ADDRESS_REGISTER:
                required_cycles = 16;
                break;
            case ADDRESS_MODE::ADDRESS_DISPLACEMENT:
                required_cycles = 18;
                break;
            case ADDRESS_MODE::ADDRESS_INDEX:
                required_cycles = 22;
                break;
            case ADDRESS_MODE::ABS_SHORT:
                required_cycles = 18;
                break;
            case ADDRESS_MODE::ABS_LONG:
                required_cycles = 20;
                break;
            case ADDRESS_MODE::PC_DISPLACEMENT:
                required_cycles = 18;
                break;
            case ADDRESS_MODE::PC_INDEX:
                required_cycles = 22;
                break;
            default:
                assert(0);
        }
        break;
    case SWAP:
        opcode_fn = std::mem_fn(&c_m68k::SWAP_);
        required_cycles = 4;
        break;
    case SUBX:
        opcode_fn = std::mem_fn(&c_m68k::SUBX_);
        get_size1();
        if ((op_word >> 3) & 0x1) {
            //address
            required_cycles = op_size == SIZE_LONG ? 30 : 18;
        }
        else {
            required_cycles = op_size == SIZE_LONG ? 8 : 4;
        }
        break;
    case SUB:
        opcode_fn = std::mem_fn(&c_m68k::SUB_);
        get_size1();
        get_address_mode();
        if (op_word & 0x100) {
            //to ea
            required_cycles = op_size == SIZE_LONG ? 12 : 8;
        }
        else {
            if (op_size == SIZE_LONG) {
                required_cycles = 6;
                if (address_mode == ADDRESS_MODE::DATA_REGISTER || address_mode == ADDRESS_MODE::ADDRESS_REGISTER ||
                    address_mode == ADDRESS_MODE::IMMEDIATE) {
                    required_cycles += 2;
                }
            }
            else {
                required_cycles = 4;
            }
        }
        required_cycles += get_ea_cycles();
        break;
    case SUBQ:
        opcode_fn = std::mem_fn(&c_m68k::SUBQ_);
        get_size1();
        get_address_mode();
        if (address_mode == ADDRESS_MODE::DATA_REGISTER) {
            required_cycles == op_size == SIZE_LONG ? 8 : 4;
        }
        else if (address_mode == ADDRESS_MODE::ADDRESS_REGISTER) {
            required_cycles = 8;
        }
        else {
            required_cycles = op_size == SIZE_LONG ? 12 : 8;
            required_cycles += get_ea_cycles();
        }
        break;
    case SUBI:
        opcode_fn = std::mem_fn(&c_m68k::SUBI_);
        get_size1();
        get_address_mode();
        if (address_mode == ADDRESS_MODE::DATA_REGISTER) {
            required_cycles = op_size == SIZE_LONG ? 16 : 8;
        }
        else {
            required_cycles = op_size == SIZE_LONG ? 20 : 12;
            required_cycles += get_ea_cycles();
        }
        break;
    case SUBA:
        opcode_fn = std::mem_fn(&c_m68k::SUBA_);
        if (op_word & 0x100) {
            //long
            required_cycles = 6;
            if (address_mode == ADDRESS_MODE::DATA_REGISTER || address_mode == ADDRESS_MODE::ADDRESS_REGISTER ||
                address_mode == ADDRESS_MODE::IMMEDIATE) {
                required_cycles += 2;
            }
        }
        else {
            //word
            required_cycles = 8;
        }
        required_cycles += get_ea_cycles();
        break;
    case MOVE:
        opcode_fn = std::mem_fn(&c_m68k::MOVE_);
        break;
    case MOVEQ:
        opcode_fn = std::mem_fn(&c_m68k::MOVEQ_);
        required_cycles = 4;
        break;
    case MOVE_USP:
        opcode_fn = std::mem_fn(&c_m68k::MOVE_USP_);
        required_cycles = 4;
        break;
    case MOVEtoCCR:
        opcode_fn = std::mem_fn(&c_m68k::MOVEtoCCR_);
        break;
    case MOVEfromSR:
        opcode_fn = std::mem_fn(&c_m68k::MOVEfromSR_);
        break;
    case MOVEtoSR:
        opcode_fn = std::mem_fn(&c_m68k::MOVEtoSR_);
        break;
    case MOVEA:
        opcode_fn = std::mem_fn(&c_m68k::MOVEA_);
        break;
    case ROd:
        opcode_fn = std::mem_fn(&c_m68k::ROd_);
        get_size1();
        get_address_mode();
        required_cycles = 8;
        required_cycles += get_ea_cycles();
        break;
    case ROd2:
        opcode_fn = std::mem_fn(&c_m68k::ROd2_);
        {
            get_size1();
            uint32_t shift_count = (op_word >> 9) & 0x7;
            required_cycles = op_size == SIZE_LONG ? 8 : 6;
            required_cycles += shift_count * 2;
        }
        break;
    case ROXd:
        opcode_fn = std::mem_fn(&c_m68k::ROXd_);
        get_size1();
        get_address_mode();
        required_cycles = 8;
        required_cycles += get_ea_cycles();
        break;
    case ROXd2:
        opcode_fn = std::mem_fn(&c_m68k::ROXd2_);
        {
            get_size1();
            uint32_t shift_count = (op_word >> 9) & 0x7;
            required_cycles = op_size == SIZE_LONG ? 8 : 6;
            required_cycles += shift_count * 2;
        }
        break;
    case ADD:
        opcode_fn = std::mem_fn(&c_m68k::ADD_);
        get_size1();
        get_address_mode();
        if (op_word & 0x100) {
            //to ea
            required_cycles = op_size == SIZE_LONG ? 12 : 8;
        }
        else {
            if (op_size == SIZE_LONG) {
                required_cycles = 6;
                if (address_mode == ADDRESS_MODE::DATA_REGISTER || address_mode == ADDRESS_MODE::ADDRESS_REGISTER ||
                    address_mode == ADDRESS_MODE::IMMEDIATE) {
                    required_cycles += 2; 
                }
            }
            else {
                required_cycles = 4;
            }
        }
        required_cycles += get_ea_cycles();
        break;
    case ADDI:
        opcode_fn = std::mem_fn(&c_m68k::ADDI_);
        get_size1();
        get_address_mode();
        if (address_mode == ADDRESS_MODE::DATA_REGISTER) {
            required_cycles = op_size == SIZE_LONG ? 16 : 8;
        }
        else {
            required_cycles = op_size == SIZE_LONG ? 20 : 12;
            required_cycles += get_ea_cycles();
        }
        break;
    case ADDQ:
        opcode_fn = std::mem_fn(&c_m68k::ADDQ_);
        get_size1();
        get_address_mode();
        if (address_mode == ADDRESS_MODE::DATA_REGISTER) {
            required_cycles == op_size == SIZE_LONG ? 8 : 4;
        }
        else if (address_mode == ADDRESS_MODE::ADDRESS_REGISTER) {
            required_cycles = 8;
        }
        else {
            required_cycles = 12;
            required_cycles += get_ea_cycles();
        }
        break;
    case ADDA:
        opcode_fn = std::mem_fn(&c_m68k::ADDA_);
        if (op_word & 0x100) {
            //long
            required_cycles = 6;
            if (address_mode == ADDRESS_MODE::DATA_REGISTER || address_mode == ADDRESS_MODE::ADDRESS_REGISTER ||
                address_mode == ADDRESS_MODE::IMMEDIATE) {
                required_cycles += 2;
            }
        }
        else {
            //word
            required_cycles = 8;
        }
        required_cycles += get_ea_cycles();
        break;
    case ADDX:
        opcode_fn = std::mem_fn(&c_m68k::ADDX_);
        get_size1();
        if ((op_word >> 3) & 0x1) {
            //address
            required_cycles = op_size == SIZE_LONG ? 30 : 18;
        }
        else {
            required_cycles = op_size == SIZE_LONG ? 8 : 4;
        }
        break;
    case NOP:
        opcode_fn = std::mem_fn(&c_m68k::NOP_);
        required_cycles = 4;
        break;
    case CMP:
        opcode_fn = std::mem_fn(&c_m68k::CMP_);
        get_size1();
        get_address_mode();
        required_cycles == op_size == SIZE_LONG ? 6 : 4;
        required_cycles += get_ea_cycles();
        break;
    case CMPI:
        opcode_fn = std::mem_fn(&c_m68k::CMPI_);
        get_size1();
        get_address_mode();
        if (address_mode == ADDRESS_MODE::DATA_REGISTER) {
            required_cycles = op_size == SIZE_LONG ? 14 : 8;
        }
        else {
            required_cycles = op_size == SIZE_LONG ? 12 : 8;
            required_cycles += get_ea_cycles();
        }
        break;
    case CMPM:
        opcode_fn = std::mem_fn(&c_m68k::CMPM_);
        get_size1();
        required_cycles = op_size == SIZE_LONG ? 20 : 12;
        break;
    case ANDItoCCR:
        opcode_fn = std::mem_fn(&c_m68k::ANDItoCCR_);
        required_cycles = 20;
        break;
    case ANDItoSR:
        opcode_fn = std::mem_fn(&c_m68k::ANDItoSR_);
        required_cycles = 20;
        break;
    case ORItoCCR:
        opcode_fn = std::mem_fn(&c_m68k::ORItoCCR_);
        required_cycles = 20;
        break;
    case ORItoSR:
        opcode_fn = std::mem_fn(&c_m68k::ORItoSR_);
        required_cycles = 20;
        break;
    case EORItoCCR:
        opcode_fn = std::mem_fn(&c_m68k::EORItoCCR_);
        required_cycles = 20;
        break;
    case EORItoSR:
        opcode_fn = std::mem_fn(&c_m68k::EORItoSR_);
        required_cycles = 20;
        break;
    case CMPA:
        opcode_fn = std::mem_fn(&c_m68k::CMPA_);
        get_address_mode();
        required_cycles = 6;
        required_cycles += get_ea_cycles();
        break;
    case PEA:
        opcode_fn = std::mem_fn(&c_m68k::PEA_);
        get_address_mode();
        switch (address_mode) {
            case ADDRESS_MODE::ADDRESS_REGISTER:
                required_cycles = 12;
                break;
            case ADDRESS_MODE::ADDRESS_DISPLACEMENT:
                required_cycles = 16;
                break;
            case ADDRESS_MODE::ADDRESS_INDEX:
                required_cycles = 20;
                break;
            case ADDRESS_MODE::ABS_SHORT:
                required_cycles = 16;
                break;
            case ADDRESS_MODE::ABS_LONG:
                required_cycles = 20;
                break;
            case ADDRESS_MODE::PC_DISPLACEMENT:
                required_cycles = 16;
                break;
            case ADDRESS_MODE::PC_INDEX:
                required_cycles = 20;
                break;
            default:
                assert(0);
        }
        break;
    case TAS:
        opcode_fn = std::mem_fn(&c_m68k::TAS_);
        get_address_mode();
        if (address_mode == ADDRESS_MODE::DATA_REGISTER || address_mode == ADDRESS_MODE::ADDRESS_REGISTER) {
            required_cycles = 4;
        }
        else {
            required_cycles = 10;
            required_cycles += get_ea_cycles();
        }
        break;
    case Bcc:
        opcode_fn = std::mem_fn(&c_m68k::Bcc_);
        required_cycles = 8;
        break;
    case BRA:
        opcode_fn = std::mem_fn(&c_m68k::BRA_);
        required_cycles = 10;
        break;
    case BSR:
        opcode_fn = std::mem_fn(&c_m68k::BSR_);
        required_cycles = 18;
        break;
    case CHK:
        opcode_fn = std::mem_fn(&c_m68k::CHK_);
        required_cycles = 10;
        // need to add + calc
        break;
    case TRAP:
        opcode_fn = std::mem_fn(&c_m68k::TRAP_);
        required_cycles = 38;
        break;
    case TRAPV:
        opcode_fn = std::mem_fn(&c_m68k::TRAPV_);
        required_cycles = flag_v ? 38 : 4;
        break;
    case RESET:
        opcode_fn = std::mem_fn(&c_m68k::RESET_);
        required_cycles = 132;
        break;
    case RTE:
        opcode_fn = std::mem_fn(&c_m68k::RTE_);
        required_cycles = 20;
        break;
    case Scc:
        opcode_fn = std::mem_fn(&c_m68k::Scc_);
        get_address_mode();
        if (address_mode == ADDRESS_MODE::DATA_REGISTER || address_mode == ADDRESS_MODE::ADDRESS_REGISTER) {
            required_cycles = 4;
        }
        else {
            required_cycles = 8;
            required_cycles += get_ea_cycles();
        }
        break;
    case RTS:
        opcode_fn = std::mem_fn(&c_m68k::RTS_);
        required_cycles = 16;
        break;
    case RTR:
        opcode_fn = std::mem_fn(&c_m68k::RTR_);
        required_cycles = 20;
        break;
    case LINK:
        opcode_fn = std::mem_fn(&c_m68k::LINK_);
        required_cycles = 16;
        break;
    case UNLK:
        opcode_fn = std::mem_fn(&c_m68k::UNLK_);
        required_cycles = 12;
        break;
    case EXT:
        opcode_fn = std::mem_fn(&c_m68k::EXT_);
        required_cycles = 4;
        break;
    case DBcc:
        opcode_fn = std::mem_fn(&c_m68k::DBcc_);
        required_cycles = 10;
        //adjusted in function
        break;
    case MOVEM:
        opcode_fn = std::mem_fn(&c_m68k::MOVEM_);
        get_address_mode();
        if (op_word & 0x400) {
            //M -> R
            switch (address_mode) {
                case ADDRESS_MODE::ADDRESS_REGISTER:
                    required_cycles = 12;
                    break;
                case ADDRESS_MODE::ADDRESS_POSTINCREMENT:
                    required_cycles = 12;
                    break;
                case ADDRESS_MODE::ADDRESS_DISPLACEMENT:
                    required_cycles = 16;
                    break;
                case ADDRESS_MODE::ADDRESS_INDEX:
                    required_cycles = 18;
                    break;
                case ADDRESS_MODE::ABS_SHORT:
                    required_cycles = 16;
                    break;
                case ADDRESS_MODE::ABS_LONG:
                    required_cycles = 20;
                    break;
                case ADDRESS_MODE::PC_DISPLACEMENT:
                    required_cycles = 16;
                    break;
                case ADDRESS_MODE::PC_INDEX:
                    required_cycles = 18;
                    break;
                default:
                    assert(0);
            }
        }
        else {
            //R -> M
            switch (address_mode) {
                case ADDRESS_MODE::ADDRESS_REGISTER:
                    required_cycles = 8;
                    break;
                case ADDRESS_MODE::ADDRESS_PREINCREMENT:
                    required_cycles = 8;
                    break;
                case ADDRESS_MODE::ADDRESS_DISPLACEMENT:
                    required_cycles = 12;
                    break;
                case ADDRESS_MODE::ADDRESS_INDEX:
                    required_cycles = 14;
                    break;
                case ADDRESS_MODE::ABS_SHORT:
                    required_cycles = 12;
                    break;
                case ADDRESS_MODE::ABS_LONG:
                    required_cycles = 16;
                    break;
                default:
                    assert(0);
            }
        }
        {
            //todo -- doing this again in the instruction; optimize?
            set_size(SIZE_WORD);
            uint16_t register_list = get_immediate();
            int num_registers = std::popcount(register_list);
            if (op_word & 0x40) {
                //long
                required_cycles += 8 * num_registers;
            }
            else {
                required_cycles += 4 * num_registers;
            }
        }
        break;
    case MOVEP:
        opcode_fn = std::mem_fn(&c_m68k::MOVEP_);
        break;
    case ABCD:
        opcode_fn = std::mem_fn(&c_m68k::ABCD_);
        required_cycles = (op_word & 0x8) ? 18 : 6;
        break;
    case SBCD:
        opcode_fn = std::mem_fn(&c_m68k::SBCD_);
        required_cycles = (op_word & 0x8) ? 18 : 6;
        break;
    case NBCD:
        opcode_fn = std::mem_fn(&c_m68k::NBCD_);
        get_address_mode();
        if (address_mode == ADDRESS_MODE::DATA_REGISTER || address_mode == ADDRESS_MODE::ADDRESS_REGISTER) {
            required_cycles = 6;
        }
        else {
            required_cycles = 8;
            required_cycles += get_ea_cycles();
        }
        break;
    case MULU:
        opcode_fn = std::mem_fn(&c_m68k::MULU_);
        get_address_mode();
        required_cycles = 70; //todo - this is max value
        required_cycles += get_ea_cycles();
        break;
    case MULS:
        opcode_fn = std::mem_fn(&c_m68k::MULS_);
        get_address_mode();
        required_cycles = 70; //todo - this is max value
        required_cycles += get_ea_cycles();
        break;
    case DIVU:
        opcode_fn = std::mem_fn(&c_m68k::DIVU_);
        get_address_mode();
        required_cycles = 140; //todo - this is max value
        required_cycles += get_ea_cycles();
        break;
    case DIVS:
        opcode_fn = std::mem_fn(&c_m68k::DIVS_);
        get_address_mode();
        required_cycles = 158; //todo - this is max value
        required_cycles += get_ea_cycles();
        break;
    case STOP:
        opcode_fn = std::mem_fn(&c_m68k::STOP_);
        required_cycles = 4;
        break;
    default:
        opcode_fn = nullptr;
        break;
    }
}

void c_m68k::STOP_()
{
    if (!(sr & 0x2000)) {
        //user mode
        assert(0);
    }
    stopped = true;
    sr = read_word(pc);
    pc += 2;
    set_flags();
    update_status();
}

void c_m68k::DIVS_()
{
    set_size(SIZE_WORD);
    get_address_mode();
    compute_ea();
    preincrement_ea();
    uint32_t dst_reg = (op_word >> 9) & 0x7;
    int32_t dst = *d[dst_reg];
    int32_t src = (int32_t)(int16_t)(read_ea() & 0x0000FFFF);
    postincrement_ea();
    writeback_ea();
    if (src == 0) {
        do_trap(5);
        return;
    }
    int32_t q = dst / src;
    uint16_t r = dst % src;
    flag_c = 0;
    if (q > 32767 || q < -32768) {
        flag_v = 1;
        return;
    }
    *d[dst_reg] = (r << 16) | (q & 0x0000FFFF);
    flag_n = q & highest_bit;
    flag_z = q == 0;
    flag_v = 0;
}

void c_m68k::DIVU_()
{
    //test 5745 differs in flags, but is divide by zero so flags are undefined
    set_size(SIZE_WORD);
    get_address_mode();
    compute_ea();
    preincrement_ea();
    uint32_t dst_reg = (op_word >> 9) & 0x7;
    uint32_t dst = *d[dst_reg];
    uint32_t src = read_ea() & 0x0000FFFF;
    postincrement_ea();
    writeback_ea();
    if (src == 0) {
        do_trap(5);
        return;
    }
    uint32_t q = dst / src;
    uint16_t r = dst % src;
    flag_c = 0;
    if (q > 0x0000FFFF) {
        flag_v = 1;
        return;
    }
    *d[dst_reg] = (r << 16) | (q & 0x0000FFFF);
    flag_n = q & highest_bit;
    flag_z = q == 0;
    flag_v = 0;
}

void c_m68k::MULS_()
{
    set_size(SIZE_WORD);
    get_address_mode();
    compute_ea();
    preincrement_ea();
    uint32_t dst_reg = (op_word >> 9) & 0x7;
    int32_t dst = (int32_t)(int16_t)(*d[dst_reg] & 0x0000FFFF);
    int32_t src = (int32_t)(int16_t)(read_ea() & 0x0000FFFF);
    set_size(SIZE_LONG);
    dst *= src;
    *d[dst_reg] = dst;
    flag_n = dst & highest_bit;
    flag_z = dst == 0;
    flag_v = 0;
    flag_c = 0;
    postincrement_ea();
    writeback_ea();
}

void c_m68k::MULU_()
{
    set_size(SIZE_WORD);
    get_address_mode();
    compute_ea();
    preincrement_ea();
    uint32_t dst_reg = (op_word >> 9) & 0x7;
    uint32_t dst = *d[dst_reg] & 0x0000FFFF;
    uint32_t src = read_ea() & 0x0000FFFF;
    set_size(SIZE_LONG);
    dst *= src;
    *d[dst_reg] = dst;
    flag_n = dst & highest_bit;
    flag_z = dst == 0;
    flag_v = 0;
    flag_c = 0;
    postincrement_ea();
    writeback_ea();
}

uint8_t c_m68k::bcd_adc(uint8_t x, uint8_t y)
{
    uint8_t carry = (flag_x ? 1 : 0);
    uint16_t result = y + x + carry;
    uint32_t half_carry = (y & 0xF) + (x & 0xF) + carry > 0xF;
    carry = result > 0xFF;
    uint32_t correction = 0;

    if (result > 0x99 || carry) {
        correction = 0x60;
        carry = 1;
    }
    else {
        carry = 0;
    }

    if ((result & 0xF) > 0x9 || half_carry) {
        correction |= 0x6;
    }
    result = (result + correction) & 0xFF;
    if (result) {
        flag_z = 0;
    }
    flag_x = flag_c = carry;
    //validate undefined flag behavior
    flag_v = ~result & 0x2;
    flag_n = result & 0x80;
    return result;
}

uint8_t c_m68k::bcd_sbc(uint8_t dst, uint8_t src)
{
    //lots of failures on what is probably invalid input.  need to revisit.
    // 
    //if ((src & 0xF) > 9 || (src & 0xF0) > 0x90 || (dst & 0xF) > 9 || (dst & 0xF0) > 0x90 || (dst & 0xF) > 9 || (dst & 0xF0) > 0x90) {
    //    std::cout << "!!!!!" << std::endl;
    //}
    uint8_t carry = (flag_x ? 1 : 0);
    int16_t result = (int)dst - (int)src - (int)carry;
    uint32_t half_carry = (int)(dst & 0xF) - (int)(src & 0xF) - (int)carry < 0;
    carry = result < 0;
    uint32_t correction = 0;

    if ((result & 0xFF) > 0x99 || carry) {
        correction = 0x60;
        carry = 1;
    }
    else {
        carry = 0;
    }

    if ((result & 0xF) > 0x9 || half_carry) {
        correction |= 0x6;
    }
    result = (result - (int)correction) & 0xFF;


    if (result) {
        flag_z = 0;
    }
    flag_x = flag_c = carry;
    //validate undefined flag behavior
    flag_v = ~result & 0x2;
    flag_n = result & 0x80;

    return result;
}

void c_m68k::NBCD_()
{
    set_size(SIZE_BYTE);
    get_address_mode();
    compute_ea();
    preincrement_ea();
    uint8_t t = bcd_sbc(0, read_ea());
    write_ea(t);
    postincrement_ea();
    writeback_ea();
}

void c_m68k::SBCD_()
{
    set_size(SIZE_BYTE);
    uint32_t src_reg = op_word & 0x7;
    uint32_t dst_reg = (op_word >> 9) & 0x7;
    if (op_word & 0x8) {
        //address register
        address_mode = ADDRESS_MODE::ADDRESS_PREINCREMENT;
        ea_indirect = 1;
        Xn = src_reg;
        compute_ea();
        preincrement_ea();
        uint8_t src = read_ea();
        writeback_ea();
        Xn = dst_reg;
        compute_ea();
        preincrement_ea();
        uint8_t dst = read_ea();
        writeback_ea();
        write_ea(bcd_sbc(dst, src));
    }
    else {
        //data register
        uint8_t dst = *d[dst_reg] & 0xFF;
        uint8_t src = *d[src_reg] & 0xFF;

        uint32_t t = (*d[dst_reg] & 0xFFFFFF00) | bcd_sbc(dst, src);
        *d[dst_reg] = t;
    }
}

void c_m68k::ABCD_()
{
    set_size(SIZE_BYTE);
    uint32_t src_reg = op_word & 0x7;
    uint32_t dst_reg = (op_word >> 9) & 0x7;
    if (op_word & 0x8) {
        //address register
        address_mode = ADDRESS_MODE::ADDRESS_PREINCREMENT;
        ea_indirect = 1;
        Xn = src_reg;
        compute_ea();
        preincrement_ea();
        uint8_t src = read_ea();
        writeback_ea();
        Xn = dst_reg;
        compute_ea();
        preincrement_ea();
        uint8_t dst = read_ea();
        writeback_ea();
        write_ea(bcd_adc(src, dst));
    }
    else {
        //data register
        uint8_t dst = *d[dst_reg];
        uint8_t src = *d[src_reg];
               
        uint32_t t = (*d[dst_reg] & 0xFFFFFF00) | bcd_adc(src, dst);
        *d[dst_reg] = t;
    }
}

void c_m68k::MOVEP_()
{
    uint32_t a_reg = op_word & 0x7;
    uint32_t d_reg = (op_word >> 9) & 0x7;
    uint32_t opmode = (op_word >> 6) & 0x7;
    set_size(SIZE_WORD);
    uint32_t displacement = (int32_t)(int16_t)get_immediate();
    uint32_t address = *a[a_reg] + displacement;
    uint32_t data = *d[d_reg];
    set_size(opmode & 0x1 ? SIZE_LONG : SIZE_WORD);
    if (opmode & 0x2) {
        //reg -> mem
        int x = 1;
        if (op_size == SIZE_WORD) {
            int x = 1;
            write_byte(address, data >> 8);
            write_byte(address + 2, data);
            
        }
        else {
            write_byte(address, data >> 24);
            write_byte(address + 2, data >> 16);
            write_byte(address + 4, data >> 8);
            write_byte(address + 6, data);
        }
    }
    else {
        //mem -> reg
        int x = 1;
        if (op_size == SIZE_WORD) {
            uint16_t temp = read_byte(address) << 8;
            temp |= read_byte(address + 2);
            *d[d_reg] = (data & 0xFFFF0000) | temp;
        }
        else {
           uint32_t temp = read_byte(address) << 24;
           temp |= read_byte(address + 2) << 16;
           temp |= read_byte(address + 4) << 8;
           temp |= read_byte(address + 6);
           *d[d_reg] = temp;
        }
    }
}

void c_m68k::MOVEM_()
{
    set_size(SIZE_WORD);
    uint16_t register_list = get_immediate();
    set_size(op_word & 0x40 ? SIZE_LONG : SIZE_WORD);
    get_address_mode();
    compute_ea();
    bool predecrement = address_mode == ADDRESS_MODE::ADDRESS_PREINCREMENT;
    for (int i = 0; i < 16; i++) {
        uint32_t *r;
        if (register_list & 0x1) {
            preincrement_ea();
            uint32_t** reg;
            if (predecrement) {
                reg = i > 7 ? d : a;
                r = reg[(i & 0x7) ^ 0x7];
            }
            else {
                reg = i > 7 ? a : d;
                r = reg[i & 0x7];
            }

            if (op_word & 0x400) {
                //memory to register
                int x = 1;
                uint32_t v = read_ea();
                if (op_size == SIZE_WORD) {
                    v = (int32_t)(int16_t)v;
                }
                *r = v;
            }
            else {
                //register to memory
                int x = 1;
                write_ea(*r & mask);
            }

            if (address_mode != ADDRESS_MODE::ADDRESS_PREINCREMENT && address_mode != ADDRESS_MODE::ADDRESS_POSTINCREMENT) {
                ea += op_size == SIZE_WORD ? 2 : 4;
            }
            postincrement_ea();
        }
        register_list >>= 1;
    }
    writeback_ea();
}


void c_m68k::DBcc_()
{
    set_size(SIZE_WORD);
    int32_t displacement = (int16_t)get_immediate();
    uint32_t condition = (op_word >> 8) & 0xF;
    uint32_t reg = op_word & 0x7;
    if (!test_condition((CONDITION)condition)) {
        uint16_t count = *d[reg] & 0xFFFF;
        count--;
        *d[reg] = (*d[reg] & 0xFFFF0000) | count;
        if (count != 0xFFFF) {
            pc += displacement - 2;
            //10 total
        }
        else {
            available_cycles -= 4; //14 total
        }
    }
    else {
        available_cycles -= 2; //12 total
    }
}

void c_m68k::EXT_()
{
    uint32_t reg = op_word & 0x7;
    switch ((op_word >> 6) & 0x7) {
        case 0b010:
            //byte -> word
            *d[reg] = (*d[reg] & 0xFFFF0000) | ((int16_t)(int8_t)(*d[reg] & 0xFF) & 0xFFFF);
            flag_n = *d[reg] & 0x8000;
            flag_z = (*d[reg] & 0xFFFF) == 0;
            break;
        case 0b011:
            //word -> long
            *d[reg] = (int32_t)(int16_t)(*d[reg]);
            flag_n = *d[reg] & 0x80000000;
            flag_z = *d[reg] == 0;
            break;
        case 0b111:
            //byte -> long
            assert(0);
            break;
        default:
            assert(0);
            break;
    }
    flag_v = 0;
    flag_c = 0;
}

void c_m68k::UNLK_()
{
    //failing on A7
    set_size(SIZE_WORD);
    uint32_t reg = op_word & 0x7;
    if (reg == 0x7) {
        //assert(0);
    }
    *a[7] = *a[reg];
    uint32_t sp = *a[7];
    *a[reg] = read_word(sp) << 16;
    *a[reg] |= read_word(sp + 2);
    *a[7] = sp + 4;
}

void c_m68k::LINK_()
{
    //a7 tests failing -- what is correct behavior?
    set_size(SIZE_WORD);
    uint32_t reg = op_word & 0x7;
    if (reg == 0x7) {
        //assert(0);
    }
    uint32_t An = *a[reg];
    *a[7] -= 4;
    write_word(*a[7], An >> 16);
    write_word(*a[7] + 2, An & 0xFFFF);
    *a[reg] = *a[7];
    int16_t displacement = get_immediate();
    *a[7] += displacement;
    
}

void c_m68k::RTR_()
{
    //todo: wrong?
    uint16_t ccr = read_word(*a[7]);// & 0xA71F;
    *a[7] += 2;
    sr = (sr & 0xFF00) | (ccr & 0xFF);
    set_flags();
    pc = read_word(*a[7]) << 16;
    pc |= read_word(*a[7] + 2);
    *a[7] += 4;
}

void c_m68k::RTS_()
{
    pc = read_word(*a[7]) << 16;
    pc |= read_word(*a[7] + 2);
    *a[7] += 4;
}

void c_m68k::Scc_()
{
    set_size(SIZE_BYTE);
    get_address_mode();
    compute_ea();
    preincrement_ea();
    if (test_condition((CONDITION)((op_word >> 8) & 0xF))) {
        write_ea(0xFF);
        if (address_mode == ADDRESS_MODE::DATA_REGISTER || address_mode == ADDRESS_MODE::ADDRESS_REGISTER) {
            available_cycles -= 2;
        }
    }
    else {
        write_ea(0x00);
    }
    postincrement_ea();
    writeback_ea();
}

void c_m68k::RTE_()
{
    //unsure if this is correct.  what should a7 point to at the end?
    if (!(sr & 0x2000)) {
        int x = 1;
        //trap
    }
    else {
        sr = read_word(*a[7]);// & 0xA71F;
        set_flags();
        *a[7] += 2;
        pc = read_word(*a[7]) << 16;
        pc |= read_word(*a[7] + 2);
        *a[7] += 4;
        update_status();
    }
}

void c_m68k::RESET_()
{
    if (!(sr & 0x2000)) {
        //trap
    }
    else {
        //assert reset
    }
}

void c_m68k::do_trap(uint32_t vector)
{
    //unsure if sr being saved to stack is correct
    update_status();
    uint16_t orig_sr = sr;
    //set sr s-bit
    sr |= 0x2000;
    update_status();
    *a[7] -= 4;
    write_word(*a[7], pc >> 16);
    write_word(*a[7] + 2, pc & 0xFFFF);
    *a[7] -= 2;
    write_word(*a[7], orig_sr);
    pc = read_word(vector * 4) << 16;
    pc |= read_word(vector * 4 + 2);
    //ssp
}

void c_m68k::TRAPV_()
{
    if (flag_v) {
        do_trap(7);
    }
}

void c_m68k::TRAP_()
{
    do_trap(32 + (op_word & 0xF));
}

void c_m68k::CHK_()
{
    //this may not be correct
    //tests have different flag values, disagree with sr saved to stack
    set_size(SIZE_WORD);
    get_address_mode();
    compute_ea();
    preincrement_ea();
    uint32_t reg = (op_word >> 9) & 0x07;
    int16_t data = *d[reg];
    int16_t src = read_ea();
    postincrement_ea();
    writeback_ea();
    //undocumented
    flag_v = 0;
    flag_c = 0;
    flag_z = data == 0;
    if (data < 0 || data > src) {
        if (data < 0) {
            flag_n = 1;
        }
        else {
            flag_n = 0;
        }
        do_trap(6);
        available_cycles -= 34;
    }
    else {
        int x = 1;
    }
}

void c_m68k::BSR_()
{
    int32_t displacement = (int32_t)(int8_t)(op_word & 0xFF);
    uint32_t orig_pc = pc;
    if (displacement == 0) {
        displacement = (int32_t)(int16_t)read_word(pc);
        pc += 2;
    }

    *a[7] -= 4;
    write_word(*a[7], pc >> 16);
    write_word(*a[7] + 2, pc & 0xFFFF);

    pc = orig_pc + displacement;
}

bool c_m68k::test_condition(CONDITION condition)
{
    switch (condition) {
    case CONDITION::T:
        return true;
    case CONDITION::F:
        return false;
    case CONDITION::HI:
        return !flag_c && !flag_z;
    case CONDITION::LS:
        return !!flag_c || !!flag_z;
    case CONDITION::CC:
        return !flag_c;
    case CONDITION::CS:
        return !!flag_c;
    case CONDITION::NE:
        return !flag_z;
    case CONDITION::EQ:
        return !!flag_z;
    case CONDITION::VC:
        return !flag_v;
    case CONDITION::VS:
        return !!flag_v;
    case CONDITION::PL:
        return !flag_n;
    case CONDITION::MI:
        return !!flag_n;
    case CONDITION::GE:
        return !!flag_n && !!flag_v || !flag_n && !flag_v;
    case CONDITION::LT:
        return !!flag_n && !flag_v || !flag_n && !!flag_v;
    case CONDITION::GT:
        return !!flag_n && !!flag_v && !flag_z || !flag_n && !flag_v && !flag_z;
    case CONDITION::LE:
        return !!flag_z || !!flag_n && !flag_v || !flag_n && !!flag_v;
    }
    return false;
}

void c_m68k::BRA_()
{
    int32_t displacement = (int32_t)(int8_t)(op_word & 0xFF);
    uint32_t orig_pc = pc;
    if (displacement == 0) {
        displacement = (int32_t)(int16_t)read_word(pc);
        pc += 2;
    }

    pc = orig_pc + displacement;
}

void c_m68k::Bcc_()
{
    int32_t displacement = (int32_t)(int8_t)(op_word & 0xFF);
    uint32_t orig_pc = pc;
    if (displacement == 0) {
        displacement = (int32_t)(int16_t)read_word(pc);
        pc += 2;
        available_cycles -= 4;
    }

    if (test_condition((CONDITION)((op_word >> 8) & 0xF))) {
        pc = orig_pc + displacement;
        available_cycles -= 2;
    }
}

void c_m68k::TAS_()
{
    set_size(SIZE_BYTE);
    get_address_mode();
    compute_ea();
    preincrement_ea();
    uint8_t operand = read_ea();
    flag_n = operand & highest_bit;
    flag_z = operand == 0;
    flag_v = 0;
    flag_c = 0;
    write_ea(operand | highest_bit);
    postincrement_ea();
    writeback_ea();
}

void c_m68k::PEA_()
{
    get_address_mode();
    compute_ea();
    //pre/post increment not valid modes
    *a[7] -= 4;
    write_word(*a[7], ea >> 16);
    write_word(*a[7] + 2, ea & 0xFFFF);
}

void c_m68k::CMPA_()
{
    uint32_t opmode = (op_word >> 6) & 0x7;
    if (opmode == 0b011) {
        set_size(SIZE_WORD);
    }
    else {
        set_size(SIZE_LONG);
    }
    uint32_t reg = (op_word >> 9) & 0x7;
    get_address_mode();
    compute_ea();
    preincrement_ea();

    uint32_t src = read_ea() & mask;
    if (opmode == 0b011) {
        src = (int32_t)(int16_t)src;
    }
    postincrement_ea();
    uint32_t dst = *a[reg];// &mask;
    //verify correctness
    highest_bit = 0x80000000;
    uint32_t result = dst - src;
    flag_n = result & highest_bit;
    flag_z = result == 0;
    flag_c = src > dst;
    flag_v = (dst ^ src) & (dst ^ result) & highest_bit;
    writeback_ea();
}

void c_m68k::EORItoSR_()
{
    update_status();
    if (!(sr & 0x2000)) {
        //user mode
        assert(0);
    }
    uint16_t immediate = read_word(pc);// & 0xA71F;
    pc += 2;
    sr ^= immediate;
    set_flags();
    update_status();
}

void c_m68k::EORItoCCR_()
{
    update_status();
    uint16_t immediate = read_word(pc);// & 0xA71F;
    pc += 2;
    uint8_t ccr = sr & 0xFF;
    ccr ^= (immediate & 0xFF);
    sr = (sr & 0xFF00) | ccr;
    set_flags();
}

void c_m68k::ORItoSR_()
{
    update_status();
    if (!(sr & 0x2000)) {
        //user mode
        assert(0);
    }
    uint16_t immediate = read_word(pc);// & 0xA71F;
    pc += 2;
    sr |= immediate;
    set_flags();
    update_status();
}

void c_m68k::ORItoCCR_()
{
    update_status();
    uint16_t immediate = read_word(pc);// & 0xA71F;
    pc += 2;
    uint8_t ccr = sr & 0xFF;
    ccr |= (immediate & 0xFF);
    sr = (sr & 0xFF00) | ccr;
    set_flags();
    update_status();
}

void c_m68k::ANDItoSR_()
{
    update_status();
    if (!(sr & 0x2000)) {
        //user mode
        assert(0);
    }
    uint16_t immediate = read_word(pc);// & 0xA71F;
    pc += 2;
    sr &= immediate;
    set_flags();
    update_status();
}

void c_m68k::ANDItoCCR_()
{
    update_status();
    uint16_t immediate = read_word(pc);// & 0xA71F;
    pc += 2;
    uint8_t ccr = sr & 0xFF;
    ccr &= (immediate & 0xFF);
    sr = (sr & 0xFF00) | ccr;
    set_flags();
}

void c_m68k::CMPM_()
{
    get_size1();
    
    address_mode = ADDRESS_MODE::ADDRESS_POSTINCREMENT;
    ea_indirect = 1;
    Xn = op_word & 0x7;
    compute_ea();
    uint32_t src = read_ea();
    postincrement_ea();
    writeback_ea();
    Xn = (op_word >> 9) & 0x7;
    compute_ea();
    uint32_t dst = read_ea();
    postincrement_ea();
    writeback_ea();
    
    uint32_t result = (dst - src) & mask;
    flag_c = src > dst;
    flag_n = result & highest_bit;
    flag_z = result == 0;
    //flag_v = (src ^ result) & (dst ^ result) & highest_bit;
    flag_v = (dst ^ src) & (dst ^ result) & highest_bit;
}

void c_m68k::CMP_()
{
    uint32_t mode = 0;
    switch ((op_word >> 6) & 0x7) {
    case 0b100:
        mode = 1;
    case 0b000:
        set_size(SIZE_BYTE);
        break;
    case 0b101:
        mode = 1;
    case 0b001:
        set_size(SIZE_WORD);
        break;
    case 0b110:
        mode = 1;
    case 0b010:
        set_size(SIZE_LONG);
        break;
    }
    uint32_t reg = (op_word >> 9) & 0x7;
    get_address_mode();
    compute_ea();
    preincrement_ea();
    uint32_t a = *d[reg] & mask;
    uint32_t b = read_ea() & mask;
    uint32_t result = 0;
    if (mode) {
        assert(0);
        //not supported - remove?
        //<ea> - Dn -> <ea>
        result = (b - a) & mask;
        write_ea(result);
        flag_x = flag_c = a > b;
        flag_n = result & highest_bit;
        flag_z = !result;
        flag_v = (b ^ a) & (b ^ result) & highest_bit;
    }
    else {
        //Dn - <ea> -> Dn
        result = (a - b) & mask;
        //*d[reg] = (*d[reg] & ~mask) | result;
        //flag_x = flag_c = b > a;
        flag_c = b > a;
        flag_n = result & highest_bit;
        flag_z = result == 0;
        flag_v = (a ^ b) & (a ^ result) & highest_bit;
    }

    postincrement_ea();
    writeback_ea();
}

void c_m68k::CMPI_()
{
    get_size1();
    uint32_t b = get_immediate();
    get_address_mode();
    compute_ea();
    preincrement_ea();
    uint32_t a = read_ea() & mask;
    uint32_t result = a - b;
    //write_ea(result);
    flag_c = b > a;
    flag_n = result & highest_bit;
    flag_z = result == 0;
    flag_v = (a ^ b) & (a ^ result) & highest_bit;

    postincrement_ea();
    writeback_ea();
}

void c_m68k::NOP_()
{
}

void c_m68k::ROXd_()
{
    get_size1();
    get_address_mode();
    compute_ea();
    preincrement_ea();
    set_size(SIZE_WORD);

    int direction = op_word & 0x100; //0 = right, 1 = left

    uint32_t shift_count = 1;

    uint32_t result = read_ea();

    flag_v = 0;
    if (shift_count == 0) {
        flag_c = flag_x;
    }
    if (direction) {
        for (int i = 0; i < shift_count; i++) {
            //this can be optimized, but doing it this way for now to simplify flag stuff
            flag_c = result & highest_bit;
            uint32_t hi_bit_before = result & highest_bit;
            result <<= 1;
            result &= mask;
            if (flag_x)
            {
                result |= 1;
            }
            flag_x = hi_bit_before ? 1 : 0;
        }
    }
    else {
        for (int i = 0; i < shift_count; i++) {
            //this can be optimized, but doing it this way for now to simplify flag stuff
            flag_c = result & 0x1;
            result >>= 1;
            result &= mask;
            if (flag_x) {
                result |= highest_bit;
            }
            flag_x = flag_c ? 1 : 0;
        }
    }
    flag_n = result & highest_bit;
    flag_z = result == 0;
    write_ea(result);
    postincrement_ea();
    writeback_ea();
}

void c_m68k::ROXd2_()
{
    get_size1();

    int direction = op_word & 0x100; //0 = right, 1 = left
    int source = op_word & 0x20; //0 = immediate, 1 = register


    uint32_t shift_count = (op_word >> 9) & 0x7;
    int register_index = op_word & 0x7;

    if (op_word == 59939) {
        int x = 1;
    }

    if (source) {
        //register
        shift_count = *d[shift_count];
        shift_count &= 63;
    }
    else {
        if (shift_count == 0) {
            shift_count = 8;
        }
    }

    uint32_t result = *d[register_index] & mask;

    flag_v = 0;
    if (shift_count == 0) {
        flag_c = flag_x;
    }
    if (direction) {
        for (int i = 0; i < shift_count; i++) {
            //this can be optimized, but doing it this way for now to simplify flag stuff
            flag_c = result & highest_bit;
            uint32_t hi_bit_before = result & highest_bit;
            result <<= 1;
            result &= mask;
            if (flag_x)
            {
                result |= 1;
            }
            flag_x = hi_bit_before ? 1 : 0;
        }
    }
    else {
        for (int i = 0; i < shift_count; i++) {
            //this can be optimized, but doing it this way for now to simplify flag stuff
            flag_c = result & 0x1;
            result >>= 1;
            result &= mask;
            if (flag_x) {
                result |= highest_bit;
            }
            flag_x = flag_c ? 1 : 0;
            
        }
    }
    flag_n = result & highest_bit;
    flag_z = result == 0;
    flag_v = 0;
    *d[register_index] = (*d[register_index] & ~mask) | (result & mask);
}

void c_m68k::ROd_()
{
    get_size1();
    get_address_mode();
    compute_ea();
    preincrement_ea();
    set_size(SIZE_WORD);

    int direction = op_word & 0x100; //0 = right, 1 = left

    uint32_t shift_count = 1;

    uint32_t result = read_ea();

    flag_v = 0;
    if (shift_count == 0) {
        flag_c = 0;
    }
    if (direction) {
        for (int i = 0; i < shift_count; i++) {
            //this can be optimized, but doing it this way for now to simplify flag stuff
            flag_c = result & highest_bit;
            uint32_t hi_bit_before = result & highest_bit;
            result <<= 1;
            result &= mask;
            if (hi_bit_before) {
                result |= 1;
            }
        }
    }
    else {
        for (int i = 0; i < shift_count; i++) {
            //this can be optimized, but doing it this way for now to simplify flag stuff
            flag_c = result & 0x1;
            result >>= 1;
            result &= mask;
            if (flag_c) {
                result |= highest_bit;
            }
        }
    }
    flag_n = result & highest_bit;
    flag_z = result == 0;
    write_ea(result);
    postincrement_ea();
    writeback_ea();
}

void c_m68k::ROd2_()
{
    get_size1();

    int direction = op_word & 0x100; //0 = right, 1 = left
    int source = op_word & 0x20; //0 = immediate, 1 = register


    uint32_t shift_count = (op_word >> 9) & 0x7;
    int register_index = op_word & 0x7;

    if (op_word == 59939) {
        int x = 1;
    }

    if (source) {
        //register
        shift_count = *d[shift_count];
        shift_count &= 63;
    }
    else {
        if (shift_count == 0) {
            shift_count = 8;
        }
    }

    uint32_t result = *d[register_index] & mask;

    flag_v = 0;
    if (shift_count == 0) {
        flag_c = 0;
    }
    if (direction) {
        for (int i = 0; i < shift_count; i++) {
            //this can be optimized, but doing it this way for now to simplify flag stuff
            flag_c = result & highest_bit;
            uint32_t hi_bit_before = result & highest_bit;
            result <<= 1;
            result &= mask;
            if (hi_bit_before) {
                result |= 1;
            }
        }
    }
    else {
        for (int i = 0; i < shift_count; i++) {
            //this can be optimized, but doing it this way for now to simplify flag stuff
            flag_c = result & 0x1;
            result >>= 1;
            result &= mask;
            if (flag_c) {
                result |= highest_bit;
            }
        }
    }
    flag_n = result & highest_bit;
    flag_z = result == 0;
    flag_v = 0;
    *d[register_index] = (*d[register_index] & ~mask) | (result & mask);
}

void c_m68k::MOVEA_()
{
    get_size3();
    get_address_mode();
    compute_ea();
    preincrement_ea();
    uint32_t src = read_ea() & mask;
    postincrement_ea();
    writeback_ea();
    uint32_t reg = (op_word >> 9) & 0x7;
    if (op_size == SIZE_WORD) {
        *a[reg] = (int32_t)(int16_t)src;
    }
    else {
        *a[reg] = src;
    }
}

void c_m68k::MOVEtoSR_()
{
    if (!(sr & 0x2000)) {
        //user mode
        assert(0);
    }
    set_size(SIZE_WORD);
    get_address_mode();
    compute_ea();
    preincrement_ea();
    sr = read_ea();// & 0xA71F;
    set_flags();
    update_status();
    postincrement_ea();
    writeback_ea();
}

void c_m68k::MOVEfromSR_()
{
    set_size(SIZE_WORD);
    get_address_mode();
    compute_ea();
    preincrement_ea();
    update_status();
    write_ea(sr);
    postincrement_ea();
    writeback_ea();
}

void c_m68k::MOVEtoCCR_()
{
    set_size(SIZE_WORD);
    get_address_mode();
    compute_ea();
    preincrement_ea();
    sr = (sr & 0xFF00) | (read_ea() & 0xFF);
    set_flags();
    update_status();
    postincrement_ea();
    writeback_ea();
}

void c_m68k::MOVE_USP_()
{
    uint32_t reg = op_word & 0x7;
    if (op_word & 0x8) {
        //usp -> a
        *a[reg] = usp;
    }
    else {
        //a -> usp
        usp = *a[reg];
    }
}

void c_m68k::MOVEQ_()
{
    set_size(SIZE_LONG);
    uint32_t data = (int32_t)(int8_t)(op_word & 0xFF);
    uint32_t reg = (op_word >> 9) & 0x7;
    *d[reg] = data;
    flag_n = data & highest_bit;
    flag_z = data == 0;
    flag_v = 0;
    flag_c = 0;
}

void c_m68k::MOVE_()
{
    get_size3();
    get_address_mode();
    compute_ea();
    preincrement_ea();
    uint32_t src = read_ea() & mask;
    postincrement_ea();
    writeback_ea();
    //overwrite opword and run ea stuff again for dest
    uint16_t op_word2 = (op_word >> 9) & 0x7;
    op_word2 |= (op_word >> 3) & 0x38;
    op_word = op_word2;
    get_address_mode();
    compute_ea();
    preincrement_ea();
    write_ea(src);
    postincrement_ea();
    writeback_ea();

    flag_n = src & highest_bit;
    flag_z = src == 0;
    flag_v = 0;
    flag_c = 0;

}

void c_m68k::ADDA_()
{
    uint32_t opmode = (op_word >> 6) & 0x7;
    if (opmode == 0b011) {
        set_size(SIZE_WORD);
    }
    else {
        set_size(SIZE_LONG);
    }
    uint32_t reg = (op_word >> 9) & 0x7;
    get_address_mode();
    compute_ea();
    preincrement_ea();

    uint32_t src = read_ea() & mask;
    if (opmode == 0b011) {
        src = (int32_t)(int16_t)src;
    }
    postincrement_ea();
    writeback_ea();
    uint32_t dst = *a[reg];// &mask;

    uint32_t result = dst + src;

    *a[reg] = result;

}

void c_m68k::SUBA_()
{
    uint32_t opmode = (op_word >> 6) & 0x7;
    if (opmode == 0b011) {
        set_size(SIZE_WORD);
    }
    else {
        set_size(SIZE_LONG);
    }
    uint32_t reg = (op_word >> 9) & 0x7;
    get_address_mode();
    compute_ea();
    preincrement_ea();

    uint32_t src = read_ea() & mask;
    if (opmode == 0b011) {
        src = (int32_t)(int16_t)src;
    }
    postincrement_ea();
    writeback_ea();
    uint32_t dst = *a[reg];// &mask;

    uint32_t result = dst - src;
    
    *a[reg] = result;

}

void c_m68k::ADDI_()
{
    get_size1();
    uint32_t b = get_immediate();
    get_address_mode();
    compute_ea();
    preincrement_ea();
    uint32_t a = read_ea() & mask;
    uint64_t result = (uint64_t)a + (uint64_t)b;
    write_ea(result);
    flag_x = flag_c = result > mask;
    result &= mask;
    flag_n = result & highest_bit;
    flag_z = !result;
    flag_v = (a ^ result) & (b ^ result) & highest_bit;

    postincrement_ea();
    writeback_ea();
}

void c_m68k::SUBI_()
{
    get_size1();
    uint32_t b = get_immediate();
    get_address_mode();
    compute_ea();
    preincrement_ea();
    uint32_t a = read_ea() & mask;
    uint32_t result = a - b;
    write_ea(result);
    flag_x = flag_c = b > a;
    flag_n = result & highest_bit;
    flag_z = !result;
    flag_v = (a ^ b) & (a ^ result) & highest_bit;

    postincrement_ea();
    writeback_ea();
}

uint32_t c_m68k::get_immediate()
{
    uint32_t b;
    switch (op_size) {
    case SIZE_BYTE:
        b = read_word(pc) & 0xFF;
        pc += 2;
        break;
    case SIZE_WORD:
        b = read_word(pc);
        pc += 2;
        break;
    case SIZE_LONG:
        b = read_word(pc) << 16;
        pc += 2;
        b |= read_word(pc);
        pc += 2;
        break;
    }
    return b;
}

void c_m68k::ADDQ_()
{
    get_size1();
    get_address_mode();
    compute_ea();
    preincrement_ea();

    if (address_mode == ADDRESS_MODE::ADDRESS_REGISTER) {
        set_size(SIZE_LONG);
    }
    uint32_t a = read_ea() & mask;
    uint32_t b = (op_word >> 9) & 0x7;
    if (b == 0) {
        b = 8;
    }
    uint64_t result = (uint64_t)a + (uint64_t)b;

    if (address_mode != ADDRESS_MODE::ADDRESS_REGISTER) {
        flag_x = flag_c = result > mask;
        result &= mask;
        flag_n = result & highest_bit;
        flag_z = !result;
        flag_v = (a ^ result) & (b ^ result) & highest_bit;
    }
    write_ea(result);


    postincrement_ea();
    writeback_ea();
}

void c_m68k::SUBQ_()
{
    get_size1();
    get_address_mode();
    compute_ea();
    preincrement_ea();

    if (address_mode == ADDRESS_MODE::ADDRESS_REGISTER) {
        set_size(SIZE_LONG);
    }
    uint32_t a = read_ea() & mask;
    uint32_t b = (op_word >> 9) & 0x7;
    if (b == 0) {
        b = 8;
    }
    uint32_t result = a - b;

    if (address_mode != ADDRESS_MODE::ADDRESS_REGISTER) {
        flag_x = flag_c = b > a;
        flag_n = result & highest_bit;
        flag_z = !result;
        flag_v = (a ^ b) & (a ^ result) & highest_bit;
    }
    write_ea(result);


    postincrement_ea();
    writeback_ea();
}

void c_m68k::ADD_()
{
    uint32_t mode = 0;
    switch ((op_word >> 6) & 0x7) {
    case 0b100:
        mode = 1;
    case 0b000:
        set_size(SIZE_BYTE);
        break;
    case 0b101:
        mode = 1;
    case 0b001:
        set_size(SIZE_WORD);
        break;
    case 0b110:
        mode = 1;
    case 0b010:
        set_size(SIZE_LONG);
        break;
    }
    uint32_t reg = (op_word >> 9) & 0x7;
    get_address_mode();
    compute_ea();
    preincrement_ea();
    uint32_t a = *d[reg] & mask;
    uint32_t b = read_ea() & mask;
    uint64_t result = 0;
    if (mode) {
        //<ea> - Dn -> <ea>
        result = (uint64_t)a + (uint64_t)b;
        flag_x = flag_c = result > mask;
        result &= mask;
        write_ea(result);
        flag_n = result & highest_bit;
        flag_z = !result;
        //flag_v = (b ^ a) & (b ^ result) & highest_bit;
        flag_v = (a ^ result) & (b ^ result) & highest_bit;
    }
    else {
        //Dn - <ea> -> Dn
        //result = (a - b) & mask;
        result = (uint64_t)a + (uint64_t)b;
        flag_x = flag_c = result > mask;
        result &= mask;
        *d[reg] = (*d[reg] & ~mask) | result;
        flag_n = result & highest_bit;
        flag_z = !result;
        flag_v = (a ^ result) & (b ^ result) & highest_bit;
    }

    postincrement_ea();
    writeback_ea();
}

void c_m68k::SUB_()
{
    uint32_t mode = 0;
    switch ((op_word >> 6) & 0x7) {
    case 0b100:
        mode = 1;
    case 0b000:
        set_size(SIZE_BYTE);
        break;
    case 0b101:
        mode = 1;
    case 0b001:
        set_size(SIZE_WORD);
        break;
    case 0b110:
        mode = 1;
    case 0b010:
        set_size(SIZE_LONG);
        break;
    }
    uint32_t reg = (op_word >> 9) & 0x7;
    get_address_mode();
    compute_ea();
    preincrement_ea();
    uint32_t a = *d[reg] & mask;
    uint32_t b = read_ea() & mask;
    uint32_t result = 0;
    if (mode) {
        //<ea> - Dn -> <ea>
        result = (b - a) & mask;
        write_ea(result);
        flag_x = flag_c = a > b;
        flag_n = result & highest_bit;
        flag_z = !result;
        flag_v = (b ^ a) & (b ^ result) & highest_bit;
    }
    else {
        //Dn - <ea> -> Dn
        result = (a - b) & mask;
        *d[reg] = (*d[reg] & ~mask) | result;
        flag_x = flag_c = b > a;
        flag_n = result & highest_bit;
        flag_z = !result;
        flag_v = (a ^ b) & (a ^ result) & highest_bit;
    }

    postincrement_ea();
    writeback_ea();
}

void c_m68k::ADDX_()
{
    // y = y - x - X
    get_size1();
    uint32_t X = flag_x ? 1 : 0;
    uint32_t reg_x = op_word & 0x7;
    uint32_t reg_y = (op_word >> 9) & 0x7;
    uint32_t mode = (op_word >> 3) & 0x1;
    uint64_t result;
    if (mode) {
        //address
        address_mode = ADDRESS_MODE::ADDRESS_PREINCREMENT;
        ea_indirect = 1;
        Xn = reg_x;
        compute_ea();
        preincrement_ea();
        uint32_t b = read_ea() & mask;
        writeback_ea();
        Xn = reg_y;
        compute_ea();
        preincrement_ea();
        uint32_t a = read_ea() & mask;
        result = (int64_t)a + (int64_t)b + (int64_t)X;
        flag_x = flag_c = result > mask;
        result &= mask;
        write_ea(result);
        writeback_ea();
        flag_n = result & highest_bit;
        if (result) {
            flag_z = 0;
        }
        flag_v = (a ^ result) & (b ^ result) & highest_bit;
    }
    else {
        //data
        uint32_t a = *d[reg_y] & mask;
        uint32_t b = *d[reg_x] & mask;
        result = (int64_t)a + (int64_t)b + (int64_t)X;
        flag_x = flag_c = result > mask;
        result &= mask;
        *d[reg_y] = (*d[reg_y] & ~mask) | result;
        flag_n = result & highest_bit;
        if (result) {
            flag_z = 0;
        }
        flag_v = (a ^ result) & (b ^ result) & highest_bit;
    }
}

void c_m68k::SUBX_()
{
    // y = y - x - X
    get_size1();
    uint32_t X = flag_x ? 1 : 0;
    uint32_t reg_x = op_word & 0x7;
    uint32_t reg_y = (op_word >> 9) & 0x7;
    uint32_t mode = (op_word >> 3) & 0x1;
    uint32_t result;
    if (mode) {
        //address
        address_mode = ADDRESS_MODE::ADDRESS_PREINCREMENT;
        ea_indirect = 1;
        Xn = reg_x;
        compute_ea();
        preincrement_ea();
        uint32_t b = read_ea() & mask;
        writeback_ea();
        Xn = reg_y;
        compute_ea();
        preincrement_ea();
        uint32_t a = read_ea() & mask;
        result = (a - b - X) & mask;
        write_ea(result);
        writeback_ea();
        flag_x = flag_c = (int64_t)(b + X) > (int64_t)a;
        flag_n = result & highest_bit;
        if (result) {
            flag_z = 0;
        }
        flag_v = (a ^ b) & (a ^ result) & highest_bit;
    }
    else {
        //data
        uint32_t a = *d[reg_y] & mask;
        uint32_t b = *d[reg_x] & mask;
        result = (a - b - X) & mask;
        *d[reg_y] = (*d[reg_y] & ~mask) | result;
        flag_x = flag_c = (int64_t)(b + X) > (int64_t)a;
        flag_n = result & highest_bit;
        if (result) {
            flag_z = 0;
        }
        flag_v = (a ^ b) & (a ^ result) & highest_bit;
    }
}

void c_m68k::SWAP_()
{
    uint32_t reg = op_word & 0x7;
    uint16_t a = *d[reg] >> 16;
    uint16_t b = *d[reg] & 0xFFFF;
    uint32_t result = (b << 16) | a;
    *d[reg] = result;
    flag_n = result & 0x8000'0000;
    flag_z = result == 0;
    flag_v = 0;
    flag_c = 0;
}

void c_m68k::JSR_()
{
    //char buf[64];
    //sprintf(buf, "PC: %08x\n", pc-2);
    //OutputDebugString(buf);
    set_size(SIZE_LONG);
    get_address_mode();
    compute_ea();
    preincrement_ea();
    *a[7] -= 4;
    write_word(*a[7], pc >> 16);
    write_word(*a[7] + 2, pc & 0xFFFF);
    pc = ea;
    postincrement_ea();
    writeback_ea();
}

void c_m68k::JMP_()
{
    set_size(SIZE_LONG);
    get_address_mode();
    compute_ea();
    preincrement_ea();
    pc = ea;
    postincrement_ea();
    writeback_ea();
}

void c_m68k::LSd_()
{
    //ASR results might not be correct because tests are broken
    get_size1();
    get_address_mode();
    compute_ea();
    preincrement_ea();
    set_size(SIZE_WORD);

    int direction = op_word & 0x100; //0 = right, 1 = left

    uint32_t shift_count = 1;

    uint32_t result = read_ea();

    flag_v = 0;
    if (shift_count == 0) {
        flag_c = 0;
    }
    if (direction) {
        for (int i = 0; i < shift_count; i++) {
            //this can be optimized, but doing it this way for now to simplify flag stuff
            flag_x = flag_c = result & highest_bit;
            result <<= 1;
            result &= mask;
            int x = 1;
        }
    }
    else {
        for (int i = 0; i < shift_count; i++) {
            //this can be optimized, but doing it this way for now to simplify flag stuff
            flag_x = flag_c = result & 0x1;
            //uint32_t high_bit = result & highest_bit;
            result >>= 1;
            result &= mask;
            //result |= high_bit;
        }
    }
    flag_n = result & highest_bit;
    flag_z = result == 0;
    write_ea(result);
    postincrement_ea();
    writeback_ea();
}

void c_m68k::LSd2_()
{
    //ASR results might not be correct because tests are broken

    get_size1();

    int direction = op_word & 0x100; //0 = right, 1 = left
    int source = op_word & 0x20; //0 = immediate, 1 = register


    uint32_t shift_count = (op_word >> 9) & 0x7;
    int register_index = op_word & 0x7;

    if (op_word == 59939) {
        int x = 1;
    }

    if (source) {
        //register
        shift_count = *d[shift_count];
        shift_count &= 63;
    }
    else {
        if (shift_count == 0) {
            shift_count = 8;
        }
    }

    uint32_t result = *d[register_index] & mask;

    flag_v = 0;
    if (shift_count == 0) {
        flag_c = 0;
    }
    if (direction) {
        for (int i = 0; i < shift_count; i++) {
            //this can be optimized, but doing it this way for now to simplify flag stuff
            flag_x = flag_c = result & highest_bit;
            result <<= 1;
            result &= mask;
        }
    }
    else {
        for (int i = 0; i < shift_count; i++) {
            //this can be optimized, but doing it this way for now to simplify flag stuff
            flag_x = flag_c = result & 0x1;
            //uint32_t high_bit = result & highest_bit;
            result >>= 1;
            result &= mask;
            //result |= high_bit;
        }
    }
    flag_n = result & highest_bit;
    flag_z = result == 0;
    *d[register_index] = (*d[register_index] & ~mask) | (result & mask);
}

void c_m68k::TST_()
{
    get_size1();
    get_address_mode();
    compute_ea();
    preincrement_ea();

    uint32_t a = read_ea();

    flag_z = (a & mask) == 0;
    flag_n = a & highest_bit;
    flag_v = 0;
    flag_c = 0;

    postincrement_ea();
    writeback_ea();
}

void c_m68k::EXG_()
{
    uint32_t Ry = op_word & 0x7;
    uint32_t Rx = (op_word >> 9) & 0x7;
    switch ((op_word >> 3) & 31) {
    case 0b01000:
        std::swap(*d[Rx], *d[Ry]);
        break;
    case 0b01001:
        std::swap(*a[Rx], *a[Ry]);
        break;
    case 0b10001:
        std::swap(*d[Rx], *a[Ry]);
        break;
    default:
        assert(0);
        break;
    }
}

void c_m68k::CLR_()
{
    get_size1();
    get_address_mode();
    compute_ea();
    preincrement_ea();
    write_ea(0);
    postincrement_ea();
    writeback_ea();
    flag_n = 0;
    flag_z = 1;
    flag_v = 0;
    flag_c = 0;
}

void c_m68k::set_size(int size)
{
    switch (size) {
    case 0:
        mask = 0xFF;
        highest_bit = 0x80;
        op_size = SIZE_BYTE;
        break;
    case 1:
        mask = 0xFFFF;
        highest_bit = 0x8000;
        op_size = SIZE_WORD;
        break;
    case 2:
        mask = 0xFFFFFFFF;
        highest_bit = 0x80000000;
        op_size = SIZE_LONG;
        break;
    }
}

void c_m68k::get_size1()
{
    int size = (op_word >> 6) & 0x3;
    set_size(size);
}

void c_m68k::get_size3()
{
    int size = (op_word >> 12) & 0x3;
    switch (size) {
    case 1:
        size = 0;
        break;
    case 2:
        size = 2;
        break;
    case 3:
        size = 1;
        break;
    }
    set_size(size);
}

int c_m68k::get_ea_cycles()
{
    assert(address_mode != ADDRESS_MODE::UNKNOWN);
    assert(op_size != SIZE_UNKNOWN);
    switch (address_mode) {
        case ADDRESS_MODE::DATA_REGISTER:
            return 0;
        case ADDRESS_MODE::ADDRESS_REGISTER:
            return 0;
        case ADDRESS_MODE::ADDRESS:
            return op_size == SIZE_LONG ? 8 : 4;
        case ADDRESS_MODE::ADDRESS_PREINCREMENT:
            return op_size == SIZE_LONG ? 10 : 6;
        case ADDRESS_MODE::ADDRESS_POSTINCREMENT:
            return op_size == SIZE_LONG ? 8 : 4;
        case ADDRESS_MODE::ADDRESS_DISPLACEMENT:
            return op_size == SIZE_LONG ? 12 : 8;
        case ADDRESS_MODE::ADDRESS_INDEX:
            return op_size == SIZE_LONG ? 14 : 10;
        case ADDRESS_MODE::ABS_SHORT:
            return op_size == SIZE_LONG ? 12 : 8;
        case ADDRESS_MODE::ABS_LONG:
            return op_size == SIZE_LONG ? 16 : 12;
        case ADDRESS_MODE::PC_DISPLACEMENT:
            return op_size == SIZE_LONG ? 12 : 8;
        case ADDRESS_MODE::PC_INDEX:
            return op_size == SIZE_LONG ? 14 : 10;
        case ADDRESS_MODE::IMMEDIATE:
            return op_size == SIZE_LONG ? 8 : 4;
    }
    return 0;
}

void c_m68k::get_address_mode()
{
    M = (op_word >> 3) & 0x7;
    Xn = op_word & 0x7;
    if (M < 7) {
        address_mode = (ADDRESS_MODE)M;
        ea_indirect = (int)address_mode < 2 ? 0 : 1;
    }
    else {
        switch (Xn) {
        case 0:
            address_mode = ADDRESS_MODE::ABS_SHORT;
            break;
        case 1:
            address_mode = ADDRESS_MODE::ABS_LONG;
            break;
        case 2:
            address_mode = ADDRESS_MODE::PC_DISPLACEMENT;
            break;
        case 3:
            address_mode = ADDRESS_MODE::PC_INDEX;
            break;
        case 4:
            address_mode = ADDRESS_MODE::IMMEDIATE;
            immediate_size = -1;
            break;
        default:
            assert(0);
            break;
        }
        ea_indirect = address_mode == ADDRESS_MODE::IMMEDIATE ? 0 : 1;
    }
}

uint32_t c_m68k::read_ea()
{
    uint32_t v;
    //if (address_increment < 0) {
    //    ea += address_increment;
    //    *address_increment_target = ea;
    //}
    if (!ea_indirect) {
        return ea;
    }
    else {
        switch (op_size) {
        case SIZE_BYTE:
            v = read_byte(ea);
            break;
        case SIZE_WORD:
            v = read_word(ea);
            break;
        case SIZE_LONG:
            v = read_word(ea) << 16;
            v |= read_word(ea + 2);
            break;
        }
    }
    //if (address_increment > 0) {
    //    ea += address_increment;
    //    *address_increment_target = ea;
    //}
    return v;
}

void c_m68k::write_ea(uint32_t value)
{
    if (!ea_indirect) {
        if (register_target) {
            //*register_target = value;
            *register_target = (*register_target & ~mask) | (value & mask);
        }
    }
    else {
        switch (op_size) {
        case SIZE_BYTE:
            write_byte(ea, value);
            break;
        case SIZE_WORD:
            write_word(ea, value);
            break;
        case SIZE_LONG:
            write_word(ea, value >> 16);
            write_word(ea + 2, value & 0xFFFF);
            break;
        }
    }
}

uint32_t c_m68k::compute_ea()
{
    //ea_indirect = 0;
    address_increment = 0;
    register_target = nullptr;
    uint32_t v;
    int16_t displacement;
    uint32_t index;
    //uint32_t i_size = immediate_size != -1 ? immediate_size : op_size;
    switch (address_mode) {
    case ADDRESS_MODE::IMMEDIATE:
            switch (immediate_size != -1 ? immediate_size : op_size) {
        case SIZE_BYTE:
            v = read_word(pc) & 0xFF;
            pc += 2;
            break;
        case SIZE_WORD:
            v = read_word(pc);
            pc += 2;
            break;
        case SIZE_LONG:
            v = read_word(pc) << 16;
            pc += 2;
            v |= read_word(pc);
            pc += 2;
            break;
        }
        break;
    case ADDRESS_MODE::DATA_REGISTER:
        register_target = d[Xn];
        v = *d[Xn];
        break;
    case ADDRESS_MODE::ADDRESS_REGISTER:
        register_target = a[Xn];
        v = *a[Xn];
        break;
    case ADDRESS_MODE::ADDRESS:
        v = *a[Xn];
        //ea_indirect = 1;
        break;
    case ADDRESS_MODE::ADDRESS_DISPLACEMENT:
        displacement = (int16_t)read_word(pc);
        pc += 2;
        v = *a[Xn] + displacement;
        //ea_indirect = 1;
        break;
    case ADDRESS_MODE::ABS_SHORT:
        v = (int32_t)(int16_t)read_word(pc);
        pc += 2;
        //ea_indirect = 1;
        break;
    case ADDRESS_MODE::ABS_LONG:
        v = read_word(pc) << 16;
        pc += 2;
        v |= read_word(pc);
        pc += 2;
        //ea_indirect = 1;
        break;
    case ADDRESS_MODE::ADDRESS_INDEX:
        extension_word.value = read_word(pc);
        pc += 2;
        index = extension_word.d_a ? *a[extension_word.reg] : *d[extension_word.reg];
        if (!extension_word.size) {
            index = (int32_t)((int16_t)(index & 0xFFFF));
        }
        v = *a[Xn] + extension_word.d8 + index;
        //ea_indirect = 1;
        break;
    case ADDRESS_MODE::PC_DISPLACEMENT:
        v = (int32_t)(int16_t)read_word(pc);
        v += pc;
        pc += 2;
        //ea_indirect = 1;
        break;
    case ADDRESS_MODE::PC_INDEX:
        extension_word.value = read_word(pc);
        index = extension_word.d_a ? *a[extension_word.reg] : *d[extension_word.reg];
        if (!extension_word.size) {
            index = (int32_t)((int16_t)(index & 0xFFFF));
        }
        v = pc + extension_word.d8 + index;
        pc += 2;
        //ea_indirect = 1;
        break;
    case ADDRESS_MODE::ADDRESS_POSTINCREMENT:
        v = *a[Xn];
        address_increment_target = a[Xn];
        switch (op_size) {
        case SIZE_BYTE:
            //should this be 2?
            address_increment = Xn == 7 ? 2 : 1;
            break;
        case SIZE_WORD:
            address_increment = 2;
            break;
        case SIZE_LONG:
            address_increment = 4;
            break;
        }
        //ea_indirect = 1;
        break;
    case ADDRESS_MODE::ADDRESS_PREINCREMENT:
        v = *a[Xn];
        address_increment_target = a[Xn];
        switch (op_size) {
        case SIZE_BYTE:
            //should this be 2?
            address_increment = Xn == 7 ? -2 : -1;
            break;
        case SIZE_WORD:
            address_increment = -2;
            break;
        case SIZE_LONG:
            address_increment = -4;
            break;
        }
        //ea_indirect = 1;
        break;
    default:
        assert(0);
        break;
    }
    ea = v;
    return v;
}

void c_m68k::preincrement_ea()
{
    if (address_increment < 0) {
        uint32_t is_odd = ea & 0x1;
        ea += address_increment;
        //*address_increment_target = ea;
    }
}

void c_m68k::postincrement_ea()
{
    if (address_increment > 0) {
        uint32_t is_odd = ea & 0x1;
        //ea += address_increment;
        ea += address_increment;
        //*address_increment_target = ea;
    }
}

void c_m68k::writeback_ea()
{
    if (address_increment != 0) {
        *address_increment_target = ea;
    }
}

void c_m68k::BTST_()
{
    //bit number static, specified as immediate data
    uint32_t bit_number = read_word(pc);
    pc += 2;
    get_address_mode();
    if (ea_indirect) {
        set_size(SIZE_BYTE);
    }
    else {
        set_size(SIZE_LONG);
    }
    compute_ea();
    preincrement_ea();
    uint32_t a = read_ea();

    uint32_t modulo = ea_indirect ? 7 : 31;
    bit_number &= modulo;
    uint32_t bit_mask = 1 << bit_number;

    if (!(a & bit_mask)) {
        flag_z = 1;
    }
    else {
        flag_z = 0;
    }
    postincrement_ea();
    writeback_ea();
}

void c_m68k::BTST2_()
{
    get_address_mode();
    if (ea_indirect) {
        set_size(SIZE_BYTE);
    }
    else if (address_mode == ADDRESS_MODE::IMMEDIATE) {
        set_size(SIZE_WORD);
    }
    else {
        set_size(SIZE_LONG);
    }
    compute_ea();
    preincrement_ea();

    uint32_t a = read_ea();

    uint32_t modulo = (ea_indirect || address_mode == ADDRESS_MODE::IMMEDIATE ) ? 7 : 31;

    uint32_t bit_number = *d[(op_word >> 9) & 0x7] & modulo;
    uint32_t bit_mask = 1 << bit_number;

    if (!(a & bit_mask)) {
        flag_z = 1;
    }
    else {
        flag_z = 0;
    }
    postincrement_ea();
    writeback_ea();
}

void c_m68k::BITOP_(std::function<uint32_t(uint32_t, uint32_t)> op)
{
    //bit number static, specified as immediate data
    uint32_t bit_number = read_word(pc);
    pc += 2;
    get_address_mode();
    if (ea_indirect) {
        set_size(SIZE_BYTE);
    }
    else {
        set_size(SIZE_LONG);
    }
    compute_ea();
    preincrement_ea();
    uint32_t a = read_ea();

    uint32_t modulo = ea_indirect ? 7 : 31;
    bit_number &= modulo;
    uint32_t bit_mask = 1 << bit_number;

    if (!(a & bit_mask)) {
        flag_z = 1;
    }
    else {
        flag_z = 0;
    }
    //a |= bit_mask;
    a = op(a, bit_mask);
    write_ea(a);

    postincrement_ea();
    writeback_ea();
}

void c_m68k::BITOP2_(std::function<uint32_t(uint32_t, uint32_t)> op)
{
    get_address_mode();
    if (ea_indirect) {
        set_size(SIZE_BYTE);
    }
    else {
        set_size(SIZE_LONG);
    }
    compute_ea();
    preincrement_ea();

    uint32_t a = read_ea();

    uint32_t modulo = ea_indirect ? 7 : 31;

    uint32_t bit_number = *d[(op_word >> 9) & 0x7] & modulo;
    uint32_t bit_mask = 1 << bit_number;

    if (!(a & bit_mask)) {
        flag_z = 1;
    }
    else {
        flag_z = 0;
    }
    a = op(a, bit_mask);
    write_ea(a);

    postincrement_ea();
    writeback_ea();
}

void c_m68k::LEA_()
{
    get_address_mode();
    *a[(op_word >> 9) & 0x7] = compute_ea();
}

void c_m68k::NEGX_()
{
    uint32_t X = flag_x ? 1 : 0;
    get_size1();
    get_address_mode();
    compute_ea();
    preincrement_ea();
    uint32_t register_index = (op_word >> 9) & 0x7;
    uint32_t a = read_ea();
    uint32_t result = (0 - a - X) & mask;
    write_ea(result);
    flag_n = result & highest_bit;
    if (result) {
        flag_z = 0;
    }
    flag_x = flag_c = (a & mask) || X;
    //double check overflow calculation
    flag_v = (0 ^ a) & (0 ^ result) & highest_bit;
    postincrement_ea();
    writeback_ea();
}

void c_m68k::NEG_()
{
    get_size1();
    get_address_mode();
    compute_ea();
    preincrement_ea();
    uint32_t register_index = (op_word >> 9) & 0x7;
    uint32_t a = read_ea();
    uint32_t result = (0-a) & mask;
    write_ea(result);
    flag_n = result & highest_bit;
    flag_z = result == 0 ? 1 : 0;
    flag_v = result == highest_bit;
    flag_x = flag_c = result == 0 ? 0 : 1;
    postincrement_ea();
    writeback_ea();
}

void c_m68k::NOT_()
{
    get_size1();
    get_address_mode();
    compute_ea();
    preincrement_ea();
    uint32_t register_index = (op_word >> 9) & 0x7;
    uint32_t a = read_ea();
    uint32_t result = (~a) & mask;
    write_ea(result);
    flag_n = result & highest_bit;
    flag_z = result == 0 ? 1 : 0;
    flag_v = 0;
    flag_c = 0;
    postincrement_ea();
    writeback_ea();
}

void c_m68k::EOR_()
{
    get_size1();
    get_address_mode();
    compute_ea();
    preincrement_ea();
    uint32_t register_index = (op_word >> 9) & 0x7;
    uint32_t a = read_ea();
    uint32_t b = *d[register_index];
    uint32_t result = (a ^ b) & mask;
    if (op_word & 0x100) {
        //store to ea
        write_ea(result);
    }
    else {
        //store to register
        *d[register_index] = (*d[register_index] & ~mask) | (result & mask);
    }
    flag_n = result & highest_bit;
    flag_z = result == 0 ? 1 : 0;
    flag_v = 0;
    flag_c = 0;
    postincrement_ea();
    writeback_ea();
}

void c_m68k::EORI_()
{
    get_size1();
    uint32_t b;
    switch (op_size) {
    case SIZE_BYTE:
        b = read_word(pc) & 0xFF;
        pc += 2;
        break;
    case SIZE_WORD:
        b = read_word(pc);
        pc += 2;
        break;
    case SIZE_LONG:
        b = read_word(pc) << 16;
        pc += 2;
        b |= read_word(pc);
        pc += 2;
        break;
    }
    //get_address_mode();
    compute_ea();
    preincrement_ea();
    uint32_t a = read_ea();
    uint32_t result = (a ^ b) & mask;
    write_ea(result);
    flag_n = result & highest_bit;
    flag_z = result == 0 ? 1 : 0;
    flag_v = 0;
    flag_c = 0;
    postincrement_ea();
    writeback_ea();
}

void c_m68k::OR_()
{
    get_size1();
    get_address_mode();
    compute_ea();
    preincrement_ea();
    uint32_t register_index = (op_word >> 9) & 0x7;
    uint32_t a = read_ea();
    uint32_t b = *d[register_index];
    uint32_t result = (a | b) & mask;
    if (op_word & 0x100) {
        //store to ea
        write_ea(result);
    }
    else {
        //store to register
        *d[register_index] = (*d[register_index] & ~mask) | (result & mask);
    }
    flag_n = result & highest_bit;
    flag_z = result == 0 ? 1 : 0;
    flag_v = 0;
    flag_c = 0;
    postincrement_ea();
    writeback_ea();
}

void c_m68k::ORI_()
{
    get_size1();
    uint32_t b;
    switch (op_size) {
    case SIZE_BYTE:
        b = read_word(pc) & 0xFF;
        pc += 2;
        break;
    case SIZE_WORD:
        b = read_word(pc);
        pc += 2;
        break;
    case SIZE_LONG:
        b = read_word(pc) << 16;
        pc += 2;
        b |= read_word(pc);
        pc += 2;
        break;
    }
    //get_address_mode();
    compute_ea();
    preincrement_ea();
    uint32_t a = read_ea();
    uint32_t result = (a | b) & mask;
    write_ea(result);
    flag_n = result & highest_bit;
    flag_z = result == 0 ? 1 : 0;
    flag_v = 0;
    flag_c = 0;
    postincrement_ea();
    writeback_ea();
}

void c_m68k::ANDI_()
{
    get_size1();
    uint32_t b;
    switch (op_size) {
    case SIZE_BYTE:
        b = read_word(pc) & 0xFF;
        pc += 2;
        break;
    case SIZE_WORD:
        b = read_word(pc);
        pc += 2;
        break;
    case SIZE_LONG:
        b = read_word(pc) << 16;
        pc += 2;
        b |= read_word(pc);
        pc += 2;
        break;
    }
    //get_address_mode();
    compute_ea();
    preincrement_ea();
    uint32_t a = read_ea();
    uint32_t result = (a & b) & mask;
    write_ea(result);
    flag_n = result & highest_bit;
    flag_z = result == 0 ? 1 : 0;
    flag_v = 0;
    flag_c = 0;
    postincrement_ea();
    writeback_ea();
}

void c_m68k::AND_()
{
    get_size1();
    get_address_mode();
    compute_ea();
    preincrement_ea();
    uint32_t register_index = (op_word >> 9) & 0x7;
    uint32_t a = read_ea();
    uint32_t b = *d[register_index];
    uint32_t result = (a & b) & mask;
    if (op_word & 0x100) {
        //store to ea
        write_ea(result);
    }
    else {
        //store to register
        *d[register_index] = (*d[register_index] & ~mask) | (result & mask);
    }
    flag_n = result & highest_bit;
    flag_z = result == 0 ? 1 : 0;
    flag_v = 0;
    flag_c = 0;
    postincrement_ea();
    writeback_ea();
}

void c_m68k::ASd_()
{
    //ASR results might not be correct because tests are broken
    get_size1();
    get_address_mode();
    compute_ea();
    preincrement_ea();
    set_size(SIZE_WORD);

    int direction = op_word & 0x100; //0 = right, 1 = left

    uint32_t shift_count = 1;

    uint32_t result = read_ea();

    flag_v = 0;
    if (shift_count == 0) {
        flag_c = 0;
    }
    if (direction) {
        for (int i = 0; i < shift_count; i++) {
            //this can be optimized, but doing it this way for now to simplify flag stuff
            flag_x = flag_c = result & highest_bit;
            uint32_t hi_bit_before = result & highest_bit;
            result <<= 1;
            result &= mask;
            uint32_t hi_bit_after = result & highest_bit;
            if (hi_bit_before != hi_bit_after) {
                flag_v = 1;
            }
            int x = 1;
        }
    }
    else {
        for (int i = 0; i < shift_count; i++) {
            //this can be optimized, but doing it this way for now to simplify flag stuff
            flag_x = flag_c = result & 0x1;
            uint32_t high_bit = result & highest_bit;
            result >>= 1;
            result &= mask;
            result |= high_bit;
        }
    }
    flag_n = result & highest_bit;
    flag_z = result == 0;
    write_ea(result);
    postincrement_ea();
    writeback_ea();
}

void c_m68k::ASd2_()
{
    //ASR results might not be correct because tests are broken

    get_size1();

    int direction = op_word & 0x100; //0 = right, 1 = left
    int source = op_word & 0x20; //0 = immediate, 1 = register


    uint32_t shift_count = (op_word >> 9) & 0x7;
    int register_index = op_word & 0x7;

    if (op_word == 59939) {
        int x = 1;
    }

    if (source) {
        //register
        shift_count = *d[shift_count];
        shift_count &= 63;
    }
    else {
        if (shift_count == 0) {
            shift_count = 8;
        }
    }

    uint32_t result = *d[register_index] & mask;
    
    flag_v = 0;
    if (shift_count == 0) {
        flag_c = 0;
    }
    if (direction) {
        for (int i = 0; i < shift_count; i++) {
            //this can be optimized, but doing it this way for now to simplify flag stuff
            flag_x = flag_c = result & highest_bit;
            uint32_t hi_bit_before = result & highest_bit;
            result <<= 1;
            result &= mask;
            uint32_t hi_bit_after = result & highest_bit;
            if (hi_bit_before != hi_bit_after) {
                flag_v = 1;
            }
            int x = 1;
        }
    }
    else {
        for (int i = 0; i < shift_count; i++) {
            //this can be optimized, but doing it this way for now to simplify flag stuff
            flag_x = flag_c = result & 0x1;
            uint32_t high_bit = result & highest_bit;
            result >>= 1;
            result &= mask;
            result |= high_bit;
        }
    }
    flag_n = result & highest_bit;
    flag_z = result == 0;
    *d[register_index] = (*d[register_index] & ~mask) | (result & mask);
}

const std::array<uint8_t, 65536> c_m68k::instructions3 = [] {
    std::array<uint8_t, 65536> ret;
    struct s_opwords
    {
        char opword[17];
        uint8_t instruction;
        std::function<bool(uint16_t)> test = nullptr;
        uint16_t mask;
        uint16_t mask_match;
        uint16_t mask_len;
    };

    // clang-format off

	s_opwords opwords[] = {
		{"0000000000111100", ORItoCCR},
        {"0000000001111100", ORItoSR},
        {"00000000--------", ORI},
        {"0000001000111100", ANDItoCCR},
		{"0000001001111100", ANDItoSR},
		{"00000010--------", ANDI},
        {"00000100--------", SUBI},
		{"00000110--------", ADDI},
		{"0000101000111100", EORItoCCR},
		{"0000101001111100", EORItoSR},
		{"00001010--------", EORI},
		{"00001100--------", CMPI},
		{"0000100000------", BTST},
		{"0000100001------", BCHG},
		{"0000100010------", BCLR},
		{"0000100011------", BSET},
		{"0000---100------", BTST2},
		{"0000---101------", BCHG2},
		{"0000---110------", BCLR2},
		{"0000---111------", BSET2},
		{"0000---1--001---", MOVEP},
		{"00-----001------", MOVEA},
		{"00--------------", MOVE},
		{"0100000011------", MOVEfromSR},
		{"0100010011------", MOVEtoCCR},
		{"0100011011------", MOVEtoSR},
		{"01000000--------", NEGX},
		{"01000010--------", CLR},
		{"01000100--------", NEG},
		{"01000110--------", NOT},
		{"010010001-000---", EXT},
		{"0100100000------", NBCD},
		{"0100100001000---", SWAP},
		{"0100100001------", PEA},
		{"0100101011111100", ILLEGAL},
		{"0100101011------", TAS},
		{"01001010--------", TST},
		{"010011100100----", TRAP},
		{"0100111001010---", LINK},
		{"0100111001011---", UNLK},
		{"010011100110----", MOVE_USP},
		{"0100111001110000", RESET},
		{"0100111001110001", NOP},
		{"0100111001110010", STOP},
		{"0100111001110011", RTE},
		{"0100111001110101", RTS},
		{"0100111001110110", TRAPV},
		{"0100111001110111", RTR},
		{"0100111010------", JSR},
		{"0100111011------", JMP},
		{"01001-001-------", MOVEM},
		{"0100---111------", LEA},
		{"0100---110------", CHK},
		{"0101---0--------", ADDQ},
		{"0101---1--------", SUBQ},
		{"0101----11------", Scc},
		{"0101----11001---", DBcc},
		{"01100000--------", BRA},
		{"01100001--------", BSR},
		{"0110------------", Bcc},
		{"0111---0--------", MOVEQ},
		{"1000---011------", DIVU},
		{"1000---111------", DIVS},
		{"1000---10000----", SBCD},
		{"1000------------", OR},
		{"1001------------", SUB},
		{"1001---1--00----", SUBX, [](uint16_t o) {return ((o >> 6) & 3) < 3;}},
		{"1001----11------", SUBA},
		{"1011---1--------", EOR},
		{"1011---1--001---", CMPM, [](uint16_t o) {return ((o >> 6) & 3) < 3;}},
		{"1011---0--------", CMP},
		{"1011----11------", CMPA},
		{"1100---011------", MULU},
		{"1100---111------", MULS},
		{"1100---10000----", ABCD},
		{"1100---101000---", EXGd},
		{"1100---101001---", EXGa},
		{"1100---110001---", EXGda},
		{"1100------------", AND},
		{"1101------------", ADD},
		{"1101---1--00----", ADDX, [](uint16_t o) {return ((o >> 6) & 3) < 3;}},
		{"1101----11------", ADDA},
		{"1110000-11------", ASd},
		{"1110001-11------", LSd},
		{"1110010-11------", ROXd},
		{"1110011-11------", ROd},
		{"1110-------00---", ASd2},
		{"1110-------01---", LSd2},
		{"1110-------10---", ROXd2},
		{"1110-------11---", ROd2},
	};

    // clang-format on

    for (auto &o : opwords) {
        o.mask = 0;
        o.mask_match = 0;
        o.mask_len = 0;
        int x = 0;
        for (int i = 0; i < std::strlen(o.opword); i++) {
            char c = o.opword[i];
            x++;
            o.mask <<= 1;
            o.mask_match <<= 1;
            if (c == '1') {
                o.mask |= 1;
                o.mask_match |= 1;
                o.mask_len += 1;
            }
            else if (c == '0') {
                o.mask |= 1;
                o.mask_len += 1;
            }
        }
    }

    for (int i = 0; i < 65536; i++) {
        int best_len = 0;
        int matched_opcode = INVALID;
        for (auto &o : opwords) {
            if ((i & o.mask) == o.mask_match) {
                if (o.test != nullptr) {
                    if (!o.test(i)) {
                        continue;
                    }
                }
                if (o.mask_len > best_len) {
                    matched_opcode = o.instruction;
                    best_len = o.mask_len;
                }
            }
        }
        ret[i] = matched_opcode;
    }
    return ret;
}();