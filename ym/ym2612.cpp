module;

module ym2612;

static const std::array<uint32_t, 512> log_sin_table = [] {
    std::array<uint32_t, 512> ret;
    for (int x = 0; x < 512; x++) {
        int i = x;
        if ((i >> 8) & 1) {
            i = (~i) & 0xFF;
        }
        float n = ((i << 1) | 1);
        float sine = std::sin((n / 512.0f * std::numbers::pi / 2.0));
        float attenuation = -std::log2f(sine);
        ret[x] = (uint32_t)std::roundf(attenuation * (1 << 8));
    }
    return ret;
}();

static const std::array<uint32_t, 256> pow2_table = [] {
    std::array<uint32_t, 256> ret;
    for (int i = 0; i < 256; i++) {
        float n = (float)(i + 1) / 256.0f;
        float inv_pow2 = std::powf(2.0, -n);
        ret[i] = (uint32_t)std::roundf(inv_pow2 * (1 << 11));
    }
    return ret;
}();

void c_ym2612::reset()
{
    out_l = 0.0f;
    out_r = 0.0f;
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

    //move this to the channel
    std::memset(freq_hi, 0, sizeof(freq_hi));

    for (auto &channel : channels) {
        channel.reset();
    }
}

void c_ym2612::clock_timer_a()
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

void c_ym2612::clock_timer_b()
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

void c_ym2612::clock(int cycles)
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

        mix();
    }
}

void c_ym2612::mix()
{
    out_l = 0.0f;
    out_r = 0.0f;

    for (int i = 0; i < 6; i++) {
        float channel_out = 0.0f;
        if (i == 5 && (registers[0x2B] & 0x80)) {
            channel_out = get_dac_sample();
        }
        else {
            channel_out = channels[i].output();
        }
        if (channels[i].panning & 0x1) {
            out_r += channel_out;
        }
        if (channels[i].panning & 0x2) {
            out_l += channel_out;
        }
    }
    out_l /= 6.0f;
    out_r /= 6.0f;
}

float c_ym2612::get_dac_sample()
{
    int dac_sample = (int)(registers[0x2A]) - 128;
    dac_sample <<= 6;
    float o = (float)dac_sample / (float)(1 << 13);
    return o;
}

void c_ym2612::clock_channels()
{
    for (auto &channel : channels) {
        channel.clock();
    }
}

uint8_t c_ym2612::read(uint16_t address)
{
    //if (address != 0x4000) {
    //    int x = 1;
    //    return 0;
    //}
    return status;
}

void c_ym2612::write(uint16_t address, uint8_t value)
{
    switch (address) {
        case 0:
        case 2:
            group = address == 0 ? GROUP_ONE : GROUP_TWO;
            reg_addr = value;
            channel_idx = reg_addr & 0x3;
            if (group == GROUP_TWO) {
                channel_idx += 3;
            }
            operator_idx = ((reg_addr & 0x8) >> 3) | ((reg_addr & 0x4) >> 1);
            break;
        case 1:
        case 3:
            data = value;
            registers[reg_addr] = value;
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
                        timer_b = timer_b_reload;
                    }
                    else if (!(value & 0x2)) {
                        timer_b_enabled = 0;
                    }
                    channels[2].multi_frequency_operators = ((value >> 6) & 0x3) == 1;
                    channels[2].update_frequency();
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
                        channels[channel_idx].operators[operator_idx].phase_generator.detune = (value >> 4) & 7;
                    }
                    break;
                case 0x40:
                case 0x41:
                case 0x42:
                case 0x43:
                case 0x44:
                case 0x45:
                case 0x46:
                case 0x47:
                case 0x48:
                case 0x49:
                case 0x4A:
                case 0x4B:
                case 0x4C:
                case 0x4D:
                case 0x4E:
                case 0x4F:
                    if (!((reg_addr & 0x3) == 3)) {
                        channels[channel_idx].operators[operator_idx].envelope_generator.total_level = value & 0x7F;
                    }
                    break;
                case 0x50:
                case 0x51:
                case 0x52:
                case 0x53:
                case 0x54:
                case 0x55:
                case 0x56:
                case 0x57:
                case 0x58:
                case 0x59:
                case 0x5A:
                case 0x5B:
                case 0x5C:
                case 0x5D:
                case 0x5E:
                case 0x5F:
                    if (!((reg_addr & 0x3) == 3)) {
                        channels[channel_idx].operators[operator_idx].envelope_generator.attack_rate = value & 0x1F;
                        //channels[channel_idx].operators[operator_idx].envelope_generator.key_scale = (value >> 6) & 0x3;
                        channels[channel_idx].operators[operator_idx].update_key_scale((value >> 6) & 0x3);
                    }
                    break;
                case 0x60:
                case 0x61:
                case 0x62:
                case 0x63:
                case 0x64:
                case 0x65:
                case 0x66:
                case 0x67:
                case 0x68:
                case 0x69:
                case 0x6A:
                case 0x6B:
                case 0x6C:
                case 0x6D:
                case 0x6E:
                case 0x6F:
                    if (!((reg_addr & 0x3) == 3)) {
                        channels[channel_idx].operators[operator_idx].envelope_generator.decay_rate = value & 0x1F;
                        channels[channel_idx].operators[operator_idx].envelope_generator.am_enable = (value >> 7) & 1;
                    }
                    break;
                case 0x70:
                case 0x71:
                case 0x72:
                case 0x73:
                case 0x74:
                case 0x75:
                case 0x76:
                case 0x77:
                case 0x78:
                case 0x79:
                case 0x7A:
                case 0x7B:
                case 0x7C:
                case 0x7D:
                case 0x7E:
                case 0x7F:
                    if (!((reg_addr & 0x3) == 3)) {
                        channels[channel_idx].operators[operator_idx].envelope_generator.sustain_rate = value & 0x1F;
                    }
                    break;
                case 0x80:
                case 0x81:
                case 0x82:
                case 0x83:
                case 0x84:
                case 0x85:
                case 0x86:
                case 0x87:
                case 0x88:
                case 0x89:
                case 0x8A:
                case 0x8B:
                case 0x8C:
                case 0x8D:
                case 0x8E:
                case 0x8F:
                    if (!((reg_addr & 0x3) == 3)) {
                        channels[channel_idx].operators[operator_idx].envelope_generator.release_rate = value & 0xF;
                        channels[channel_idx].operators[operator_idx].envelope_generator.sustain_level = value >> 4;
                    }
                    break;
                case 0xA0:
                case 0xA1:
                case 0xA2: {
                    if (!((reg_addr & 0x3) == 3)) {
                        uint32_t f = ((channels[channel_idx].operator_f_number_hi[3] & 7) << 8) | value;
                        channels[channel_idx].operator_f_number[3] = f;
                        channels[channel_idx].operator_block[3] =
                            (channels[channel_idx].operator_f_number_hi[3] >> 3) & 7;
                        channels[channel_idx].update_frequency();
                    }
                }
                    break;
                case 0xA4:
                case 0xA5:
                case 0xA6: {
                    if (!((reg_addr & 0x3) == 3)) {
                        channels[channel_idx].operator_f_number_hi[3] = value;
                    }
                }
                    break;
                case 0xA9:
                case 0xAA:
                case 0xA8: {
                    int op = 0;
                    if (reg_addr == 0xAA) {
                        op = 1;
                    }
                    else if (reg_addr == 0xA8) {
                        op = 2;
                    }
                    uint32_t f = (((uint32_t)channels[2].operator_f_number_hi[op] & 7) << 8) | value;
                    channels[2].operator_f_number[op] = f;
                    channels[2].operator_block[op] = (channels[2].operator_f_number_hi[op] >> 3) & 7;
                    channels[2].update_frequency();
                }
                    break;
                case 0xAD:
                case 0xAE:
                case 0xAC: {
                    int op = 0;
                    if (reg_addr == 0xAE) {
                        op = 1;
                    }
                    else if (reg_addr == 0xAC) {
                        op = 2;
                    }
                    channels[2].operator_f_number_hi[op] = value;
                }
                    break;
                case 0xB0:
                case 0xB1:
                case 0xB2: {
                    if (!((reg_addr & 0x3) == 3)) {
                        channels[channel_idx].algorithm = value & 7;
                        channels[channel_idx].feedback = (value >> 3) & 7;
                    }
                } break;
                case 0xB4:
                case 0xB5:
                case 0xB6: {
                    if (!((reg_addr & 0x3) == 3)) {
                        channels[channel_idx].panning = value >> 6;
                    }
                } break;
            }
            break;
    }
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
    uint32_t modulated_f_num = (f_number << 1) & 0xFFF;
    uint32_t shifted_f_num = (modulated_f_num << block) >> 2;

    //todo: modulation

    uint32_t key_code = compute_key_code(f_number, block);
    uint32_t detune_magnitude = detune & 3;
    uint32_t detune_increment_magnitude = detune_table[key_code][detune_magnitude];

    uint32_t detuned_f_num = 0;
    if ((detune_increment_magnitude >> 2) & 0x1) {
        detuned_f_num = (shifted_f_num - detune_increment_magnitude) & 0x1FFFF;
    }
    else {
        detuned_f_num = (shifted_f_num + detune_increment_magnitude) & 0x1FFFF;
    }

    if (multiple) {
        detuned_f_num *= multiple;
    }
    else {
        detuned_f_num >>= 1;
    }


    counter = (counter + detuned_f_num) & ((1 << 20) - 1);
}

uint32_t c_phase_generator::output()
{
    return counter >> 10;
}

void c_phase_generator::reset_counter()
{
    counter = 0;
}

void c_fm_operator::reset()
{
    key_on = false;
    current_output = 0;
    last_output = 0;
    phase_generator.reset();
    envelope_generator.reset();
}

void c_fm_operator::key(bool on)
{
    //if (on && !key_on) {
    //    phase_generator.reset_counter();
    //    key_on = true;
    //}
    //else {
    //    key_on = on;
    //}

    //envelope_generator.set_key_on(on);
    if (on) {
        if (!(envelope_generator.phase != ADSR_PHASE::RELEASE)) {
            phase_generator.reset_counter();
            envelope_generator.set_key_on(true);
        }
    }
    else {
        envelope_generator.set_key_on(false);
    }
}

void c_fm_operator::update_frequency(uint32_t f_number, uint8_t block)
{
    phase_generator.f_number = f_number;
    phase_generator.block = block;
    update_key_scale_rate();
}

void c_fm_operator::update_key_scale(uint32_t key_scale)
{
    envelope_generator.key_scale = key_scale;
    update_key_scale_rate();
}

void c_fm_operator::update_key_scale_rate()
{
    uint32_t key_code = compute_key_code(phase_generator.f_number, phase_generator.block);
    envelope_generator.key_scale_rate = key_code >> (3 - envelope_generator.key_scale);
}

int32_t c_fm_operator::output(int32_t modulation_input)
{
    //modulation_input >>= 4; //this is definitely not right but helps... why?
    uint32_t phase = (phase_generator.output() & 0x3FF) + (uint32_t)(modulation_input & 0x3FF);
    phase &= 0x3FF;

    uint32_t sign = phase & 0x200;

    uint32_t sine_attenuation = log_sin_table.at(phase & (0x3FF >> 1));

    uint32_t envelope_attenuation = envelope_generator.output();
    //todo: amplitude modulation
    uint32_t envelope_am_attenuation = envelope_attenuation;

    uint32_t total_attenuation = sine_attenuation + (envelope_am_attenuation << 2);
    if (total_attenuation > 0x1FFF) {
        total_attenuation = 0x1FFF;
    }

    uint32_t amplitude = attenuation_to_amplitude(total_attenuation);
    int32_t output = sign ? 0 - amplitude: amplitude;
    
    last_output = current_output;
    current_output = output;

    return output;
}

uint32_t c_fm_operator::attenuation_to_amplitude(uint32_t attenuation)
{
    uint32_t int_part = (attenuation >> 8) & 0x1F;
    if (int_part >= 13) {
        return 0;
        //int x = 1;
    }
    uint32_t fract_part = attenuation & 0xFF;
    uint32_t fract_pow2 = pow2_table.at(fract_part);
    return ((fract_pow2 << 2) >> int_part);
}

void c_fm_channel::reset()
{
    out = 0.0f;
    panning = 3;
    algorithm = 0;
    feedback = 0;
    multi_frequency_operators = false;
    f_number = 0;
    block = 0;
    memset(operator_f_number_hi, 0, sizeof(operator_f_number_hi));
    memset(operator_f_number, 0, sizeof(operator_f_number));
    memset(operator_f_number, 0, sizeof(operator_block));

    for (auto &op : operators) {
        op.reset();
    }
}

void c_fm_channel::update_frequency()
{
    for (int i = 0; i < 4; i++) {
        int f_index = multi_frequency_operators ? i : 3;
        operators[i].update_frequency(operator_f_number[f_index], operator_block[f_index]);
    }
}

void c_fm_channel::clock()
{
    for (auto &op : operators) {
        op.phase_generator.clock();
        op.envelope_generator.clock();
    }

    int32_t op1_feedback = 0;
    if (feedback) {
        op1_feedback = (operators[0].current_output + operators[0].last_output) >> (10 - feedback);
    }
    //todo quanitzation mask on output
    int32_t int_out = 0;
    uint32_t mask = ~((1 << 5) - 1);
    switch (algorithm) {
        case 0: {
            int32_t m1 = operators[0].output(op1_feedback);
            //int32_t m2_old = operators[1].current_output;
            //operators[1].output(m1 >> 1);
            //int32_t m3 = operators[2].output(m2_old >> 1);
            int32_t m2 = operators[1].output(m1 >> 1);
            int32_t m3 = operators[2].output(m2 >> 1);
            int32_t c4 = operators[3].output(m3 >> 1);
            int_out = c4;
        } break;
        case 1: {
            int32_t m1 = operators[0].output(op1_feedback);
            int32_t m2 = operators[1].output(0);
            int32_t m3 = operators[2].output((m1 + m2) >> 1);
            int32_t c4 = operators[3].output(m3 >> 1);
            int_out = c4;
        } break;
        case 2: {
            int32_t m1 = operators[0].output(op1_feedback);
            int32_t m2 = operators[1].output(0);
            int32_t m3 = operators[2].output(m2 >> 1);
            int32_t c4 = operators[3].output((m1 + m3) >> 1);
            int_out = c4;
        } break;
        case 3: {
            int32_t m1 = operators[0].output(op1_feedback);
            int32_t m2 = operators[1].output(m1 >> 1);
            int32_t m3 = operators[2].output(0);
            int32_t c4 = operators[3].output((m2 + m3) >> 1);
            int_out = c4;
        } break;
        case 4: {
            int32_t m1 = operators[0].output(op1_feedback);
            int32_t m3 = operators[2].output(0);
            int32_t c2 = operators[1].output(m1 >> 1) & mask;
            int32_t c4 = operators[3].output(m3 >> 1) & mask;
            int_out = c2 + c4;
        } break;
        case 5: {
            int32_t m1 = operators[0].output(op1_feedback);
            int32_t c2 = operators[1].output(m1 >> 1) & mask;
            int32_t c3 = operators[2].output(m1 >> 1) & mask;
            int32_t c4 = operators[3].output(m1 >> 1) & mask;
            int_out = c2 + c3 + c4;
        } break;
        case 6: {
            int32_t m1 = operators[0].output(op1_feedback);
            int32_t c2 = operators[1].output(m1 >> 1) & mask;
            int32_t c3 = operators[2].output(0) & mask;
            int32_t c4 = operators[3].output(0) & mask;
            int_out = c2 + c3 + c4;
        } break;
        case 7: {
            int32_t c1 = operators[0].output(op1_feedback) & mask;
            int32_t c2 = operators[1].output(0) & mask;
            int32_t c3 = operators[2].output(0) & mask;
            int32_t c4 = operators[3].output(0) & mask;
            int_out = c1 + c2 + c3 + c4;
        } break;
        default:
            break;
    }
    out_i = out;
    out = std::clamp(int_out, (int32_t)(-0x2000 & mask), (int32_t)(0x1FFF & mask));
    out /= 8192.0f;
}

float c_fm_channel::output()
{
    return out;
}

void c_envelope_generator::reset()
{
    phase = ADSR_PHASE::RELEASE;
    key_on = false;
    //level = 0;
    sustain_level = 0;

    attack_rate = 0;
    decay_rate = 0;
    sustain_rate = 0;
    release_rate = 0;
    cycle_count = 1;

    attenuation = 0x3FF;

    total_level = 0;
    rate_scaling = 0;

    am_enable = 0;
    divider = 0;

    key_scale = 0;
    key_scale_rate = 0;
}

void c_envelope_generator::set_key_on(bool k)
{
    if (!k) {
        //key off
        phase = ADSR_PHASE::RELEASE;
        return;
    }


    if (phase != ADSR_PHASE::RELEASE) {
        return;
    }
    
    //phase = key_on ? ADSR_PHASE::ATTACK : ADSR_PHASE::RELEASE;
    uint32_t rate = 2 * attack_rate + key_scale_rate;
    //should attack_rate of 0 = rate 0?
    if (rate >= 62) {
        phase = ADSR_PHASE::DECAY;
        attenuation = 0;
    }
    else {
        phase = ADSR_PHASE::ATTACK;
    }
}

void c_envelope_generator::clock()
{
    if (++divider != 3) {
        return;
    }
    divider = 0;

    cycle_count++;
    cycle_count = (cycle_count & 0xFFF) + (cycle_count >> 12);

    uint32_t sl_steps = sustain_level == 15 ? (uint32_t)0x3FF >> 5 : sustain_level;
    sl_steps <<= 5;

    if (phase == ADSR_PHASE::ATTACK && attenuation == 0) {
        phase = ADSR_PHASE::DECAY;
    }

    if (phase == ADSR_PHASE::DECAY && attenuation >= sl_steps) {
        phase = ADSR_PHASE::SUSTAIN;
    }
    uint32_t r = 0;
    switch (phase) {
        case ADSR_PHASE::ATTACK:
            r = attack_rate;
            break;
        case ADSR_PHASE::DECAY:
            r = decay_rate;
            break;
        case ADSR_PHASE::SUSTAIN:
            r = sustain_rate;
            break;
        case ADSR_PHASE::RELEASE:
            r = (release_rate << 1) | 1;
            break;
    }

    uint32_t rate = r == 0 ? 0 : std::min(63u, 2 * r + key_scale_rate);

    uint32_t rr = rate >> 2;
    if (rr > 11) {
        rr = 11;
    }
    uint32_t update_frequency_shift = 11 - rr;

    if ((cycle_count & ((1 << update_frequency_shift) - 1)) == 0) {
        uint32_t increment_idx = (cycle_count >> update_frequency_shift) & 7;
        uint32_t increment = attenuation_increments[rate][increment_idx];

        switch (phase) {
            case ADSR_PHASE::ATTACK:
                if (rate <= 61) {
                    attenuation = attenuation + ((~attenuation * increment) >> 4);
                    attenuation &= 0x3FF;
                }
                break;
            default:
                attenuation = std::min((uint32_t)0x3FF, attenuation + increment);
        }
    }
    

}

uint32_t c_envelope_generator::output()
{
    return std::min((uint32_t)0x3FF, attenuation + (total_level << 3));
}

uint32_t compute_key_code(uint32_t f_number, uint8_t block)
{
    uint32_t f11 = (f_number >> 10) & 0x1;
    uint32_t f10 = (f_number >> 9) & 0x1;
    uint32_t f9 = (f_number >> 8) & 0x1;
    uint32_t f8 = (f_number >> 7) & 0x1;

    return (block << 2) | (f11 << 1) | ((f11 && (f10 || f9 || f8)) || (!f11 && f10 && f9 && f8));

}