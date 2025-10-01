module;

module gb:apu;
import gb;

namespace gb
{

constexpr double c_gbapu::GB_AUDIO_RATE = (456.0f * 154.0f * 60.0f) / 4.0f;

c_gbapu::c_gbapu(c_gb *gb)
{
    this->gb = gb;
    mixer_enabled = 0;

    resampler = resampler_t::create((float)(GB_AUDIO_RATE / 48000.0));
}

c_gbapu::~c_gbapu()
{
}

void c_gbapu::reset()
{
    frame_seq_counter = CLOCKS_PER_FRAME_SEQ;
    frame_seq_step = 0;
    NR50 = 0;
    NR51 = 0;
    NR52 = 0;
    
    // https://gbdev.io/pandocs/Power_Up_Sequence.html#hardware-registers
    const struct
    {
        uint16_t address;
        uint8_t value;
    } power_on_values[] = {
        {0xFF26, 0xF1},
        {0xFF25, 0xF3},
        {0xFF24, 0x77},
        {0xFF23, 0xBF},
        {0xFF22, 0x00},
        {0xFF21, 0x00},
        {0xFF20, 0xFF},
        {0xFF1E, 0xBF},
        {0xFF1D, 0xFF},
        {0xFF1C, 0x9F},
        {0xFF1B, 0xFF},
        {0xFF1A, 0x7F},
        {0xFF19, 0xBF},
        {0xFF18, 0xFF},
        {0xFF17, 0x00},
        {0xFF16, 0x3F},
        {0xFF14, 0xBF},
        {0xFF13, 0xFF},
        {0xFF12, 0xF3},
        {0xFF11, 0xBF},
        {0xFF10, 0x80},
    };

    for (auto &p : power_on_values) {
        write_byte(p.address, p.value);
    }

    square1.reset();
    square2.reset();
    wave.reset();
    noise.reset();
    std::memset(registers, 0, sizeof(uint32_t) * 64);
}

void c_gbapu::set_audio_rate(double freq)
{
    double x = GB_AUDIO_RATE / freq;
    resampler->set_m((float)x);
}

void c_gbapu::enable_mixer()
{
    mixer_enabled = 1;
}

void c_gbapu::disable_mixer()
{
    mixer_enabled = 0;
}

int c_gbapu::get_buffers(const float **buf_l, const float **buf_r)
{
    int num_samples = resampler->get_output_buf(0, buf_l);
    resampler->get_output_buf(1, buf_r);
    return num_samples;
}

void c_gbapu::clear_buffers()
{
    resampler->clear_buf();
}

void c_gbapu::write_byte(uint16_t address, uint8_t data)
{
    int x = 1;
    registers[address & 0x3F] = data;
    if (!(NR52 & 0x80) && address != 0xFF26) {
        //NR52 controls power to the sound hardware. When powered off, all registers (NR10-NR51) are instantly written
        //with zero and any writes to those registers are ignored while power remains off (except on the DMG, where
        //length counters are unaffected by power and can still be written while off)
        int x = 1;
        return;
    }
    switch (address) {
        case 0xFF10:
        case 0xFF11:
        case 0xFF12:
        case 0xFF13:
        case 0xFF14:
            //square 1
            square1.write(address - 0xFF10, data);
            break;

        //case 0xFF15 - this is unused on square 2
        case 0xFF16:
        case 0xFF17:
        case 0xFF18:
        case 0xFF19:
            //square 2
            square2.write(address - 0xFF15, data);
            break;

        case 0xFF1A:
        case 0xFF1B:
        case 0xFF1C:
        case 0xFF1D:
        case 0xFF1E:
        case 0xFF30:
        case 0xFF31:
        case 0xFF32:
        case 0xFF33:
        case 0xFF34:
        case 0xFF35:
        case 0xFF36:
        case 0xFF37:
        case 0xFF38:
        case 0xFF39:
        case 0xFF3A:
        case 0xFF3B:
        case 0xFF3C:
        case 0xFF3D:
        case 0xFF3E:
        case 0xFF3F:
            //wave
            wave.write(address, data);
            break;

        case 0xFF20:
        case 0xFF21:
        case 0xFF22:
        case 0xFF23:
            //noise
            noise.write(address - 0xFF20, data);
            break;
        /*
        NR50 FF24 ALLL BRRR Vin L enable, Left vol, Vin R enable, Right vol
        NR51 FF25 NW21 NW21 Left enables, Right enables
        NR52 FF26 P--- NW21 Power control/status, Channel length statuses
        */
        case 0xFF24:
            right_vol = ((data & 0x7) + 1) / 8.0f;
            left_vol = (((data >> 4) & 0x7) + 1) / 8.0f;
            NR50 = data;
            break;
        case 0xFF25:
            enable_1_l = data & 0x1 ? 1 : 0;
            enable_2_l = data & 0x2 ? 1 : 0;
            enable_w_l = data & 0x4 ? 1 : 0;
            enable_n_l = data & 0x8 ? 1 : 0;

            enable_1_r = data & 0x10 ? 1 : 0;
            enable_2_r = data & 0x20 ? 1 : 0;
            enable_w_r = data & 0x40 ? 1 : 0;
            enable_n_r = data & 0x80 ? 1 : 0;
            NR51 = data;
            break;
        case 0xFF26:
            if ((NR52 ^ data) & 0x80) { //power state change
                if (data & 0x80) {
                    power_on();
                }
                else {
                    power_off();
                }
            }
            NR52 = data;
            break;
    }
}

uint8_t c_gbapu::read_byte(uint16_t address)
{
    if ((address & 0xFFF0) == 0xFF30) {
        if (wave.enabled) {
            return 0xFF;
        }
    }
    if (address == 0xFF26) {
        //return NR52;
        uint8_t return_value = NR52 & 0xF0 | (square1.enabled ? 1 << 0 : 0) | (square2.enabled ? 1 << 1 : 0) |
                               (wave.enabled ? 1 << 2 : 0) | (noise.enabled ? 1 << 3 : 0);
        return return_value;
    }
    else {
        return registers[address & 0x3F];
    }
}

void c_gbapu::power_on()
{
    frame_seq_step = 0;
    frame_seq_counter = CLOCKS_PER_FRAME_SEQ;
    square1.power_on();
    square2.power_on();
    noise.power_on();
    wave.power_on();
}

void c_gbapu::power_off()
{
    for (int i = 0xFF10; i <= 0xFF25; i++) {
        write_byte(i, 0);
    }
}

void c_gbapu::clock()
{
    static int clock_count = 0;

    //effective rate = 1.05MHz

    if (tick & 0x1) {
        //512kHz
        noise.clock_timer();
    }

    //For efficiency, we can clock everything at half rate
    //All timer periods need to be divided by 2
    //CLOCKS_PER_FRAME_SEQ also needs to be divided by 2
    for (int i = 0; i < 2; i++) {
        //effective rate = 2.1MHz

        int x = 1;
        //clock timers
        square1.clock_timer();
        square2.clock_timer();
        //noise.clock_timer();
        wave.clock_timer();
        clock_count++;

        frame_seq_counter--;
        if (frame_seq_counter <= 0) {
            frame_seq_counter = CLOCKS_PER_FRAME_SEQ;
            switch (frame_seq_step & 0x7) {
                case 2:
                case 6:
                    //clock sweep
                    square1.clock_sweep();
                    [[fallthrough]];
                case 0:
                case 4:
                    //length
                    square1.clock_length();
                    square2.clock_length();
                    noise.clock_length();
                    wave.clock_length();
                    break;
                case 1:
                case 3:
                case 5:
                    //nothing
                    break;
                case 7:
                    //vol env
                    square1.clock_envelope();
                    square2.clock_envelope();
                    noise.clock_envelope();
                    break;
            }
            frame_seq_step++;
        }
    }
    if (mixer_enabled) {
        mix();
    }
    tick++;
}

void c_gbapu::mix()
{
    float left_sample = 0.0f;
    float right_sample = 0.0f;

    //output range of each channel is 0 - 15
    float square1_out = (float)square1.get_output();
    float square2_out = (float)square2.get_output();
    float wave_out = (float)wave.get_output();
    float noise_out = (float)noise.get_output();

    int square1_dac = square1.dac_power;
    int square2_dac = square2.dac_power;
    int wave_dac = wave.dac_power;
    int noise_dac = noise.dac_power;


    left_sample = 
        ((square1_dac & enable_1_l) ? square1_out : 7.5f) +
        ((square2_dac & enable_2_l) ? square2_out : 7.5f) +
        ((wave_dac & enable_w_l) ? wave_out : 7.5f) +
        ((noise_dac & enable_n_l) ? noise_out : 7.5f);

    right_sample = 
        ((square1_dac & enable_1_r) ? square1_out : 7.5f) +
        ((square2_dac & enable_2_r) ? square2_out : 7.5f) +
        ((wave_dac & enable_w_r) ? wave_out : 7.5f) +
        ((noise_dac & enable_n_r) ? noise_out : 7.5f);

    //divide by 60 (max 15/channel * 4), then normalize to -1..1
    left_sample = (left_sample / 60.0f) * 2.0f - 1.0f;
    right_sample = (right_sample / 60.0f) * 2.0f - 1.0f;

    right_sample *= right_vol;
    left_sample *= left_vol;

    resampler->process({left_sample, right_sample});
}

c_gbapu::c_timer::c_timer()
{
    reset();
}

c_gbapu::c_timer::~c_timer()
{
}

void c_gbapu::c_timer::reset()
{
    counter = 1;
    period = 64;
}

int c_gbapu::c_timer::clock()
{
    int prev = counter;
    counter--;
    if (prev && counter == 0) {
        counter = period;
        return 1;
    }
    //if (counter == 0) {
    //    counter = period;
    //    return 1;
    //}
    //counter--;
    //counter &= 0x1FFF;
    return 0;
}

void c_gbapu::c_timer::set_period(int period)
{
    this->period = period;
}

c_gbapu::c_duty::c_duty()
{
    reset();
}

c_gbapu::c_duty::~c_duty()
{
}

void c_gbapu::c_duty::reset()
{
    duty_cycle = 0;
    step = 0;
}

void c_gbapu::c_duty::set_duty_cycle(int duty)
{
    duty_cycle = duty & 0x3;
}

void c_gbapu::c_duty::clock()
{
    step = (step + 1) & 0x7;
}

const int c_gbapu::c_duty::duty_cycle_table[4][8] = {
    {0, 0, 0, 0, 0, 0, 0, 1}, {1, 0, 0, 0, 0, 0, 0, 1}, {1, 0, 0, 0, 0, 1, 1, 1}, {0, 1, 1, 1, 1, 1, 1, 0}};

int c_gbapu::c_duty::get_output()
{
    return duty_cycle_table[duty_cycle][step];
}

void c_gbapu::c_duty::reset_step()
{
    step = 0;
}

c_gbapu::c_length::c_length()
{
    reset();
}

c_gbapu::c_length::~c_length()
{
}

void c_gbapu::c_length::reset()
{
    counter = 0;
    enabled = 0;
}

int c_gbapu::c_length::clock()
{
    if (enabled) {
        int x = 1;
    }
    if (enabled && counter != 0) {
        counter--;
        if (counter == 0) {
            enabled = 0;
        }
        return counter;
    }
    return 1;
}

int c_gbapu::c_length::get_output()
{
    return counter;
}

void c_gbapu::c_length::set_enable(int enable)
{
    enabled = enable;
}

void c_gbapu::c_length::set_length(int length)
{
    counter = length;
}

c_gbapu::c_envelope::c_envelope()
{
    reset();
}

c_gbapu::c_envelope::~c_envelope()
{
}

void c_gbapu::c_envelope::reset()
{
    volume = 0;
    counter = 0;
    period = 0;
    output = 0;
    mode = 0;
}

int c_gbapu::c_envelope::get_output()
{
    return output;
}

void c_gbapu::c_envelope::set_volume(int volume)
{
    this->volume = volume;
    output = volume;
}

void c_gbapu::c_envelope::set_period(int period)
{
    this->period = period;
    counter = period;
}

void c_gbapu::c_envelope::set_mode(int mode)
{
    this->mode = mode;
}

void c_gbapu::c_envelope::clock()
{
    if (period != 0) {
        int prev = counter;
        counter--;
        if (prev && counter == 0) {
            counter = period;
            int new_vol = output + (mode ? 1 : -1);
            if (new_vol >= 0 && new_vol < 16) {
                output = new_vol;
            }
        }
        //counter &= 0x7;
    }
}

c_gbapu::c_square::c_square()
{
    reset();
}

c_gbapu::c_square::~c_square()
{
}

void c_gbapu::c_square::clock_timer()
{
    if (++clock_divider == 2) {
        clock_divider = 0;
        if (timer.clock())
            duty.clock();
    }
}

void c_gbapu::c_square::clock_length()
{
    if (length.clock() == 0) {
        enabled = 0;
    }
}

void c_gbapu::c_square::clock_envelope()
{
    envelope.clock();
}

void c_gbapu::c_square::reset()
{
    enabled = 0;
    sweep_period = 0;
    sweep_negate = 1;
    sweep_shift = 0;
    starting_volume = 0;
    timer.reset();
    duty.reset();
    length.reset();
    length.counter = 64;
    period_hi = 0;
    period_lo = 0;
    timer.set_period(2048);
    envelope_period = 0;
    sweep_period = 0;
    sweep_counter = 0;
    sweep_shadow = 0;
    sweep_enabled = 0;
    dac_power = 0;
    clock_divider = 0;
}

void c_gbapu::c_square::power_on()
{
    //square duty units are reset to the first step of the waveform
    duty.reset_step();
}

void c_gbapu::c_square::write(int reg, uint8_t data)
{
    /*
    NR10 FF10 -PPP NSSS Sweep period, negate, shift
    NR11 FF11 DDLL LLLL Duty, Length load (64-L)
    NR12 FF12 VVVV APPP Starting volume, Envelope add mode, period
    NR13 FF13 FFFF FFFF Frequency LSB
    NR14 FF14 TL-- -FFF Trigger, Length enable, Frequency MSB
    */
    switch (reg) {
        case 0:
            sweep_period = (data >> 4) & 0x7;
            sweep_shift = data & 0x7;
            sweep_negate = data & 0x8 ? -1 : 1;
            break;
        case 1:
            duty.set_duty_cycle(data >> 6);
            length.set_length(64 - (data & 0x3F));
            break;
        case 2:
            starting_volume = data >> 4;
            envelope.set_volume(starting_volume);
            envelope.set_mode(data & 0x8);
            envelope_period = data & 0x7;
            envelope.set_period(envelope_period);
            dac_power = !!(data >> 3);
            if (!dac_power) {
                enabled = 0;
            }
            break;
        case 3:
            period_lo = data;
            timer.set_period((2048 - ((period_hi << 8) | period_lo)));
            break;
        case 4:
            period_hi = data & 0x7;
            timer.set_period((2048 - ((period_hi << 8) | period_lo)));
            length.set_enable(data & 0x40);
            if (data & 0x80) {
                trigger();
            }
            break;
    }
}

void c_gbapu::c_square::trigger()
{
    if (length.counter == 0) {
        length.counter = 64;
    }
    int p = (period_hi << 8) | period_lo;
    int freq = (2048 - p);
    timer.set_period(freq);
    sweep_shadow = p;
    sweep_counter = sweep_period;
    if (sweep_period || sweep_shift) {
        sweep_enabled = 1;
        calc_sweep();
    }
    else {
        sweep_enabled = 0;
    }
    envelope.set_period(envelope_period);
    envelope.set_volume(starting_volume);
    enabled = dac_power ? 1 : 0;
}

int c_gbapu::c_square::calc_sweep()
{
    sweep_freq = sweep_shadow + ((sweep_shadow >> sweep_shift) * sweep_negate);
    if (sweep_freq > 2047) {
        enabled = 0;
        sweep_enabled = 0;
    }
    return sweep_freq;
}

void c_gbapu::c_square::clock_sweep()
{
    if (sweep_enabled && sweep_period) {
        int prev = sweep_counter;
        sweep_counter = (sweep_counter - 1) & 0x7;
        if (sweep_counter == 0 && prev) {
            sweep_counter = sweep_period;
            int f = calc_sweep();
            if (f < 2048 && sweep_shift) {
                sweep_shadow = f;
                timer.set_period(2048 - f);
                calc_sweep();
            }
        }
    }
}

int c_gbapu::c_square::get_output()
{
    if (enabled && duty.get_output() /* && length.get_output()*/) {
        return envelope.get_output();
    }
    return 0;
}

c_gbapu::c_noise::c_noise()
{
    reset();
}

c_gbapu::c_noise::~c_noise()
{
}

void c_gbapu::c_noise::reset()
{
    lfsr = ~1;
    lfsr &= 0x7FFF;
    enabled = 0;
    clock_shift = 0;
    width_mode = 0;
    divisor = 1;
    starting_volume = 0;
    envelope_period = 0;
    envelope_mode = 0;
    timer.reset();
    envelope.reset();
    length.reset();
    length.counter = 64;
    timer.set_period(8);
    dac_power = 0;
    clock_divider = 0;
}

void c_gbapu::c_noise::write(uint16_t address, uint8_t data)
{
    /*
    NR41 FF20 --LL LLLL Length load (64-L)
    NR42 FF21 VVVV APPP Starting volume, Envelope add mode, period
    NR43 FF22 SSSS WDDD Clock shift, Width mode of LFSR, Divisor code
    NR44 FF23 TL-- ---- Trigger, Length enable
    */

    switch (address) {
        case 0:
            //length.set_length(64 - (data & 0x3F));
            next_length = 64 - (data & 0x3F);
            break;
        case 1:
            starting_volume = data >> 4;
            envelope.set_volume(starting_volume);
            envelope.set_mode(data & 0x8);
            envelope_period = data & 0x7;
            envelope.set_period(envelope_period);
            dac_power = !!(data >> 3);
            if (!dac_power) {
                enabled = 0;
            }
            break;
        case 2:
            clock_shift = data >> 4;
            width_mode = data & 0x8;
            divisor = (data & 0x7) * 16;
            if (divisor == 0)
                divisor = 8;
            divisor /= 8; //we're clocking the timer at 512kHz
            timer.set_period(divisor << clock_shift);
            break;
        case 3:
            length.set_enable(data & 0x40);
            if (data & 0x80) {
                trigger();
            }
            break;
    }
}

void c_gbapu::c_noise::trigger()
{
    lfsr = ~1;
    lfsr &= 0x7FFF;
    length.set_length(next_length);
    envelope.set_mode(envelope_mode);
    envelope.set_period(envelope_period);
    envelope.set_volume(starting_volume);
    enabled = dac_power ? 1 : 0;
}

void c_gbapu::c_noise::clock_timer()
{
    if (timer.clock()) {
        int x = (lfsr & 0x1) ^ ((lfsr & 0x2) >> 1);
        lfsr >>= 1;
        lfsr |= (x << 14);
        lfsr &= 0x7FFF;
        if (width_mode) {
            lfsr &= (~0x40);
            lfsr |= (x << 6);
        }
    }
}

void c_gbapu::c_noise::clock_length()
{
    if (length.clock() == 0) {
        enabled = 0;
    }
}

void c_gbapu::c_noise::clock_envelope()
{
    envelope.clock();
}

int c_gbapu::c_noise::get_output()
{
    if (enabled) {
        if ((~lfsr) & 0x1) {
            return envelope.get_output();
        }
    }
    return 0;
}

void c_gbapu::c_noise::power_on()
{
}

c_gbapu::c_wave::c_wave()
{
    reset();
}

c_gbapu::c_wave::~c_wave()
{
}

void c_gbapu::c_wave::reset()
{
    enabled = 0;
    timer_period = 0;
    timer.reset();
    length.reset();
    length.counter = 256;
    period_hi = 0;
    period_lo = 0;
    timer.set_period(2048 * (2 / 2));
    int startup_values[] = {0x84, 0x40, 0x43, 0xAA, 0x2D, 0x78, 0x92, 0x3C,
                            0x60, 0x59, 0x59, 0xB0, 0x34, 0xB8, 0x2E, 0xDA};
    auto w = wave_table;
    for (auto v : startup_values) {
        *w++ = (v >> 4) & 0xF;
        *w++ = v & 0xF;
    }
    volume_shift = 4;
    dac_power = 0;
}

void c_gbapu::c_wave::power_on()
{
    sample_buffer = 0;
}

void c_gbapu::c_wave::clock()
{
}

int c_gbapu::c_wave::get_output()
{
    if (enabled) {
        return sample_buffer >> volume_shift;
    }
    return 0;
}

void c_gbapu::c_wave::clock_timer()
{
    if (timer.clock()) {
        wave_pos = (wave_pos + 1) & 0x1F;
        sample_buffer = wave_table[wave_pos];
    }
}

void c_gbapu::c_wave::clock_length()
{
    if (length.clock() == 0) {
        enabled = 0;
    }
}

void c_gbapu::c_wave::write(uint16_t address, uint8_t data)
{
    /*
    NR30 FF1A E--- ---- DAC power
    NR31 FF1B LLLL LLLL Length load (256-L)
    NR32 FF1C -VV- ---- Volume code (00=0%, 01=100%, 10=50%, 11=25%)
    NR33 FF1D FFFF FFFF Frequency LSB
    NR34 FF1E TL-- -FFF Trigger, Length enable, Frequency MSB
    */
    static const int shift_table[4] = {4, 0, 1, 2};
    if (address >= 0xFF30) {
        int *p = &wave_table[(address - 0xFF30) * 2];
        *p = data >> 4;
        *(p + 1) = data & 0xF;
    }
    else {
        switch (address - 0xFF1A) {
            case 0:
                dac_power = !!(data & 0x80);
                if (!dac_power) {
                    enabled = 0;
                }
                break;
            case 1:
                length.set_length(256 - data);
                break;
            case 2:
                volume_shift = shift_table[(data >> 5) & 0x3];
                break;
            case 3:
                period_lo = data;
                timer.set_period((2048 - ((period_hi << 8) | period_lo)) * (2 / 2));
                break;
            case 4:
                period_hi = data & 0x7;
                timer.set_period((2048 - ((period_hi << 8) | period_lo)) * (2 / 2));
                length.set_enable(data & 0x40);
                if (data & 0x80) {
                    trigger();
                }
                break;
        }
    }
}

void c_gbapu::c_wave::trigger()
{
    if (length.counter == 0) {
        length.counter = 256;
    }
    timer.set_period((2048 - ((period_hi << 8) | period_lo)) * (2 / 2));
    wave_pos = 0;
    enabled = dac_power ? 1 : 0;
}

} //namespace gb
