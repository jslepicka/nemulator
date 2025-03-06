module;

#define DEBUG 0

#define dprintf(fmt, ...)                                                                                              \
    do {                                                                                                               \
        if (DEBUG)                                                                                                     \
            printf(fmt, __VA_ARGS__);                                                                                  \
    } while (0)

module gb:sm83;
import gb;


namespace gb
{

// clang-format off
alignas(64) const c_sm83::s_ins c_sm83::ins_info[517] = {
              /*x0*/    /*x1*/    /*x2*/    /*x3*/    /*x4*/    /*x5*/    /*x6*/    /*x7*/    /*x8*/    /*x9*/    /*xA*/    /*xB*/    /*xC*/    /*xD*/    /*xE*/    /*xF*/
    /*  0x*/  {1, 4},  {3, 12},   {1, 8},   {1, 8},   {1, 4},   {1, 4},   {2, 8},   {1, 4},  {3, 20},   {1, 8},   {1, 8},   {1, 8},   {1, 4},   {1, 4},   {2, 8},   {1, 4},
    /*  1x*/  {2, 4},  {3, 12},   {1, 8},   {1, 8},   {1, 4},   {1, 4},   {2, 8},   {1, 4},  {2, 12},   {1, 8},   {1, 8},   {1, 8},   {1, 4},   {1, 4},   {2, 8},   {1, 4},
    /*  2x*/  {2, 8},  {3, 12},   {1, 8},   {1, 8},   {1, 4},   {1, 4},   {2, 8},   {1, 4},   {2, 8},   {1, 8},   {1, 8},   {1, 8},   {1, 4},   {1, 4},   {2, 8},   {1, 4},
    /*  3x*/  {2, 8},  {3, 12},   {1, 8},   {1, 8},  {1, 12},  {1, 12},  {2, 12},   {1, 4},   {2, 8},   {1, 8},   {1, 8},   {1, 8},   {1, 4},   {1, 4},   {2, 8},   {1, 4},
    /*  4x*/  {1, 4},   {1, 4},   {1, 4},   {1, 4},   {1, 4},   {1, 4},   {1, 8},   {1, 4},   {1, 4},   {1, 4},   {1, 4},   {1, 4},   {1, 4},   {1, 4},   {1, 8},   {1, 4},
    /*  5x*/  {1, 4},   {1, 4},   {1, 4},   {1, 4},   {1, 4},   {1, 4},   {1, 8},   {1, 4},   {1, 4},   {1, 4},   {1, 4},   {1, 4},   {1, 4},   {1, 4},   {1, 8},   {1, 4},
    /*  6x*/  {1, 4},   {1, 4},   {1, 4},   {1, 4},   {1, 4},   {1, 4},   {1, 8},   {1, 4},   {1, 4},   {1, 4},   {1, 4},   {1, 4},   {1, 4},   {1, 4},   {1, 8},   {1, 4},
    /*  7x*/  {1, 8},   {1, 8},   {1, 8},   {1, 8},   {1, 8},   {1, 8},   {1, 4},   {1, 8},   {1, 4},   {1, 4},   {1, 4},   {1, 4},   {1, 4},   {1, 4},   {1, 8},   {1, 4},
    /*  8x*/  {1, 4},   {1, 4},   {1, 4},   {1, 4},   {1, 4},   {1, 4},   {1, 8},   {1, 4},   {1, 4},   {1, 4},   {1, 4},   {1, 4},   {1, 4},   {1, 4},   {1, 8},   {1, 4},
    /*  9x*/  {1, 4},   {1, 4},   {1, 4},   {1, 4},   {1, 4},   {1, 4},   {1, 8},   {1, 4},   {1, 4},   {1, 4},   {1, 4},   {1, 4},   {1, 4},   {1, 4},   {1, 8},   {1, 4},
    /*  Ax*/  {1, 4},   {1, 4},   {1, 4},   {1, 4},   {1, 4},   {1, 4},   {1, 8},   {1, 4},   {1, 4},   {1, 4},   {1, 4},   {1, 4},   {1, 4},   {1, 4},   {1, 8},   {1, 4},
    /*  Bx*/  {1, 4},   {1, 4},   {1, 4},   {1, 4},   {1, 4},   {1, 4},   {1, 8},   {1, 4},   {1, 4},   {1, 4},   {1, 4},   {1, 4},   {1, 4},   {1, 4},   {1, 8},   {1, 4},
    /*  Cx*/  {1, 8},  {1, 12},  {3, 12},  {3, 16},  {3, 12},  {1, 16},   {2, 8},  {1, 16},   {1, 8},  {1, 16},  {3, 12},   {1, 0},  {3, 12},  {3, 24},   {2, 8},  {1, 16},
    /*  Dx*/  {1, 8},  {1, 12},  {3, 12},   {1, 0},  {3, 12},  {1, 16},   {2, 8},  {1, 16},   {1, 8},  {1, 16},  {3, 12},   {1, 0},  {3, 12},   {1, 0},   {2, 8},  {1, 16},
    /*  Ex*/ {2, 12},  {1, 12},   {1, 8},   {1, 0},   {1, 0},  {1, 16},   {2, 8},  {1, 16},  {2, 16},   {1, 4},  {3, 16},   {1, 0},   {1, 0},   {1, 0},   {2, 8},  {1, 16},
    /*  Fx*/ {2, 12},  {1, 12},   {1, 8},   {1, 4},   {1, 0},  {1, 16},   {2, 8},  {1, 16},  {2, 12},   {1, 8},  {3, 16},   {1, 4},   {1, 0},   {1, 0},   {2, 8},  {1, 16},

    /* 10x*/  {1, 8},   {1, 8},   {1, 8},   {1, 8},   {1, 8},   {1, 8},  {1, 16},   {1, 8},   {1, 8},   {1, 8},   {1, 8},   {1, 8},   {1, 8},   {1, 8},  {1, 16},   {1, 8},
    /* 11x*/  {1, 8},   {1, 8},   {1, 8},   {1, 8},   {1, 8},   {1, 8},  {1, 16},   {1, 8},   {1, 8},   {1, 8},   {1, 8},   {1, 8},   {1, 8},   {1, 8},  {1, 16},   {1, 8},
    /* 12x*/  {1, 8},   {1, 8},   {1, 8},   {1, 8},   {1, 8},   {1, 8},  {1, 16},   {1, 8},   {1, 8},   {1, 8},   {1, 8},   {1, 8},   {1, 8},   {1, 8},  {1, 16},   {1, 8},
    /* 13x*/  {1, 8},   {1, 8},   {1, 8},   {1, 8},   {1, 8},   {1, 8},  {1, 16},   {1, 8},   {1, 8},   {1, 8},   {1, 8},   {1, 8},   {1, 8},   {1, 8},  {1, 16},   {1, 8},
    /* 14x*/  {1, 8},   {1, 8},   {1, 8},   {1, 8},   {1, 8},   {1, 8},  {1, 12},   {1, 8},   {1, 8},   {1, 8},   {1, 8},   {1, 8},   {1, 8},   {1, 8},  {1, 12},   {1, 8},
    /* 15x*/  {1, 8},   {1, 8},   {1, 8},   {1, 8},   {1, 8},   {1, 8},  {1, 12},   {1, 8},   {1, 8},   {1, 8},   {1, 8},   {1, 8},   {1, 8},   {1, 8},  {1, 12},   {1, 8},
    /* 16x*/  {1, 8},   {1, 8},   {1, 8},   {1, 8},   {1, 8},   {1, 8},  {1, 12},   {1, 8},   {1, 8},   {1, 8},   {1, 8},   {1, 8},   {1, 8},   {1, 8},  {1, 12},   {1, 8},
    /* 17x*/  {1, 8},   {1, 8},   {1, 8},   {1, 8},   {1, 8},   {1, 8},  {1, 12},   {1, 8},   {1, 8},   {1, 8},   {1, 8},   {1, 8},   {1, 8},   {1, 8},  {1, 12},   {1, 8},
    /* 18x*/  {1, 8},   {1, 8},   {1, 8},   {1, 8},   {1, 8},   {1, 8},  {1, 16},   {1, 8},   {1, 8},   {1, 8},   {1, 8},   {1, 8},   {1, 8},   {1, 8},  {1, 16},   {1, 8},
    /* 19x*/  {1, 8},   {1, 8},   {1, 8},   {1, 8},   {1, 8},   {1, 8},  {1, 16},   {1, 8},   {1, 8},   {1, 8},   {1, 8},   {1, 8},   {1, 8},   {1, 8},  {1, 16},   {1, 8},
    /* 1Ax*/  {1, 8},   {1, 8},   {1, 8},   {1, 8},   {1, 8},   {1, 8},  {1, 16},   {1, 8},   {1, 8},   {1, 8},   {1, 8},   {1, 8},   {1, 8},   {1, 8},  {1, 16},   {1, 8},
    /* 1Bx*/  {1, 8},   {1, 8},   {1, 8},   {1, 8},   {1, 8},   {1, 8},  {1, 16},   {1, 8},   {1, 8},   {1, 8},   {1, 8},   {1, 8},   {1, 8},   {1, 8},  {1, 16},   {1, 8},
    /* 1Cx*/  {1, 8},   {1, 8},   {1, 8},   {1, 8},   {1, 8},   {1, 8},  {1, 16},   {1, 8},   {1, 8},   {1, 8},   {1, 8},   {1, 8},   {1, 8},   {1, 8},  {1, 16},   {1, 8},
    /* 1Dx*/  {1, 8},   {1, 8},   {1, 8},   {1, 8},   {1, 8},   {1, 8},  {1, 16},   {1, 8},   {1, 8},   {1, 8},   {1, 8},   {1, 8},   {1, 8},   {1, 8},  {1, 16},   {1, 8},
    /* 1Ex*/  {1, 8},   {1, 8},   {1, 8},   {1, 8},   {1, 8},   {1, 8},  {1, 16},   {1, 8},   {1, 8},   {1, 8},   {1, 8},   {1, 8},   {1, 8},   {1, 8},  {1, 16},   {1, 8},
    /* 1Fx*/  {1, 8},   {1, 8},   {1, 8},   {1, 8},   {1, 8},   {1, 8},  {1, 16},   {1, 8},   {1, 8},   {1, 8},   {1, 8},   {1, 8},   {1, 8},   {1, 8},  {1, 16},   {1, 8},

    /* 20x*/ {1, 20},  {1, 20},  {1, 20},  {1, 20},  {1, 20}
};
// clang-format on

c_sm83::c_sm83(c_gb *g)
{
    gb = g;
}

void c_sm83::reset(uint16_t PC)
{
    available_cycles = 0;
    required_cycles = 0;
    fetch_opcode = 1;
    this->PC = PC;
    SP = 0xFFFE;
    opcode = 0;
    ins_start = 0;
    AF.w = gb->get_model() == GB_MODEL::CGB ? 0x11B0 : 0x01B0;
    BC.w = 0x0013;
    DE.w = 0x00D8;
    HL.w = 0x014D;
    fz = 1;
    fn = 0;
    fh = 1;
    fc = 1;
    //todo: check initial state
    IME = 0;
    pending_ei = 0;
    instruction_len = 0;
    halted = 0;
}

void c_sm83::execute(int cycles)
{
    available_cycles += cycles;
    while (true) {
        if (fetch_opcode) {
            fetch_opcode = 0;
            int offset = 0;
            prefix = 0;
            ins_start = PC;
            if (halted && gb->IF) {
                int x = 1;
            }
            if ((gb->IF & gb->IE) && (IME || halted)) {
                if (IME) {
                    int x = gb->IF & gb->IE;
                    int i = 0;
                    while (x) {
                        if (x & 1) {
                            opcode = i;
                            offset = 512;
                            gb->IF &= ~(1 << i);
                            break;
                        }
                        i++;
                        x >>= 1;
                    }
                }
                else if (halted) {
                    available_cycles -= 4;
                }
                else {
                    int x = 1;
                }
                halted = 0;
            }
            else if (halted) {
                opcode = 0x00;
            }
            else {
                opcode = gb->read_byte(PC++);
                if (opcode == 0xCB) {
                    opcode = gb->read_byte(PC++);
                    prefix = 0xCB;
                    offset = 256;
                }
            }

            //instruction = &instructions[opcode + offset];
            opcode += offset;

            required_cycles = ins_info[opcode].cycles;
            instruction_len = ins_info[opcode].len;
            dprintf("PC:%04X SP:%04X %c%c%c%c\t", ins_start, SP, fz ? 'Z' : 'z', fn ? 'N' : 'n', fh ? 'H' : 'h',
                    fc ? 'C' : 'c');
            dprintf("A:%02X F:%02X B:%02X C:%02X D:%02X E:%02X H:%02X L:%02X\n", AF.b.h, AF.b.l, BC.b.h, BC.b.l, DE.b.h,
                    DE.b.l, HL.b.h, HL.b.l);

            // dprintf("fetched opcode %02X, requires %d cycles\n", opcode, required_cycles);
            if (required_cycles == 0) {
                //char txt[256];
                //sprintf(txt, "Missing cycle count for opcode %s%02X at %04X\n", prefix == 0xCB ? "CB" : "", opcode,
                //ins_start); MessageBox(NULL, txt, "oops", MB_OK); exit(0);
            }
            if (pending_ei) {
                pending_ei = 0;
                IME = 1;
            }
        }
        if (available_cycles >= required_cycles) {
            available_cycles -= required_cycles;
            fetch_opcode = 1;

            execute_opcode();
        }
        else {
            break;
        }
    }
}

void c_sm83::execute_opcode()
{
    if (ins_start == 0xc33d) {
        int x = 1;
    }
    uint8_t l, h;
    switch (instruction_len) {
        case 2:
            o = gb->read_byte(PC++);
            break;
        case 3:
            l = gb->read_byte(PC++);
            h = gb->read_byte(PC++);
            o = (h << 8) | l;
            break;
    }
    // clang-format off
    switch (opcode) {
    case 0x000: {} break; //NOP
    case 0x001: {BC.w = o; } break; //LD BC, %04X
    case 0x002: {gb->write_byte(BC.w, AF.b.h); } break; //LD (BC), A
    case 0x003: {BC.w++; } break; //INC BC
    case 0x004: {INC(BC.b.h); } break; //INC B
    case 0x005: {DEC(BC.b.h); } break; //DEC B
    case 0x006: {BC.b.h = o; } break; //LD B, %02X
    case 0x007: {fc = (AF.b.h & 0x80) ? 1 : 0; AF.b.h <<= 1; AF.b.h |= fc; fz = fn = fh = 0; } break; //RLCA
    case 0x008: {gb->write_word(o, SP); } break; //LD (%04X), SP
    case 0x009: {ADD16(HL.w, BC.w); } break; //ADD HL, BC
    case 0x00A: {AF.b.h = gb->read_byte(BC.w); } break; //LD A, (BC)
    case 0x00B: {BC.w--; } break; //DEC BC
    case 0x00C: {INC(BC.b.l); } break; //INC C
    case 0x00D: {DEC(BC.b.l); } break; //DEC C
    case 0x00E: {BC.b.l = o; } break; //LD C, %02X
    case 0x00F: {fc = AF.b.h & 0x1 ? 0x80 : 0; AF.b.h >>= 1; AF.b.h |= fc; fz = fn = fh = 0; } break; //RRCA
    case 0x010: {STOP();} break; //STOP
    case 0x011: {DE.w = o; } break; //LD DE, %04X
    case 0x012: {gb->write_byte(DE.w, AF.b.h); } break; //LD (DE), A
    case 0x013: {DE.w++; } break; //INC DE
    case 0x014: {INC(DE.b.h); } break; //INC D
    case 0x015: {DEC(DE.b.h); } break; //DEC D
    case 0x016: {DE.b.h = o; } break; //LD D, %02X
    case 0x017: {int c = fc ? 1 : 0; fc = AF.b.h & 0x80; AF.b.h <<= 1; AF.b.h |= c; fz = fn = fh = 0; } break; //RLA
    case 0x018: {PC += (int8_t)(o & 0xFF); } break; //JR %02X
    case 0x019: {ADD16(HL.w, DE.w); } break; //ADD HL, DE
    case 0x01A: {AF.b.h = gb->read_byte(DE.w); } break; //LD A, (DE)
    case 0x01B: {DE.w--; } break; //DEC DE
    case 0x01C: {INC(DE.b.l); } break; //INC E
    case 0x01D: {DEC(DE.b.l); } break; //DEC E
    case 0x01E: {DE.b.l = o; } break; //LD E, %02X
    case 0x01F: {int c = fc ? 0x80 : 0; fc = AF.b.h & 0x1; AF.b.h >>= 1; AF.b.h |= c; fz = fn = fh = 0; } break; //RRA
    case 0x020: {if (!fz) { PC += (int8_t)(o & 0xFF); available_cycles -= 4; } } break; //JR NZ, %02X
    case 0x021: {HL.w = o; } break; //LD HL, %04X
    case 0x022: {gb->write_byte(HL.w, AF.b.h); HL.w++; } break; //LD (HL+), A
    case 0x023: {HL.w++; } break; //INC HL
    case 0x024: {INC(HL.b.h); } break; //INC H
    case 0x025: {DEC(HL.b.h); } break; //DEC H
    case 0x026: {HL.b.h = o; } break; //LD H, %02X
    case 0x027: {DAA(); } break; //DAA
    case 0x028: {if (fz) { PC += (int8_t)(o & 0xFF); available_cycles -= 4; } } break; //JR Z, %02X
    case 0x029: {ADD16(HL.w, HL.w); } break; //ADD HL, HL
    case 0x02A: {AF.b.h = gb->read_byte(HL.w); HL.w++; } break; //LD A, (HL+)
    case 0x02B: {HL.w--; } break; //DEC HL
    case 0x02C: {INC(HL.b.l); } break; //INC L
    case 0x02D: {DEC(HL.b.l); } break; //DEC L
    case 0x02E: {HL.b.l = o; } break; //LD L, %02X
    case 0x02F: {AF.b.h = ~AF.b.h; fn = fh = 1; } break; //CPL
    case 0x030: {if (!fc) { PC += (int8_t)(o & 0xFF); available_cycles -= 4; } } break; //JR NC, %02X
    case 0x031: {SP = o; } break; //LD SP, %04X
    case 0x032: {gb->write_byte(HL.w, AF.b.h); HL.w--; } break; //LD (HL-), A
    case 0x033: {SP++; } break; //INC SP
    case 0x034: {uint8_t d = gb->read_byte(HL.w); INC(d); gb->write_byte(HL.w, d); } break; //INC (HL)
    case 0x035: {uint8_t d = gb->read_byte(HL.w); DEC(d); gb->write_byte(HL.w, d); } break; //DEC (HL)
    case 0x036: {gb->write_byte(HL.w, o); } break; //LD (HL), %02x
    case 0x037: {fc = 1; fn = fh = 0; } break; //SCF
    case 0x038: {if (fc) { PC += (int8_t)(o & 0xFF); available_cycles -= 4; } } break; //JR C, %02X
    case 0x039: {ADD16(HL.w, SP); } break; //ADD HL, SP
    case 0x03A: {AF.b.h = gb->read_byte(HL.w); HL.w--; } break; //LD A, (HL-)
    case 0x03B: {SP--; } break; //DEC SP
    case 0x03C: {INC(AF.b.h); } break; //INC A
    case 0x03D: {DEC(AF.b.h); } break; //DEC A
    case 0x03E: {AF.b.h = o; } break; //LD A, %02X
    case 0x03F: {fc = fc ? 0 : 1; fn = fh = 0; } break; //CCF
    case 0x040: {BC.b.h = BC.b.h; } break; //LD B, B
    case 0x041: {BC.b.h = BC.b.l; } break; //LD B, C
    case 0x042: {BC.b.h = DE.b.h; } break; //LD B, D
    case 0x043: {BC.b.h = DE.b.l; } break; //LD B, E
    case 0x044: {BC.b.h = HL.b.h; } break; //LD B, H
    case 0x045: {BC.b.h = HL.b.l; } break; //LD B, L
    case 0x046: {BC.b.h = gb->read_byte(HL.w); } break; //LD B, (HL)
    case 0x047: {BC.b.h = AF.b.h; } break; //LD B, A
    case 0x048: {BC.b.l = BC.b.h; } break; //LD C, B
    case 0x049: {BC.b.l = BC.b.l; } break; //LD C, C
    case 0x04A: {BC.b.l = DE.b.h; } break; //LD C, D
    case 0x04B: {BC.b.l = DE.b.l; } break; //LD C, E
    case 0x04C: {BC.b.l = HL.b.h; } break; //LD C, H
    case 0x04D: {BC.b.l = HL.b.l; } break; //LD C, L
    case 0x04E: {BC.b.l = gb->read_byte(HL.w); } break; //LD C, (HL)
    case 0x04F: {BC.b.l = AF.b.h; } break; //LD C, A
    case 0x050: {DE.b.h = BC.b.h; } break; //LD D, B
    case 0x051: {DE.b.h = BC.b.l; } break; //LD D, C
    case 0x052: {DE.b.h = DE.b.h; } break; //LD D, D
    case 0x053: {DE.b.h = DE.b.l; } break; //LD D, E
    case 0x054: {DE.b.h = HL.b.h; } break; //LD D, H
    case 0x055: {DE.b.h = HL.b.l; } break; //LD D, L
    case 0x056: {DE.b.h = gb->read_byte(HL.w); } break; //LD D, (HL)
    case 0x057: {DE.b.h = AF.b.h; } break; //LD D, A
    case 0x058: {DE.b.l = BC.b.h; } break; //LD E, B
    case 0x059: {DE.b.l = BC.b.l; } break; //LD E, C
    case 0x05A: {DE.b.l = DE.b.h; } break; //LD E, D
    case 0x05B: {DE.b.l = DE.b.l; } break; //LD E, E
    case 0x05C: {DE.b.l = HL.b.h; } break; //LD E, H
    case 0x05D: {DE.b.l = HL.b.l; } break; //LD E, L
    case 0x05E: {DE.b.l = gb->read_byte(HL.w); } break; //LD E, (HL)
    case 0x05F: {DE.b.l = AF.b.h; } break; //LD E, A
    case 0x060: {HL.b.h = BC.b.h; } break; //LD H, B
    case 0x061: {HL.b.h = BC.b.l; } break; //LD H, C
    case 0x062: {HL.b.h = DE.b.h; } break; //LD H, D
    case 0x063: {HL.b.h = DE.b.l; } break; //LD H, E
    case 0x064: {HL.b.h = HL.b.h; } break; //LD H, H
    case 0x065: {HL.b.h = HL.b.l; } break; //LD H, L
    case 0x066: {HL.b.h = gb->read_byte(HL.w); } break; //LD H, (HL)
    case 0x067: {HL.b.h = AF.b.h; } break; //LD H, A
    case 0x068: {HL.b.l = BC.b.h; } break; //LD L, B
    case 0x069: {HL.b.l = BC.b.l; } break; //LD L, C
    case 0x06A: {HL.b.l = DE.b.h; } break; //LD L, D
    case 0x06B: {HL.b.l = DE.b.l; } break; //LD L, E
    case 0x06C: {HL.b.l = HL.b.h; } break; //LD L, H
    case 0x06D: {HL.b.l = HL.b.l; } break; //LD L, L
    case 0x06E: {HL.b.l = gb->read_byte(HL.w); } break; //LD L, (HL)
    case 0x06F: {HL.b.l = AF.b.h; } break; //LD L, A
    case 0x070: {gb->write_byte(HL.w, BC.b.h); } break; //LD (HL), B
    case 0x071: {gb->write_byte(HL.w, BC.b.l); } break; //LD (HL), C
    case 0x072: {gb->write_byte(HL.w, DE.b.h); } break; //LD (HL), D
    case 0x073: {gb->write_byte(HL.w, DE.b.l); } break; //LD (HL), E
    case 0x074: {gb->write_byte(HL.w, HL.b.h); } break; //LD (HL), H
    case 0x075: {gb->write_byte(HL.w, HL.b.l); } break; //LD (HL), L
    case 0x076: {halted = 1; } break; //HALT
    case 0x077: {gb->write_byte(HL.w, AF.b.h); } break; //LD (HL), A
    case 0x078: {AF.b.h = BC.b.h; } break; //LD A, B
    case 0x079: {AF.b.h = BC.b.l; } break; //LD A, C
    case 0x07A: {AF.b.h = DE.b.h; } break; //LD A, D
    case 0x07B: {AF.b.h = DE.b.l; } break; //LD A, E
    case 0x07C: {AF.b.h = HL.b.h; } break; //LD A, H
    case 0x07D: {AF.b.h = HL.b.l; } break; //LD A, L
    case 0x07E: {AF.b.h = gb->read_byte(HL.w); } break; //LD A, (HL)
    case 0x07F: {AF.b.h = AF.b.h; } break; //LD A, A
    case 0x080: {ADD8(BC.b.h); } break; //ADD A, B
    case 0x081: {ADD8(BC.b.l); } break; //ADD A, C
    case 0x082: {ADD8(DE.b.h); } break; //ADD A, D
    case 0x083: {ADD8(DE.b.l); } break; //ADD A, E
    case 0x084: {ADD8(HL.b.h); } break; //ADD A, H
    case 0x085: {ADD8(HL.b.l); } break; //ADD A, L
    case 0x086: {ADD8(gb->read_byte(HL.w)); } break; //ADD A, (HL)
    case 0x087: {ADD8(AF.b.h); } break; //ADD A, A
    case 0x088: {ADC8(BC.b.h); } break; //ADC A, B
    case 0x089: {ADC8(BC.b.l); } break; //ADC A, C
    case 0x08A: {ADC8(DE.b.h); } break; //ADC A, D
    case 0x08B: {ADC8(DE.b.l); } break; //ADC A, E
    case 0x08C: {ADC8(HL.b.h); } break; //ADC A, H
    case 0x08D: {ADC8(HL.b.l); } break; //ADC A, L
    case 0x08E: {ADC8(gb->read_byte(HL.w)); } break; //ADC A, (HL)
    case 0x08F: {ADC8(AF.b.h); } break; //ADC A, A
    case 0x090: {SUB8(BC.b.h); } break; //SUB B
    case 0x091: {SUB8(BC.b.l); } break; //SUB C
    case 0x092: {SUB8(DE.b.h); } break; //SUB D
    case 0x093: {SUB8(DE.b.l); } break; //SUB E
    case 0x094: {SUB8(HL.b.h); } break; //SUB H
    case 0x095: {SUB8(HL.b.l); } break; //SUB L
    case 0x096: {SUB8(gb->read_byte(HL.w)); } break; //SUB (HL)
    case 0x097: {SUB8(AF.b.h); } break; //SUB A
    case 0x098: {SBC8(BC.b.h); } break; //SBC A, B
    case 0x099: {SBC8(BC.b.l); } break; //SBC A, C
    case 0x09A: {SBC8(DE.b.h); } break; //SBC A, D
    case 0x09B: {SBC8(DE.b.l); } break; //SBC A, E
    case 0x09C: {SBC8(HL.b.h); } break; //SBC A, H
    case 0x09D: {SBC8(HL.b.l); } break; //SBC A, L
    case 0x09E: {SBC8(gb->read_byte(HL.w)); } break; //SBC (HL)
    case 0x09F: {SBC8(AF.b.h); } break; //SBC A, A
    case 0x0A0: {AND(BC.b.h); } break; //AND B
    case 0x0A1: {AND(BC.b.l); } break; //AND C
    case 0x0A2: {AND(DE.b.h); } break; //AND D
    case 0x0A3: {AND(DE.b.l); } break; //AND E
    case 0x0A4: {AND(HL.b.h); } break; //AND H
    case 0x0A5: {AND(HL.b.l); } break; //AND L
    case 0x0A6: {AND(gb->read_byte(HL.w)); } break; //AND (HL)
    case 0x0A7: {AND(AF.b.h); } break; //AND A
    case 0x0A8: {XOR(BC.b.h); } break; //XOR B
    case 0x0A9: {XOR(BC.b.l); } break; //XOR C
    case 0x0AA: {XOR(DE.b.h); } break; //XOR D
    case 0x0AB: {XOR(DE.b.l); } break; //XOR E
    case 0x0AC: {XOR(HL.b.h); } break; //XOR H
    case 0x0AD: {XOR(HL.b.l); } break; //XOR L
    case 0x0AE: {XOR(gb->read_byte(HL.w)); } break; //XOR (HL)
    case 0x0AF: {XOR(AF.b.h); } break; //XOR A
    case 0x0B0: {OR(BC.b.h); } break; //OR B
    case 0x0B1: {OR(BC.b.l); } break; //OR C
    case 0x0B2: {OR(DE.b.h); } break; //OR D
    case 0x0B3: {OR(DE.b.l); } break; //OR E
    case 0x0B4: {OR(HL.b.h); } break; //OR H
    case 0x0B5: {OR(HL.b.l); } break; //OR L
    case 0x0B6: {OR(gb->read_byte(HL.w)); } break; //OR (HL)
    case 0x0B7: {OR(AF.b.h); } break; //OR A
    case 0x0B8: {CP(BC.b.h); } break; //CP B
    case 0x0B9: {CP(BC.b.l); } break; //CP C
    case 0x0BA: {CP(DE.b.h); } break; //CP D
    case 0x0BB: {CP(DE.b.l); } break; //CP E
    case 0x0BC: {CP(HL.b.h); } break; //CP H
    case 0x0BD: {CP(HL.b.l); } break; //CP L
    case 0x0BE: {CP(gb->read_byte(HL.w)); } break; //CP (HL)
    case 0x0BF: {CP(AF.b.h); } break; //CP A
    case 0x0C0: {if (!fz) { PC = pop_word(); available_cycles -= 12; } } break; //RET NZ
    case 0x0C1: {BC.w = pop_word(); } break; //POP BC
    case 0x0C2: {if (!fz) { PC = o; available_cycles -= 4; } } break; //JP NZ, %04X
    case 0x0C3: {PC = o; } break; //JP %04X
    case 0x0C4: {if (!fz) { CALL(o); available_cycles -= 12; } } break; //CALL NZ, %04X
    case 0x0C5: {push_word(BC.w); } break; //PUSH BC
    case 0x0C6: {ADD8(o); } break; //ADD A, %02X
    case 0x0C7: {push_word(PC); PC = 0x00; } break; //RST 00H
    case 0x0C8: {if (fz) { PC = pop_word(); available_cycles -= 12; } } break; //RET Z
    case 0x0C9: {PC = pop_word(); } break; //RET
    case 0x0CA: {if (fz) { PC = o; available_cycles -= 4; }} break; //JP Z, %04X
    case 0x0CB: {} break; //
    case 0x0CC: {if (fz) { CALL(o); available_cycles -= 12; } } break; //CALL Z, %04X
    case 0x0CD: {CALL(o); } break; //CALL %04X
    case 0x0CE: {ADC8(o); } break; //ADC A, %02X
    case 0x0CF: {push_word(PC); PC = 0x08; } break; //RST 08H
    case 0x0D0: {if (!fc) { PC = pop_word(); available_cycles -= 12; } } break; //RET NC
    case 0x0D1: {DE.w = pop_word(); } break; //POP DE
    case 0x0D2: {if (!fc) { PC = o; available_cycles -= 4; } } break; //JP NC, %04X
    case 0x0D3: {} break; //
    case 0x0D4: {if (!fc) { CALL(o); available_cycles -= 12; } } break; //CALL NC, %04X
    case 0x0D5: {push_word(DE.w); } break; //PUSH DE
    case 0x0D6: {SUB8(o); } break; //SUB %02X
    case 0x0D7: {push_word(PC); PC = 0x10; } break; //RST 10H
    case 0x0D8: {if (fc) { PC = pop_word(); available_cycles -= 12; } } break; //RET C
    case 0x0D9: {PC = pop_word(); pending_ei = 1; } break; //RETI
    case 0x0DA: {if (fc) { PC = o; available_cycles -= 4; } } break; //JP C, %04X
    case 0x0DB: {} break; //
    case 0x0DC: {if (fc) { CALL(o); available_cycles -= 12; } } break; //CALL C, %04X
    case 0x0DD: {} break; //
    case 0x0DE: {SBC8(o); } break; //SBC A, %02X
    case 0x0DF: {push_word(PC); PC = 0x18; } break; //RST 18H
    case 0x0E0: {gb->write_byte(0xFF00 + o, AF.b.h); } break; //LD ($FF00+%02X), A
    case 0x0E1: {HL.w = pop_word(); } break; //POP HL
    case 0x0E2: {gb->write_byte(0xFF00 + BC.b.l, AF.b.h); } break; //LD ($FF00+C), A
    case 0x0E3: {} break; //
    case 0x0E4: {} break; //
    case 0x0E5: {push_word(HL.w); } break; //PUSH HL
    case 0x0E6: {AND(o); } break; //AND %02X
    case 0x0E7: {push_word(PC); PC = 0x20; } break; //RST 20H
    case 0x0E8: {ADD16r(SP, (int8_t)o);  } break; //ADD SP, %02X
    case 0x0E9: {PC = HL.w; } break; //JP (HL)
    case 0x0EA: {gb->write_byte(o, AF.b.h); } break; //LD (%04X), A
    case 0x0EB: {} break; //
    case 0x0EC: {} break; //
    case 0x0ED: {} break; //
    case 0x0EE: {XOR(o); } break; //XOR %02X
    case 0x0EF: {push_word(PC); PC = 0x28; } break; //RST 28H
    case 0x0F0: {AF.b.h = gb->read_byte(0xFF00 + o); } break; //LD A, ($FF00+%02X)
    case 0x0F1: {AF.w = pop_word(); update_flags(); } break; //POP AF
    case 0x0F2: {AF.b.h = gb->read_byte(0xFF00 + BC.b.l); } break; //LD A, ($FF00+C)
    case 0x0F3: {pending_ei = 0; IME = 0; } break; //DI
    case 0x0F4: {} break; //
    case 0x0F5: {update_f();  push_word(AF.w); } break; //PUSH AF
    case 0x0F6: {OR(o); } break; //OR %02X
    case 0x0F7: {push_word(PC); PC = 0x30; } break; //RST 30H
    case 0x0F8: {uint16_t t = SP; ADD16r(t, (int8_t)o); HL.w = t; } break; //LD HL, SP+%02X
    case 0x0F9: {SP = HL.w; } break; //LD SP, HL
    case 0x0FA: {AF.b.h = gb->read_byte(o); } break; //LD A, (%04X)
    case 0x0FB: {pending_ei = 1; } break; //EI
    case 0x0FC: {} break; //
    case 0x0FD: {} break; //
    case 0x0FE: {CP(o); } break; //CP %02X
    case 0x0FF: {push_word(PC); PC = 0x38; } break; //RST 38H
    case 0x100: {RLC(BC.b.h); } break; //RLC B
    case 0x101: {RLC(BC.b.l); } break; //RLC C
    case 0x102: {RLC(DE.b.h); } break; //RLC D
    case 0x103: {RLC(DE.b.l); } break; //RLC E
    case 0x104: {RLC(HL.b.h); } break; //RLC H
    case 0x105: {RLC(HL.b.l); } break; //RLC L
    case 0x106: {uint8_t d = gb->read_byte(HL.w); RLC(d); gb->write_byte(HL.w, d); } break; //RLC (HL)
    case 0x107: {RLC(AF.b.h); } break; //RLC A
    case 0x108: {RRC(BC.b.h); } break; //RRC B
    case 0x109: {RRC(BC.b.l); } break; //RRC C
    case 0x10A: {RRC(DE.b.h); } break; //RRC D
    case 0x10B: {RRC(DE.b.l); } break; //RRC E
    case 0x10C: {RRC(HL.b.h); } break; //RRC H
    case 0x10D: {RRC(HL.b.l); } break; //RRC L
    case 0x10E: {uint8_t d = gb->read_byte(HL.w); RRC(d); gb->write_byte(HL.w, d); } break; //RRC (HL)
    case 0x10F: {RRC(AF.b.h); } break; //RRC A
    case 0x110: {RL(BC.b.h); } break; //RL B
    case 0x111: {RL(BC.b.l); } break; //RL C
    case 0x112: {RL(DE.b.h); } break; //RL D
    case 0x113: {RL(DE.b.l); } break; //RL E
    case 0x114: {RL(HL.b.h); } break; //RL H
    case 0x115: {RL(HL.b.l); } break; //RL L
    case 0x116: {uint8_t d = gb->read_byte(HL.w); RL(d); gb->write_byte(HL.w, d); } break; //RL (HL)
    case 0x117: {RL(AF.b.h); } break; //RL A
    case 0x118: {RR(BC.b.h); } break; //RR B
    case 0x119: {RR(BC.b.l); } break; //RR C
    case 0x11A: {RR(DE.b.h); } break; //RR D
    case 0x11B: {RR(DE.b.l); } break; //RR E
    case 0x11C: {RR(HL.b.h); } break; //RR H
    case 0x11D: {RR(HL.b.l); } break; //RR L
    case 0x11E: {uint8_t d = gb->read_byte(HL.w); RR(d); gb->write_byte(HL.w, d); } break; //RR (HL)
    case 0x11F: {RR(AF.b.h); } break; //RR A
    case 0x120: {SLA(BC.b.h); } break; //SLA B
    case 0x121: {SLA(BC.b.l); } break; //SLA C
    case 0x122: {SLA(DE.b.h); } break; //SLA D
    case 0x123: {SLA(DE.b.l); } break; //SLA E
    case 0x124: {SLA(HL.b.h); } break; //SLA H
    case 0x125: {SLA(HL.b.l); } break; //SLA L
    case 0x126: {uint8_t d = gb->read_byte(HL.w); SLA(d); gb->write_byte(HL.w, d); } break; //SLA (HL)
    case 0x127: {SLA(AF.b.h); } break; //SLA A
    case 0x128: {SRA(BC.b.h); } break; //SRA B
    case 0x129: {SRA(BC.b.l); } break; //SRA C
    case 0x12A: {SRA(DE.b.h); } break; //SRA D
    case 0x12B: {SRA(DE.b.l); } break; //SRA E
    case 0x12C: {SRA(HL.b.h); } break; //SRA H
    case 0x12D: {SRA(HL.b.l); } break; //SRA L
    case 0x12E: {uint8_t d = gb->read_byte(HL.w); SRA(d); gb->write_byte(HL.w, d); } break; //SRA (HL)
    case 0x12F: {SRA(AF.b.h); } break; //SRA A
    case 0x130: {SWAP(BC.b.h); } break; //SWAP B
    case 0x131: {SWAP(BC.b.l); } break; //SWAP C
    case 0x132: {SWAP(DE.b.h); } break; //SWAP D
    case 0x133: {SWAP(DE.b.l); } break; //SWAP E
    case 0x134: {SWAP(HL.b.h); } break; //SWAP H
    case 0x135: {SWAP(HL.b.l); } break; //SWAP L
    case 0x136: {uint8_t d = gb->read_byte(HL.w); SWAP(d); gb->write_byte(HL.w, d); } break; //SWAP (HL)
    case 0x137: {SWAP(AF.b.h); } break; //SWAP A
    case 0x138: {SRL(BC.b.h); } break; //SRL B
    case 0x139: {SRL(BC.b.l); } break; //SRL C
    case 0x13A: {SRL(DE.b.h); } break; //SRL D
    case 0x13B: {SRL(DE.b.l); } break; //SRL E
    case 0x13C: {SRL(HL.b.h); } break; //SRL H
    case 0x13D: {SRL(HL.b.l); } break; //SRL L
    case 0x13E: {uint8_t d = gb->read_byte(HL.w);  SRL(d); gb->write_byte(HL.w, d); } break; //SRL (HL)
    case 0x13F: {SRL(AF.b.h); } break; //SRL A
    case 0x140: {BIT(0, BC.b.h); } break; //BIT 0, B
    case 0x141: {BIT(0, BC.b.l); } break; //BIT 0, C
    case 0x142: {BIT(0, DE.b.h); } break; //BIT 0, D
    case 0x143: {BIT(0, DE.b.l); } break; //BIT 0, E
    case 0x144: {BIT(0, HL.b.h); } break; //BIT 0, H
    case 0x145: {BIT(0, HL.b.l); } break; //BIT 0, L
    case 0x146: {BIT(0, gb->read_byte(HL.w)); } break; //BIT 0, (HL)
    case 0x147: {BIT(0, AF.b.h); } break; //BIT 0, A
    case 0x148: {BIT(1, BC.b.h); } break; //BIT 1, B
    case 0x149: {BIT(1, BC.b.l); } break; //BIT 1, C
    case 0x14A: {BIT(1, DE.b.h); } break; //BIT 1, D
    case 0x14B: {BIT(1, DE.b.l); } break; //BIT 1, E
    case 0x14C: {BIT(1, HL.b.h); } break; //BIT 1, H
    case 0x14D: {BIT(1, HL.b.l); } break; //BIT 1, L
    case 0x14E: {BIT(1, gb->read_byte(HL.w)); } break; //BIT 1, (HL)
    case 0x14F: {BIT(1, AF.b.h); } break; //BIT 1, A
    case 0x150: {BIT(2, BC.b.h); } break; //BIT 2, B
    case 0x151: {BIT(2, BC.b.l); } break; //BIT 2, C
    case 0x152: {BIT(2, DE.b.h); } break; //BIT 2, D
    case 0x153: {BIT(2, DE.b.l); } break; //BIT 2, E
    case 0x154: {BIT(2, HL.b.h); } break; //BIT 2, H
    case 0x155: {BIT(2, HL.b.l); } break; //BIT 2, L
    case 0x156: {BIT(2, gb->read_byte(HL.w)); } break; //BIT 2, (HL)
    case 0x157: {BIT(2, AF.b.h); } break; //BIT 2, A
    case 0x158: {BIT(3, BC.b.h); } break; //BIT 3, B
    case 0x159: {BIT(3, BC.b.l); } break; //BIT 3, C
    case 0x15A: {BIT(3, DE.b.h); } break; //BIT 3, D
    case 0x15B: {BIT(3, DE.b.l); } break; //BIT 3, E
    case 0x15C: {BIT(3, HL.b.h); } break; //BIT 3, H
    case 0x15D: {BIT(3, HL.b.l); } break; //BIT 3, L
    case 0x15E: {BIT(3, gb->read_byte(HL.w)); } break; //BIT 3, (HL)
    case 0x15F: {BIT(3, AF.b.h); } break; //BIT 3, A
    case 0x160: {BIT(4, BC.b.h); } break; //BIT 4, B
    case 0x161: {BIT(4, BC.b.l); } break; //BIT 4, C
    case 0x162: {BIT(4, DE.b.h); } break; //BIT 4, D
    case 0x163: {BIT(4, DE.b.l); } break; //BIT 4, E
    case 0x164: {BIT(4, HL.b.h); } break; //BIT 4, H
    case 0x165: {BIT(4, HL.b.l); } break; //BIT 4, L
    case 0x166: {BIT(4, gb->read_byte(HL.w)); } break; //BIT 4, (HL)
    case 0x167: {BIT(4, AF.b.h); } break; //BIT 4, A
    case 0x168: {BIT(5, BC.b.h); } break; //BIT 5, B
    case 0x169: {BIT(5, BC.b.l); } break; //BIT 5, C
    case 0x16A: {BIT(5, DE.b.h); } break; //BIT 5, D
    case 0x16B: {BIT(5, DE.b.l); } break; //BIT 5, E
    case 0x16C: {BIT(5, HL.b.h); } break; //BIT 5, H
    case 0x16D: {BIT(5, HL.b.l); } break; //BIT 5, L
    case 0x16E: {BIT(5, gb->read_byte(HL.w)); } break; //BIT 5, (HL)
    case 0x16F: {BIT(5, AF.b.h); } break; //BIT 5, A
    case 0x170: {BIT(6, BC.b.h); } break; //BIT 6, B
    case 0x171: {BIT(6, BC.b.l); } break; //BIT 6, C
    case 0x172: {BIT(6, DE.b.h); } break; //BIT 6, D
    case 0x173: {BIT(6, DE.b.l); } break; //BIT 6, E
    case 0x174: {BIT(6, HL.b.h); } break; //BIT 6, H
    case 0x175: {BIT(6, HL.b.l); } break; //BIT 6, L
    case 0x176: {BIT(6, gb->read_byte(HL.w)); } break; //BIT 6, (HL)
    case 0x177: {BIT(6, AF.b.h); } break; //BIT 6, A
    case 0x178: {BIT(7, BC.b.h); } break; //BIT 7, B
    case 0x179: {BIT(7, BC.b.l); } break; //BIT 7, C
    case 0x17A: {BIT(7, DE.b.h); } break; //BIT 7, D
    case 0x17B: {BIT(7, DE.b.l); } break; //BIT 7, E
    case 0x17C: {BIT(7, HL.b.h); } break; //BIT 7, H
    case 0x17D: {BIT(7, HL.b.l); } break; //BIT 7, L
    case 0x17E: {BIT(7, gb->read_byte(HL.w)); } break; //BIT 7, (HL)
    case 0x17F: {BIT(7, AF.b.h); } break; //BIT 7, A
    case 0x180: {RES(0, BC.b.h); } break; //RES 0, B
    case 0x181: {RES(0, BC.b.l); } break; //RES 0, C
    case 0x182: {RES(0, DE.b.h); } break; //RES 0, D
    case 0x183: {RES(0, DE.b.l); } break; //RES 0, E
    case 0x184: {RES(0, HL.b.h); } break; //RES 0, H
    case 0x185: {RES(0, HL.b.l); } break; //RES 0, L
    case 0x186: {uint8_t t = gb->read_byte(HL.w);  RES(0, t); gb->write_byte(HL.w, t); } break; //RES 0, (HL)
    case 0x187: {RES(0, AF.b.h); } break; //RES 0, A
    case 0x188: {RES(1, BC.b.h); } break; //RES 1, B
    case 0x189: {RES(1, BC.b.l); } break; //RES 1, C
    case 0x18A: {RES(1, DE.b.h); } break; //RES 1, D
    case 0x18B: {RES(1, DE.b.l); } break; //RES 1, E
    case 0x18C: {RES(1, HL.b.h); } break; //RES 1, H
    case 0x18D: {RES(1, HL.b.l); } break; //RES 1, L
    case 0x18E: {uint8_t t = gb->read_byte(HL.w);  RES(1, t); gb->write_byte(HL.w, t); } break; //RES 1, (HL)
    case 0x18F: {RES(1, AF.b.h); } break; //RES 1, A
    case 0x190: {RES(2, BC.b.h); } break; //RES 2, B
    case 0x191: {RES(2, BC.b.l); } break; //RES 2, C
    case 0x192: {RES(2, DE.b.h); } break; //RES 2, D
    case 0x193: {RES(2, DE.b.l); } break; //RES 2, E
    case 0x194: {RES(2, HL.b.h); } break; //RES 2, H
    case 0x195: {RES(2, HL.b.l); } break; //RES 2, L
    case 0x196: {uint8_t t = gb->read_byte(HL.w);  RES(2, t); gb->write_byte(HL.w, t); } break; //RES 2, (HL)
    case 0x197: {RES(2, AF.b.h); } break; //RES 2, A
    case 0x198: {RES(3, BC.b.h); } break; //RES 3, B
    case 0x199: {RES(3, BC.b.l); } break; //RES 3, C
    case 0x19A: {RES(3, DE.b.h); } break; //RES 3, D
    case 0x19B: {RES(3, DE.b.l); } break; //RES 3, E
    case 0x19C: {RES(3, HL.b.h); } break; //RES 3, H
    case 0x19D: {RES(3, HL.b.l); } break; //RES 3, L
    case 0x19E: {uint8_t t = gb->read_byte(HL.w);  RES(3, t); gb->write_byte(HL.w, t); } break; //RES 3, (HL)
    case 0x19F: {RES(3, AF.b.h); } break; //RES 3, A
    case 0x1A0: {RES(4, BC.b.h); } break; //RES 4, B
    case 0x1A1: {RES(4, BC.b.l); } break; //RES 4, C
    case 0x1A2: {RES(4, DE.b.h); } break; //RES 4, D
    case 0x1A3: {RES(4, DE.b.l); } break; //RES 4, E
    case 0x1A4: {RES(4, HL.b.h); } break; //RES 4, H
    case 0x1A5: {RES(4, HL.b.l); } break; //RES 4, L
    case 0x1A6: {uint8_t t = gb->read_byte(HL.w);  RES(4, t); gb->write_byte(HL.w, t); } break; //RES 4, (HL)
    case 0x1A7: {RES(4, AF.b.h); } break; //RES 4, A
    case 0x1A8: {RES(5, BC.b.h); } break; //RES 5, B
    case 0x1A9: {RES(5, BC.b.l); } break; //RES 5, C
    case 0x1AA: {RES(5, DE.b.h); } break; //RES 5, D
    case 0x1AB: {RES(5, DE.b.l); } break; //RES 5, E
    case 0x1AC: {RES(5, HL.b.h); } break; //RES 5, H
    case 0x1AD: {RES(5, HL.b.l); } break; //RES 5, L
    case 0x1AE: {uint8_t t = gb->read_byte(HL.w);  RES(5, t); gb->write_byte(HL.w, t); } break; //RES 5, (HL)
    case 0x1AF: {RES(5, AF.b.h); } break; //RES 5, A
    case 0x1B0: {RES(6, BC.b.h); } break; //RES 6, B
    case 0x1B1: {RES(6, BC.b.l); } break; //RES 6, C
    case 0x1B2: {RES(6, DE.b.h); } break; //RES 6, D
    case 0x1B3: {RES(6, DE.b.l); } break; //RES 6, E
    case 0x1B4: {RES(6, HL.b.h); } break; //RES 6, H
    case 0x1B5: {RES(6, HL.b.l); } break; //RES 6, L
    case 0x1B6: {uint8_t t = gb->read_byte(HL.w);  RES(6, t); gb->write_byte(HL.w, t); } break; //RES 6, (HL)
    case 0x1B7: {RES(6, AF.b.h); } break; //RES 6, A
    case 0x1B8: {RES(7, BC.b.h); } break; //RES 7, B
    case 0x1B9: {RES(7, BC.b.l); } break; //RES 7, C
    case 0x1BA: {RES(7, DE.b.h); } break; //RES 7, D
    case 0x1BB: {RES(7, DE.b.l); } break; //RES 7, E
    case 0x1BC: {RES(7, HL.b.h); } break; //RES 7, H
    case 0x1BD: {RES(7, HL.b.l); } break; //RES 7, L
    case 0x1BE: {uint8_t t = gb->read_byte(HL.w);  RES(7, t); gb->write_byte(HL.w, t); } break; //RES 7, (HL)
    case 0x1BF: {RES(7, AF.b.h); } break; //RES 7, A
    case 0x1C0: {SET(0, BC.b.h); } break; //SET 0, B
    case 0x1C1: {SET(0, BC.b.l); } break; //SET 0, C
    case 0x1C2: {SET(0, DE.b.h); } break; //SET 0, D
    case 0x1C3: {SET(0, DE.b.l); } break; //SET 0, E
    case 0x1C4: {SET(0, HL.b.h); } break; //SET 0, H
    case 0x1C5: {SET(0, HL.b.l); } break; //SET 0, L
    case 0x1C6: {uint8_t t = gb->read_byte(HL.w);  SET(0, t); gb->write_byte(HL.w, t); } break; //SET 0, (HL)
    case 0x1C7: {SET(0, AF.b.h); } break; //SET 0, A
    case 0x1C8: {SET(1, BC.b.h); } break; //SET 1, B
    case 0x1C9: {SET(1, BC.b.l); } break; //SET 1, C
    case 0x1CA: {SET(1, DE.b.h); } break; //SET 1, D
    case 0x1CB: {SET(1, DE.b.l); } break; //SET 1, E
    case 0x1CC: {SET(1, HL.b.h); } break; //SET 1, H
    case 0x1CD: {SET(1, HL.b.l); } break; //SET 1, L
    case 0x1CE: {uint8_t t = gb->read_byte(HL.w);  SET(1, t); gb->write_byte(HL.w, t); } break; //SET 1, (HL)
    case 0x1CF: {SET(1, AF.b.h); } break; //SET 1, A
    case 0x1D0: {SET(2, BC.b.h); } break; //SET 2, B
    case 0x1D1: {SET(2, BC.b.l); } break; //SET 2, C
    case 0x1D2: {SET(2, DE.b.h); } break; //SET 2, D
    case 0x1D3: {SET(2, DE.b.l); } break; //SET 2, E
    case 0x1D4: {SET(2, HL.b.h); } break; //SET 2, H
    case 0x1D5: {SET(2, HL.b.l); } break; //SET 2, L
    case 0x1D6: {uint8_t t = gb->read_byte(HL.w);  SET(2, t); gb->write_byte(HL.w, t); } break; //SET 2, (HL)
    case 0x1D7: {SET(2, AF.b.h); } break; //SET 2, A
    case 0x1D8: {SET(3, BC.b.h); } break; //SET 3, B
    case 0x1D9: {SET(3, BC.b.l); } break; //SET 3, C
    case 0x1DA: {SET(3, DE.b.h); } break; //SET 3, D
    case 0x1DB: {SET(3, DE.b.l); } break; //SET 3, E
    case 0x1DC: {SET(3, HL.b.h); } break; //SET 3, H
    case 0x1DD: {SET(3, HL.b.l); } break; //SET 3, L
    case 0x1DE: {uint8_t t = gb->read_byte(HL.w);  SET(3, t); gb->write_byte(HL.w, t); } break; //SET 3, (HL)
    case 0x1DF: {SET(3, AF.b.h); } break; //SET 3, A
    case 0x1E0: {SET(4, BC.b.h); } break; //SET 4, B
    case 0x1E1: {SET(4, BC.b.l); } break; //SET 4, C
    case 0x1E2: {SET(4, DE.b.h); } break; //SET 4, D
    case 0x1E3: {SET(4, DE.b.l); } break; //SET 4, E
    case 0x1E4: {SET(4, HL.b.h); } break; //SET 4, H
    case 0x1E5: {SET(4, HL.b.l); } break; //SET 4, L
    case 0x1E6: {uint8_t t = gb->read_byte(HL.w);  SET(4, t); gb->write_byte(HL.w, t); } break; //SET 4, (HL)
    case 0x1E7: {SET(4, AF.b.h); } break; //SET 4, A
    case 0x1E8: {SET(5, BC.b.h); } break; //SET 5, B
    case 0x1E9: {SET(5, BC.b.l); } break; //SET 5, C
    case 0x1EA: {SET(5, DE.b.h); } break; //SET 5, D
    case 0x1EB: {SET(5, DE.b.l); } break; //SET 5, E
    case 0x1EC: {SET(5, HL.b.h); } break; //SET 5, H
    case 0x1ED: {SET(5, HL.b.l); } break; //SET 5, L
    case 0x1EE: {uint8_t t = gb->read_byte(HL.w);  SET(5, t); gb->write_byte(HL.w, t); } break; //SET 5, (HL)
    case 0x1EF: {SET(5, AF.b.h); } break; //SET 5, A
    case 0x1F0: {SET(6, BC.b.h); } break; //SET 6, B
    case 0x1F1: {SET(6, BC.b.l); } break; //SET 6, C
    case 0x1F2: {SET(6, DE.b.h); } break; //SET 6, D
    case 0x1F3: {SET(6, DE.b.l); } break; //SET 6, E
    case 0x1F4: {SET(6, HL.b.h); } break; //SET 6, H
    case 0x1F5: {SET(6, HL.b.l); } break; //SET 6, L
    case 0x1F6: {uint8_t t = gb->read_byte(HL.w);  SET(6, t); gb->write_byte(HL.w, t); } break; //SET 6, (HL)
    case 0x1F7: {SET(6, AF.b.h); } break; //SET 6, A
    case 0x1F8: {SET(7, BC.b.h); } break; //SET 7, B
    case 0x1F9: {SET(7, BC.b.l); } break; //SET 7, C
    case 0x1FA: {SET(7, DE.b.h); } break; //SET 7, D
    case 0x1FB: {SET(7, DE.b.l); } break; //SET 7, E
    case 0x1FC: {SET(7, HL.b.h); } break; //SET 7, H
    case 0x1FD: {SET(7, HL.b.l); } break; //SET 7, L
    case 0x1FE: {uint8_t t = gb->read_byte(HL.w);  SET(7, t); gb->write_byte(HL.w, t); } break; //SET 7, (HL)
    case 0x1FF: {SET(7, AF.b.h); } break; //SET 7, A
    case 0x200: {IME = 0; CALL(0x40); } break; //INTERRUPT 40
    case 0x201: {IME = 0; CALL(0x48); } break; //INTERRUPT 48
    case 0x202: {IME = 0; CALL(0x50); } break; //INTERRUPT 50
    case 0x203: {IME = 0; CALL(0x58); } break; //INTERRUPT 58
    case 0x204: {IME = 0; CALL(0x60); } break; //INTERRUPT 60
    default:
    { int x = 1; }
        break;
    }
    // clang-format on
}

void c_sm83::ADC8(uint8_t d)
{
    ADD8(d, fc ? 1 : 0);
}

void c_sm83::SBC8(uint8_t d)
{
    SUB8(d, fc ? 1 : 0);
}

void c_sm83::ADD8(uint8_t d, int carry)
{
    int result = AF.b.h + d + carry;
    fn = 0;
    fc = result > 255 ? 1 : 0;
    fh = (AF.b.h & 0xF) + (d & 0xF) + (carry) > 0xF ? 1 : 0;
    AF.b.h = result & 0xFF;
    fz = AF.b.h == 0 ? 1 : 0;
}

void c_sm83::SUB8(uint8_t d, int carry)
{
    int c = d + carry;
    int result = (int)AF.b.h - (int)c;
    unsigned int uresult = (unsigned int)result;
    fc = result < 0 ? 1 : 0;
    fn = 1;
    fh = (AF.b.h & 0xF) - (d & 0xF) - (carry & 0xF) < 0 ? 1 : 0;
    AF.b.h = result & 0xFF;
    fz = AF.b.h == 0 ? 1 : 0;
}

void c_sm83::CP(uint8_t d)
{
    int c = d;
    int result = (int)AF.b.h - (int)c;
    unsigned int uresult = (unsigned int)result;
    fc = result < 0 ? 1 : 0;
    fn = 1;
    fz = (result & 0xFF) == 0 ? 1 : 0;
    fh = (AF.b.h & 0xF) - (c & 0xF) < 0 ? 1 : 0;
}

void c_sm83::DEC(uint8_t &reg)
{
    fh = (reg & 0xF) == 0 ? 1 : 0;
    reg--;
    fz = reg == 0 ? 1 : 0;
    fn = 1;
}

void c_sm83::INC(uint8_t &reg)
{
    fh = (reg & 0xF) == 0xF ? 1 : 0;
    reg++;
    fz = reg == 0 ? 1 : 0;
    fn = 0;
}

void c_sm83::LD(uint8_t &reg, uint8_t d)
{
    reg = d;
}

void c_sm83::LD(uint16_t &reg, uint16_t d)
{
    reg = d;
}

void c_sm83::LD(uint16_t address, uint8_t d)
{
    gb->write_byte(address, d);
}

void c_sm83::AND(uint8_t d)
{
    AF.b.h &= d;
    fn = 0;
    fh = 1;
    fc = 0;
    fz = AF.b.h == 0 ? 1 : 0;
}

void c_sm83::OR(uint8_t d)
{
    AF.b.h |= d;
    fz = AF.b.h == 0 ? 1 : 0;
    fn = 0;
    fh = 0;
    fc = 0;
}
void c_sm83::XOR(uint8_t d)
{
    AF.b.h ^= d;
    fz = AF.b.h == 0 ? 1 : 0;
    fn = 0;
    fh = 0;
    fc = 0;
}

void c_sm83::push_word(uint16_t d)
{
    SP -= 2;
    gb->write_word(SP, d);
}

uint16_t c_sm83::pop_word()
{
    uint16_t ret = gb->read_word(SP);
    SP += 2;
    return ret;
}

void c_sm83::update_f()
{
    int F = 0;
    //znhc
    if (fz)
        F |= 0x80;
    if (fn)
        F |= 0x40;
    if (fh)
        F |= 0x20;
    if (fc)
        F |= 0x10;
    AF.b.l = F;
}

void c_sm83::update_flags()
{
    fz = fn = fh = fc = 0;
    int F = AF.b.l;
    if (F & 0x80)
        fz = 1;
    if (F & 0x40)
        fn = 1;
    if (F & 0x20)
        fh = 1;
    if (F & 0x10)
        fc = 1;
}

void c_sm83::CALL(uint16_t addr)
{
    push_word(PC);
    PC = addr;
}

void c_sm83::SRL(uint8_t &reg)
{
    fc = reg & 0x1;
    fh = 0;
    fn = 0;
    reg >>= 1;
    fz = reg == 0 ? 1 : 0;
}

void c_sm83::RR(uint8_t &reg)
{
    int c = fc ? 0x80 : 0;
    fc = reg & 0x1;
    reg >>= 1;
    reg |= c;
    fh = 0;
    fn = 0;
    fz = reg == 0 ? 1 : 0;
}

void c_sm83::RL(uint8_t &reg)
{
    int c = fc ? 0x1 : 0;
    fc = reg & 0x80;
    reg <<= 1;
    reg |= c;
    fh = 0;
    fn = 0;
    fz = reg == 0 ? 1 : 0;
}

void c_sm83::RLC(uint8_t &reg)
{
    fc = reg & 0x80 ? 1 : 0;
    reg <<= 1;
    reg |= fc;
    fz = reg == 0 ? 1 : 0;
    fn = fh = 0;
}

void c_sm83::RRC(uint8_t &reg)
{
    fc = reg & 0x01 ? 0x80 : 0;
    reg >>= 1;
    reg |= fc;
    fz = reg == 0 ? 1 : 0;
    fn = fh = 0;
}

void c_sm83::SLA(uint8_t &reg)
{
    fc = reg & 0x80;
    reg <<= 1;
    fz = reg == 0 ? 1 : 0;
    fn = fh = 0;
}

void c_sm83::SRA(uint8_t &reg)
{
    int t = reg & 0x80;
    fc = reg & 0x1;
    reg >>= 1;
    reg |= t;
    fz = reg == 0 ? 1 : 0;
    fn = fh = 0;
}

void c_sm83::ADD16(uint16_t &reg, uint16_t d)
{
    fh = (reg & 0xFFF) + (d & 0xFFF) > 0xFFF ? 1 : 0;
    int result = reg + d;
    fc = result > 0xFFFF;
    fn = 0;
    reg = result & 0xFFFF;
}

void c_sm83::ADD16r(uint16_t &reg, int8_t d)
{
    uint8_t a = reg & 0xFF;
    uint8_t b = d;
    fh = (a & 0xF) + (b & 0xF) > 0xF ? 1 : 0;
    int c = a + b;
    fc = c > 0xFF;

    int result = reg + d;
    fn = 0;
    fz = 0;
    reg = reg + d;
}

void c_sm83::STOP()
{
    gb->ppu->on_stop();
}

void c_sm83::SWAP(uint8_t &reg)
{
    reg = ((reg & 0xF0) >> 4) | ((reg & 0x0F) << 4);
    fz = reg == 0 ? 1 : 0;
    fn = fh = fc = 0;
}

void c_sm83::DAA()
{
    if (fn) {
        if (fc) {
            AF.b.h -= 0x60;
            fc = 1;
        }
        if (fh) {
            AF.b.h -= 0x6;
        }
    }
    else {
        if (fc || AF.b.h > 0x99) {
            AF.b.h += 0x60;
            fc = 1;
        }
        if (fh || (AF.b.h & 0xF) > 0x9) {
            AF.b.h += 0x6;
        }
    }
    fz = AF.b.h == 0 ? 1 : 0;
    fh = 0;
}

void c_sm83::BIT(int bit, uint8_t d)
{
    fz = (d & (1 << bit)) == 0 ? 1 : 0;
    fn = 0;
    fh = 1;
}

void c_sm83::RES(int bit, uint8_t &d)
{
    d &= ~(1 << bit);
}

void c_sm83::SET(int bit, uint8_t &d)
{
    d |= (1 << bit);
}

} //namespace gb