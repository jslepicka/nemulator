export module gb:apu_channels;
import nemulator.std;

namespace gb
{
class c_timer
{
  public:
    c_timer()
    {
        reset();
    }

    void reset()
    {
        counter = 1;
        period = 64;
    }

    void set_period(int period)
    {
        this->period = period;
    }

    int clock()
    {
        int prev = counter;
        counter--;
        if (prev && counter == 0) {
            counter = period;
            return 1;
        }
        return 0;
    }

  private:
    int counter;
    int period;
    int mask;
};

class c_duty
{
  public:
    c_duty()
    {
        reset();
    }

    int get_output()
    {
        return duty_cycle_table[duty_cycle][step];
    }

    void set_duty_cycle(int duty)
    {
        duty_cycle = duty & 0x3;
    }

    void clock()
    {
        step = (step + 1) & 0x7;
    }

    void reset()
    {
        duty_cycle = 0;
        step = 0;
    }

    void reset_step()
    {
        step = 0;
    }

  private:
    static constexpr int duty_cycle_table[4][8] = {
        {0, 0, 0, 0, 0, 0, 0, 1}, {1, 0, 0, 0, 0, 0, 0, 1}, {1, 0, 0, 0, 0, 1, 1, 1}, {0, 1, 1, 1, 1, 1, 1, 0}};

    int duty_cycle;
    int step;
};

class c_length
{
  public:
    c_length()
    {
        reset();
    }

    int get_output()
    {
        return counter;
    }

    void set_length(int length)
    {
        counter = length;
    }

    void set_enable(int enable)
    {
        enabled = enable;
    }

    void reset()
    {
        counter = 0;
        enabled = 0;
    }

    int clock()
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

  public:
    int counter;

  private:
    int enabled;
};

class c_envelope
{
  public:
    c_envelope()
    {
        reset();
    }

    void reset()
    {
        volume = 0;
        counter = 0;
        period = 0;
        output = 0;
        mode = 0;
    }

    int get_output()
    {
        return output;
    }

    void set_volume(int volume)
    {
        this->volume = volume;
        output = volume;
    }

    void set_period(int period)
    {
        this->period = period;
        counter = period;
    }

    void set_mode(int mode)
    {
        this->mode = mode;
    }

    void clock()
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
        }
    }

  private:
    int volume;
    int counter;
    int period;
    int output;
    int mode;
    int enabled;
};

class c_square
{
  public:
    c_square()
    {
        reset();
    }

    void clock_timer()
    {
        if (++clock_divider == 2) {
            clock_divider = 0;
            if (timer.clock())
                duty.clock();
        }
    }

    void clock_length()
    {
        if (length.clock() == 0) {
            enabled = 0;
        }
    }

    void clock_sweep()
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

    void clock_envelope()
    {
        envelope.clock();
    }

    int get_output()
    {
        if (enabled && duty.get_output() /* && length.get_output()*/) {
            return envelope.get_output();
        }
        return 0;
    }

    void write(int reg, uint8_t data)
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

    void reset()
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

    void power_on()
    {
        //square duty units are reset to the first step of the waveform
        duty.reset_step();
    }

    void trigger()
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

  private:
    int calc_sweep()
    {
        sweep_freq = sweep_shadow + ((sweep_shadow >> sweep_shift) * sweep_negate);
        if (sweep_freq > 2047) {
            enabled = 0;
            sweep_enabled = 0;
        }
        return sweep_freq;
    }

  public:
    int enabled;
    int dac_power;

  private:
    c_timer timer;
    c_duty duty;
    c_length length;
    c_envelope envelope;
    int sweep_period;
    int sweep_negate;
    int sweep_shift;
    int sweep_shadow;
    int sweep_counter;
    int sweep_enabled;
    int sweep_freq;
    int starting_volume;
    int period_hi;
    int period_lo;
    int envelope_period;
    int clock_divider;
};

class c_noise
{
  public:
    c_noise()
    {
        reset();
    }

    void reset()
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

    int get_output()
    {
        if (enabled) {
            if ((~lfsr) & 0x1) {
                return envelope.get_output();
            }
        }
        return 0;
    }

    void trigger()
    {
        lfsr = ~1;
        lfsr &= 0x7FFF;
        length.set_length(next_length);
        envelope.set_mode(envelope_mode);
        envelope.set_period(envelope_period);
        envelope.set_volume(starting_volume);
        enabled = dac_power ? 1 : 0;
    }

    void write(uint16_t address, uint8_t data)
    {
        /*
        NR41 FF20 --LL LLLL Length load (64-L)
        NR42 FF21 VVVV APPP Starting volume, Envelope add mode, period
        NR43 FF22 SSSS WDDD Clock shift, Width mode of LFSR, Divisor code
        NR44 FF23 TL-- ---- Trigger, Length enable
        */

        switch (address) {
            case 0:
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

    void clock_timer()
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

    void clock_length()
    {
        if (length.clock() == 0) {
            enabled = 0;
        }
    }

    void clock_envelope()
    {
        envelope.clock();
    }

    void power_on()
    {
    }

  public:
    int enabled;
    int dac_power;

  private:
    int clock_shift;
    int width_mode;
    int divisor;
    int lfsr;
    int starting_volume;
    int envelope_period;
    int envelope_mode;
    c_timer timer;
    c_length length;
    c_envelope envelope;
    int clock_divider;
    int next_length = 64;
};

class c_wave
{
  public:
    c_wave()
    {
        reset();
    }

    void reset()
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

    void clock()
    {
    }

    int get_output()
    {
        if (enabled) {
            return sample_buffer >> volume_shift;
        }
        return 0;
    }

    void trigger()
    {
        if (length.counter == 0) {
            length.counter = 256;
        }
        timer.set_period((2048 - ((period_hi << 8) | period_lo)) * (2 / 2));
        wave_pos = 0;
        enabled = dac_power ? 1 : 0;
    }

    void write(uint16_t address, uint8_t data)
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

    void clock_timer()
    {
        if (timer.clock()) {
            wave_pos = (wave_pos + 1) & 0x1F;
            sample_buffer = wave_table[wave_pos];
        }
    }

    void clock_length()
    {
        if (length.clock() == 0) {
            enabled = 0;
        }
    }

    void power_on()
    {
        sample_buffer = 0;
    }

  public:
    int enabled;
    int dac_power;

  private:
    int starting_volume;
    int envelope_period;
    c_timer timer;
    c_length length;

    int timer_period;
    int sample_buffer;
    int wave_table[32];
    int wave_pos;
    int volume_shift;
    int period_hi;
    int period_lo;
};
} //namespace gb