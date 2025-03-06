module;

module nes:joypad;

namespace nes
{

int c_joypad::reset()
{
    strobe = 0;
    joy1 = 0;
    joy2 = 0;
    joy3 = 0;
    joy4 = 0;
    read1 = 0;
    read2 = 0;
    return 1;
}

unsigned char c_joypad::read_byte(unsigned short address)
{
    unsigned char x = 0;
    switch (address) {
        case 0x4016: {
            if (strobe)
                read1 = 0;
            if (read1 < 8)
                x = (joy1 >> (read1 & 0x07)) & 0x01;
            else if (read1 < 16)
                x = (joy3 >> ((read1 - 8) & 0x07)) & 0x01;

            if (read1 < 20)
                read1++;
            return x | 0x40;
        }
        case 0x4017: {
            if (strobe)
                read2 = 0;
            if (read2 < 8)
                x = (joy2 >> (read2 & 0x07)) & 0x01;
            else if (read2 < 16)
                x = (joy4 >> ((read2 - 8) & 0x07)) & 0x01;

            if (read2 < 20)
                read2++;
            return x | 0x40;
        }
        default:
            return 0;
    }
}

void c_joypad::write_byte(unsigned short address, unsigned char value)
{
    switch (address) {
        case 0x4016:

            strobe = value & 0x01;
            if (strobe) {
                read1 = 0;
                read2 = 0;
            }
            break;

        default:
            break;
    }
}

} //namespace nes