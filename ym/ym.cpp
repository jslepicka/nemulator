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
    timer_a = 0;
    timer_a_reload = 0;
    timer_a_enabled = 0;
    status = 0;
    busy_counter = 0;
}

void c_ym::clock(int cycles)
{
    if (busy_counter) {
        busy_counter--;
        if (!busy_counter) {
            status &= ~0x80;
        }
    }
    if (++ticks == 24) {
        ticks = 0;
        //clock timers
        if (timer_a_enabled) {
            timer_a++;
            if (timer_a == 0x400) {
                if (registers[0x27] & 0x4) {
                    status |= 1;
                }
                timer_a = timer_a_reload;
            }
        }


        if (registers[0x2B] & 0x80) {
            //dac enabled
            int dac_sample = (int)(registers[0x2A]) - 128;
            dac_sample <<= 6;
            float o = (float)dac_sample / (float)(1 << 13);
            out = o / 6.0;
        }
    }
}

uint8_t c_ym::read(uint16_t address)
{
    //if (address != 0x4000) {
    //    int x = 1;
    //    return 0;
    //}
    return status;
}

void c_ym::write(uint16_t address, uint8_t value)
{
    switch (address) {
        case 0:
        case 2:
            group = address == 0 ? GROUP_ONE : GROUP_TWO;
            addr = value;
            if (value == 0x1) {
                int x = 1;
            }
            channel_idx = addr & 0x3;
            if (group == GROUP_TWO) {
                channel_idx += 3;
            }
            operator_idx = ((addr >> 3) & 0x1) | ((addr >> 1) & 2);
            break;
        case 1:
        case 3:
            if (group == GROUP_TWO) {
                int x = 1;
            }
            data = value;
            registers[addr] = value;
            if (addr == 0x26) {
                int x = 1;
            }
            busy_counter = 32;
            status |= 0x80;
            switch (addr) {
                case 0x24:
                    timer_a_reload = (timer_a_reload & 3) | (value << 2);
                    break;
                case 0x25:
                    //should timer_a be reset here?
                    timer_a_reload = (timer_a_reload & ~3) | (value & 0x3);
                    break;
                case 0x27:
                    if (value & 0x10) {
                        status &= ~1;
                    }
                    else if (value & 0x20) {
                        status &= ~2;
                    }
                    if (!timer_a_enabled && (value & 0x1)) {
                        timer_a_enabled = 1;
                        timer_a = timer_a_reload;
                    }
                    else if (!(value & 0x1)) {
                        timer_a_enabled = 0;
                    }
                    break;
                case 0x2A: {
                    int x = 1;
                } break;
            }
            break;
    }
}