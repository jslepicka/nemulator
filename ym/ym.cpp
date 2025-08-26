module;

module ym;

c_ym::c_ym()
{
}

c_ym::~c_ym()
{
}

void c_ym::reset()
{
    out = 0.0f;
    ticks = 0;
    group = GROUP_ONE;
}

void c_ym::clock(int cycles)
{
    if (++ticks == 24) {
        ticks = 0;
        if (registers[0x2B] & 0x80) {
            //dac enabled
            int dac_sample = (int)(registers[0x2A]) - 128;
            dac_sample <<= 6;
            float o = (float)dac_sample / (float)(1 << 13);
            out = o / 6.0;
        }
    }
}

void c_ym::write(uint16_t address, uint8_t value)
{
    int x = 1;
    switch (address) {
        case 0:
        case 2:
            group = address == 0 ? GROUP_ONE : GROUP_TWO;
            addr = value;
            channel_idx = addr & 0x3;
            if (group == GROUP_TWO) {
                channel_idx += 3;
            }
            operator_idx = ((addr >> 3) & 0x1) | ((addr >> 1) & 2);
            break;
        case 1:
        case 3:
            data = value;
            registers[addr] = value;
            break;
    }
}