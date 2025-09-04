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

    timer_b = 0;
    timer_b_reload = 0;
    timer_b_enabled = 0;

    status = 0;
    busy_counter = 0;
    timer_b_divider = 0;

    std::memset(freq_hi, 0, sizeof(freq_hi));
}

void c_ym::clock_timer_a()
{
    if (timer_a_enabled) {
        timer_a++;
        if (timer_a == 0x400) {
            if (registers[0x27] & 0x4) {
                status |= 1;
            }
            timer_a = timer_a_reload;
        }
    }
}

void c_ym::clock_timer_b()
{
    if (timer_b_enabled) {
        timer_b++;
        if (timer_b == 0x100) {
            if (registers[0x27] & 0x8) {
                status |= 2;
            }
            timer_b = timer_b_reload;
        }
    }
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
        clock_timer_a();
        if (++timer_b_divider == 16) {
            timer_b_divider = 0;
            clock_timer_b();
        }

        clock_channels();

        out = mix();
    }
}

float c_ym::mix()
{
    float out = 0.0f;
    for (int i = 0; i < 6; i++) {
        if (i == 5 && (registers[0x2B] & 0x80)) {
            out += get_dac_sample();
        }
        else {
            out += .3 * channels[i].output();
        }
    }
    out /= 6.0f;
    return out;
}

float c_ym::get_dac_sample()
{
    int dac_sample = (int)(registers[0x2A]) - 128;
    dac_sample <<= 6;
    float o = (float)dac_sample / (float)(1 << 13);
    return o;
}

void c_ym::clock_channels()
{
    for (auto &channel : channels) {
        channel.clock();
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
            reg_addr = value;
            if (value == 0x1) {
                int x = 1;
            }
            channel_idx = reg_addr & 0x3;
            if (channel_idx == 3) {
                int x = 1;
            }
            if (group == GROUP_TWO) {
                channel_idx += 3;
            }
            operator_idx = ((reg_addr & 0x8) >> 3) | ((reg_addr & 0x4) >> 1);
            break;
        case 1:
        case 3:
            if (group == GROUP_TWO) {
                int x = 1;
            }
            data = value;
            registers[reg_addr] = value;
            if (reg_addr == 0x26) {
                int x = 1;
            }
            busy_counter = 32;
            status |= 0x80;
            switch (reg_addr) {
                case 0x24:
                    timer_a_reload = (timer_a_reload & 3) | (value << 2);
                    break;
                case 0x25:
                    //should timer_a be reset here?
                    timer_a_reload = (timer_a_reload & ~3) | (value & 0x3);
                    break;
                case 0x26:
                    timer_b_reload = value;
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
                    if (!timer_b_enabled && (value & 0x2)) {
                        timer_b_enabled = 1;
                        timer_b = timer_a_reload;
                    }
                    else if (!(value & 0x2)) {
                        timer_b_enabled = 0;
                    }
                    break;
                case 0x28: {
                    int c = value & 0x7;
                    if (c != 3 && c != 7) {
                        if (c >= 4) {
                            c -= 1;
                        }
                        value >>= 4;
                        for (int o = 0; o < 4; o++) {
                            channels[c].operators[o].key(value & 0x1);
                            value >>= 1;
                        }
                    }
                }
                    break;
                case 0x2A: {
                    int x = 1;
                    break;
                } break;
                case 0x30:
                case 0x31:
                case 0x32:
                case 0x33:
                case 0x34:
                case 0x35:
                case 0x36:
                case 0x37:
                case 0x38:
                case 0x39:
                case 0x3A:
                case 0x3B:
                case 0x3C:
                case 0x3D:
                case 0x3E:
                case 0x3F:
                    if (!((reg_addr & 0x3) == 3)) {
                        channels[channel_idx].operators[operator_idx].phase_generator.multiple = value & 0xF;
                    }
                    break;
                case 0xA0:
                case 0xA1:
                case 0xA2: {
                    if (channel_idx == 3) {
                        int x = 1;
                    }
                    uint32_t f = (((uint32_t)freq_hi[channel_idx] & 7) << 8) | value;
                    for (int o = 0; o < 4; o++) {
                        channels[channel_idx].operators[o].phase_generator.f_number = f;
                        channels[channel_idx].operators[o].phase_generator.block = (value >> 3) & 7;
                    }
                }
                    break;
                case 0xA4:
                case 0xA5:
                case 0xA6: {
                    freq_hi[channel_idx] = value;
                }
                    break;
            }
            break;
    }
}

c_phase_generator::c_phase_generator()
{
    reset();
}

void c_phase_generator::reset()
{
    counter = 0;
    f_number = 0;
    block = 0;
    detune = 0;
    multiple = 0;
}

void c_phase_generator::clock()
{
    uint32_t increment = (f_number << block) >> 1;
    //todo: detune
    increment = increment & ((1 << 17) - 1);

    if (multiple) {
        increment *= multiple;
    }
    else {
        increment >>= 1;
    }

    counter = (counter + increment) & ((1 << 20) - 1);
}

uint32_t c_phase_generator::output()
{
    return counter >> 10;
}

void c_phase_generator::reset_counter()
{
    counter = 0;
}

c_fm_operator::c_fm_operator()
{
    reset();
}

void c_fm_operator::reset()
{
    key_on = false;
}

void c_fm_operator::key(bool on)
{
    if (on && !key_on) {
        phase_generator.reset_counter();
        key_on = true;
    }
    else {
        key_on = on;
    }
}

c_fm_channel::c_fm_channel()
{
    reset();
}

void c_fm_channel::reset()
{
    out = 0.0f;
}

void c_fm_channel::clock()
{
    c_fm_operator &carrier = operators[3];
    if (!carrier.key_on) {
        out = 0.0f;
        return;
    }
    carrier.phase_generator.clock();
    uint32_t phase = carrier.phase_generator.output();

    float p = ((float)phase / 1024.0f) * 2.0f * (float)std::numbers::pi;
    out = sin(p);
}

float c_fm_channel::output()
{
    return out;
}