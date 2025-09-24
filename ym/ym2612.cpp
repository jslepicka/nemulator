module;
#include <cassert>
module ym2612;

static const std::array<uint32_t, 512> log_sin_table = [] {
    std::array<uint32_t, 512> ret;
    for (int x = 0; x < 512; x++) {
        int i = x;
        if ((i >> 8) & 1) {
            i = (~i) & 0xFF;
        }
        float n = ((i << 1) | 1);
        float sine = (float)std::sin((n / 512.0 * std::numbers::pi / 2.0));
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
    timer_a_enabled = false;
    timer_a_set_flag = false;

    timer_b = 0;
    timer_b_reload = 0;
    timer_b_enabled = false;
    timer_b_set_flag = false;

    status = 0;
    busy_counter = 0;
    timer_b_divider = 0;

    for (auto &channel : channels) {
        channel.reset();
    }

    lfo_counter = 0;
    lfo_divider = 0;
    lfo_freq = 0;
    lfo_enabled = 0;
    dac_output = 0;
    dac_enabled = false;
}

void c_ym2612::clock_timer_a()
{
    if (timer_a_enabled) {
        timer_a++;
        if (timer_a == 0x400) {
            if (timer_a_set_flag) {
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
            if (timer_b_set_flag) {
                status |= 2;
            }
            timer_b = timer_b_reload;
        }
    }
}

void c_ym2612::clock_lfo()
{
    lfo_divider++;
    if (lfo_divider >= lfo_freqs[lfo_freq]) {
        lfo_divider = 0;
        lfo_counter = (lfo_counter + 1) & 0x7F;
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
        
        if (lfo_enabled) {
            clock_lfo();
        }

        clock_channels();

        mix();
    }
}

void c_ym2612::mix()
{
    out_l = 0.0f;
    out_r = 0.0f;

    const uint32_t mask = ~((1u << 5) - 1);

    for (int i = 0; i < 6; i++) {
        int32_t sample;
        if (i == 5 && dac_enabled) {
            sample = get_dac_sample();
        }
        else {
            sample = channels[i].get_output();
        }

        auto output_ladder = [](int32_t sample, bool enabled) {
            if (enabled) {
                if (sample >= 0) {
                    sample += (4 << 5);
                }
                else {
                    sample -= (3 << 5);
                }
            }
            else {
                if (sample >= 0) {
                    sample = (4 << 5);
                }
                else {
                    sample = -((4 << 5));
                }
            }
            sample = std::clamp(sample, (int32_t)(-0x2000 & mask), (int32_t)(0x1FFF & mask));
            return (float)sample / 8192.0f;
        };
        out_l += output_ladder(sample, channels[i].panning & 0x1);
        out_r += output_ladder(sample, channels[i].panning & 0x2);
    }
    out_l /= 6.0f;
    out_r /= 6.0f;
}

int32_t c_ym2612::get_dac_sample()
{
    int32_t dac_sample = (int32_t)(dac_output) - 128;
    dac_sample = (int32_t)(((uint32_t)dac_sample) << 6);
    return dac_sample;
}

void c_ym2612::clock_channels()
{
    for (auto &channel : channels) {
        channel.clock();
    }
}

uint8_t c_ym2612::read(uint16_t address)
{
    return status;
}

void c_ym2612::write_global()
{
    switch (reg_addr) {
        case 0x22:
            lfo_freq = reg_value & 7;
            lfo_enabled = reg_value & 0x8;
            if (!lfo_enabled) {
                lfo_counter = 0;
            }
            break;
        case 0x24:
            timer_a_reload = (timer_a_reload & 3) | (reg_value << 2);
            break;
        case 0x25:
            //should timer_a be reset here?
            timer_a_reload = (timer_a_reload & ~3) | (reg_value & 0x3);
            break;
        case 0x26:
            timer_b_reload = reg_value;
            break;
        case 0x27:
            if (reg_value & 0x10) {
                status &= ~1;
            }
            if (reg_value & 0x20) {
                status &= ~2;
            }

            if (!timer_a_enabled && (reg_value & 0x1)) {
                timer_a_enabled = true;
                timer_a = timer_a_reload;
            }
            else if (!(reg_value & 0x1)) {
                timer_a_enabled = false;
            }

            if (!timer_b_enabled && (reg_value & 0x2)) {
                timer_b_enabled = true;
                timer_b = timer_b_reload;
            }
            else if (!(reg_value & 0x2)) {
                timer_b_enabled = false;
            }

            timer_a_set_flag = reg_value & 0x4;
            timer_b_set_flag = reg_value & 0x8;

            channels[2].multi_frequency_operators = ((reg_value >> 6) & 0x3) == 1;
            channels[2].update_frequency();
            break;
        case 0x28: {
            int c = reg_value & 0x7;
            if ((c & 3) != 3) {
                if (c >= 4) {
                    c -= 1;
                }
                for (int i = 0; i < 4; i++) {
                    channels[c].operators[i].key(reg_value & (0x10 << i));
                }
            }
        } break;
        case 0x2A:
            dac_output = reg_value;
            break;
        case 0x2B:
            dac_enabled = reg_value & 0x80;
            break;
    }
}

void c_ym2612::write_operator()
{
    if ((reg_addr & 3) == 3) {
        return;
    }
    auto &op = channels[channel_idx].operators[operator_idx];
    switch (reg_addr >> 4) {
        case 3:
            op.phase_generator.multiple = reg_value & 0xF;
            op.phase_generator.detune = (reg_value >> 4) & 7;
            break;
        case 4:
            op.envelope_generator.total_level = reg_value & 0x7F;
            break;
        case 5:
            op.envelope_generator.attack_rate = reg_value & 0x1F;
            op.update_key_scale((reg_value >> 6) & 0x3);
            break;
        case 6:
            op.envelope_generator.decay_rate = reg_value & 0x1F;
            op.am_enable = (reg_value >> 7) & 1;
            break;
        case 7:
            op.envelope_generator.sustain_rate = reg_value & 0x1F;
            break;
        case 8:
            op.envelope_generator.release_rate = reg_value & 0xF;
            op.envelope_generator.sustain_level = reg_value >> 4;
            break;
        case 9:
            op.envelope_generator.ssg_hold = reg_value & 0x1;
            op.envelope_generator.ssg_alternate = reg_value & 0x2;
            op.envelope_generator.ssg_attack = reg_value & 0x4;
            op.envelope_generator.ssg_enabled = reg_value & 0x8;
            break;
        default:
            break;
    }
}

void c_ym2612::write_channel()
{
    if ((reg_addr & 3) == 3) {
        return;
    }
    auto &channel = channels[channel_idx];
    switch (reg_addr >> 2) {
        case 0xA0 >> 2: {
                uint32_t f = ((channel.operator_f_number_hi[3] & 7) << 8) | reg_value;
                channel.operator_f_number[3] = f;
                channel.operator_block[3] = (channel.operator_f_number_hi[3] >> 3) & 7;
                channel.update_frequency();
        } break;
        case 0xA4 >> 2:
            channel.operator_f_number_hi[3] = reg_value;
            break;
        case 0xA8 >> 2: {
            int op = reg_addr & 3 ? (reg_addr & 3) - 1 : 2;
            uint32_t f = (((uint32_t)channels[2].operator_f_number_hi[op] & 7) << 8) | reg_value;
            channels[2].operator_f_number[op] = f;
            channels[2].operator_block[op] = (channels[2].operator_f_number_hi[op] >> 3) & 7;
            channels[2].update_frequency();
        } break;
        case 0xAC >> 2: {
            int op = reg_addr & 3 ? (reg_addr & 3) - 1 : 2;
            channels[2].operator_f_number_hi[op] = reg_value;
        } break;
        case 0xB0 >> 2:
            channel.algorithm = reg_value & 7;
            channel.feedback = (reg_value >> 3) & 7;
            break;
        case 0xB4 >> 2:
            channel.panning = reg_value >> 6;
            channel.fm_level = reg_value & 7;
            channel.am_level = (reg_value >> 4) & 3;
            break;
        default:
            break;
    }
}

void c_ym2612::write_register()
{
    switch (reg_addr >> 4) {
        case 2:
            write_global();
            break;
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
            write_operator();
            break;
        case 0xA:
        case 0xB:
            write_channel();
            break;
        default:
            break;
    }
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
            reg_value = value;
            busy_counter = 32;
            status |= 0x80;
            write_register();
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

void c_phase_generator::clock(uint8_t lfo_counter, uint8_t fm_level)
{
    uint8_t lfo_high = lfo_counter >> 2;
    uint8_t lfo_fm_idx = lfo_high & 7;
    if (lfo_high & (1 << 3)) {
        lfo_fm_idx ^= 7;
    }
    uint8_t multiplier = vibrato_table[fm_level][lfo_fm_idx];

    uint32_t f_num_delta = 0;
    uint32_t f_bits = (f_number >> 4) & 0x7F;
    f_num_delta += (f_bits & 0x01) ? (multiplier >> 6) : 0;
    f_num_delta += (f_bits & 0x02) ? (multiplier >> 5) : 0;
    f_num_delta += (f_bits & 0x04) ? (multiplier >> 4) : 0;
    f_num_delta += (f_bits & 0x08) ? (multiplier >> 3) : 0;
    f_num_delta += (f_bits & 0x10) ? (multiplier >> 2) : 0;
    f_num_delta += (f_bits & 0x20) ? (multiplier >> 1) : 0;
    f_num_delta += (f_bits & 0x40) ? (multiplier >> 0) : 0;

    uint32_t modulated_f_num = f_number << 1;
    if (lfo_high & (1 << 4)) {
        modulated_f_num -= f_num_delta;
    }
    else {
        modulated_f_num += f_num_delta;
    }
    uint32_t shifted_f_num = (modulated_f_num << block) >> 2;

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

uint32_t c_phase_generator::get_output()
{
    return counter >> 10;
}

void c_phase_generator::reset_counter()
{
    counter = 0;
}

void c_fm_operator::reset()
{
    current_output = 0;
    last_output = 0;
    phase_generator.reset();
    envelope_generator.reset();
    tremolo_attenuation = 0;
}

void c_fm_operator::key(bool key_on)
{
    if (key_on) {
        if (!(envelope_generator.phase != ADSR_PHASE::RELEASE)) {
            phase_generator.reset_counter();
            envelope_generator.key(true);
        }
    }
    else {
        envelope_generator.key(false);
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

int32_t c_fm_operator::get_output(int32_t modulation_input)
{
    uint32_t phase = (phase_generator.get_output() & 0x3FF) + (uint32_t)(modulation_input & 0x3FF);
    phase &= 0x3FF;
    uint32_t sign = phase & 0x200;

    uint32_t sine_attenuation = log_sin_table.at(phase & (0x3FF >> 1));
    uint32_t envelope_attenuation = envelope_generator.get_output();
    
    uint32_t envelope_am_attenuation = envelope_attenuation;
    if (am_enable) {
        envelope_am_attenuation = std::clamp(envelope_attenuation + tremolo_attenuation, 0u, 0x3FFu);
    }

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
    panning = 3;
    algorithm = 0;
    feedback = 0;
    multi_frequency_operators = false;
    f_number = 0;
    block = 0;
    fm_level = 0;
    am_level = 0;
    std::memset(operator_f_number_hi, 0, sizeof(operator_f_number_hi));
    std::memset(operator_f_number, 0, sizeof(operator_f_number));
    std::memset(operator_f_number, 0, sizeof(operator_block));

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
    uint8_t lfo_am = lfo_counter & 0x3F;
    uint32_t tremolo_attenuation = 0;
    if (!(lfo_counter & (1 << 6))) {
        lfo_counter ^= 0x3F;
    }
    lfo_am <<= 1;
    switch (am_level) {
        case 0:
            tremolo_attenuation = 0;
            break;
        case 1:
            tremolo_attenuation = lfo_am >> 3;
            break;
        case 2:
            tremolo_attenuation = lfo_am >> 1;
            break;
        case 3:
            tremolo_attenuation = lfo_am;
            break;
    }

    for (auto &op : operators) {
        op.phase_generator.clock(lfo_counter, fm_level);
        if (op.envelope_generator.clock()) {
            op.phase_generator.reset_counter();
        }
        op.tremolo_attenuation = tremolo_attenuation;
    }

    int32_t op1_feedback = 0;
    if (feedback) {
        op1_feedback = (operators[0].current_output + operators[0].last_output) >> (10 - feedback);
    }
    int32_t int_out = 0;
    const uint32_t mask = ~((1u << 5) - 1);
    switch (algorithm) {
        case 0: {
            int32_t m1 = operators[0].get_output(op1_feedback);
            //int32_t m2_old = operators[1].current_output;
            //operators[1].output(m1 >> 1);
            //int32_t m3 = operators[2].output(m2_old >> 1);
            int32_t m2 = operators[1].get_output(m1 >> 1);
            int32_t m3 = operators[2].get_output(m2 >> 1);
            int32_t c4 = operators[3].get_output(m3 >> 1);
            int_out = c4;
        } break;
        case 1: {
            int32_t m1 = operators[0].get_output(op1_feedback);
            int32_t m2 = operators[1].get_output(0);
            int32_t m3 = operators[2].get_output((m1 + m2) >> 1);
            int32_t c4 = operators[3].get_output(m3 >> 1);
            int_out = c4;
        } break;
        case 2: {
            int32_t m1 = operators[0].get_output(op1_feedback);
            int32_t m2 = operators[1].get_output(0);
            int32_t m3 = operators[2].get_output(m2 >> 1);
            int32_t c4 = operators[3].get_output((m1 + m3) >> 1);
            int_out = c4;
        } break;
        case 3: {
            int32_t m1 = operators[0].get_output(op1_feedback);
            int32_t m2 = operators[1].get_output(m1 >> 1);
            int32_t m3 = operators[2].get_output(0);
            int32_t c4 = operators[3].get_output((m2 + m3) >> 1);
            int_out = c4;
        } break;
        case 4: {
            int32_t m1 = operators[0].get_output(op1_feedback);
            int32_t m3 = operators[2].get_output(0);
            int32_t c2 = operators[1].get_output(m1 >> 1) & mask;
            int32_t c4 = operators[3].get_output(m3 >> 1) & mask;
            int_out = c2 + c4;
        } break;
        case 5: {
            int32_t m1 = operators[0].get_output(op1_feedback);
            int32_t c2 = operators[1].get_output(m1 >> 1) & mask;
            int32_t c3 = operators[2].get_output(m1 >> 1) & mask;
            int32_t c4 = operators[3].get_output(m1 >> 1) & mask;
            int_out = c2 + c3 + c4;
        } break;
        case 6: {
            int32_t m1 = operators[0].get_output(op1_feedback);
            int32_t c2 = operators[1].get_output(m1 >> 1) & mask;
            int32_t c3 = operators[2].get_output(0) & mask;
            int32_t c4 = operators[3].get_output(0) & mask;
            int_out = c2 + c3 + c4;
        } break;
        case 7: {
            int32_t c1 = operators[0].get_output(op1_feedback) & mask;
            int32_t c2 = operators[1].get_output(0) & mask;
            int32_t c3 = operators[2].get_output(0) & mask;
            int32_t c4 = operators[3].get_output(0) & mask;
            int_out = c1 + c2 + c3 + c4;
        } break;
        default:
            break;
    }
    out_i = int_out;
}

int32_t c_fm_channel::get_output()
{
    return out_i;
}

void c_envelope_generator::reset()
{
    phase = ADSR_PHASE::RELEASE;
    sustain_level = 0;
    attack_rate = 0;
    decay_rate = 0;
    sustain_rate = 0;
    release_rate = 0;
    cycle_count = 1;

    attenuation = 0x3FF;
    total_level = 0;
    rate_scaling = 0;
    divider = 0;
    key_scale = 0;
    key_scale_rate = 0;

    ssg_enabled = false;
    ssg_attack = false;
    ssg_alternate = false;
    ssg_hold = false;
    ssg_invert_output = false;
}

void c_envelope_generator::key(bool key_on)
{
    if (!key_on) {
        //key off

        if (ssg_enabled && phase != ADSR_PHASE::RELEASE && ssg_invert_output != ssg_attack) {
            attenuation = 0x200 - attenuation;
            attenuation &= 0x3FF;
        }

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
    ssg_invert_output = false;
}

bool c_envelope_generator::ssg_clock()
{
    if (attenuation < 0x200) {
        return false;
    }

    if (ssg_alternate) {
        if (ssg_hold) {
            ssg_invert_output = true;
        }
        else {
            ssg_invert_output = !ssg_invert_output;
        }
    }

    if (!ssg_alternate && ssg_hold) {
        return true;
    }

    if ((phase == ADSR_PHASE::DECAY || phase == ADSR_PHASE::SUSTAIN) && !ssg_hold) {
        if (2 * attack_rate + key_scale_rate >= 62) {
            attenuation = 0;
            phase = ADSR_PHASE::DECAY;
        }
        else {
            phase = ADSR_PHASE::ATTACK;
        }
    }
    else if (phase == ADSR_PHASE::RELEASE || (phase != ADSR_PHASE::ATTACK && ssg_invert_output == ssg_attack)) {
        attenuation = 0x3FF;
    }

    return false;
}

bool c_envelope_generator::clock()
{
    bool reset_phase = false;

    if (ssg_enabled) {
        reset_phase = ssg_clock();
    }

    if (++divider != 3) {
        return reset_phase;
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
                if (ssg_enabled) {
                    if (attenuation < 0x200) {
                        attenuation = std::min(0x3FFu, attenuation + 4 * increment);
                    }
                }

                attenuation = std::min((uint32_t)0x3FF, attenuation + increment);
        }
    }
    return reset_phase;
}

uint32_t c_envelope_generator::get_output()
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