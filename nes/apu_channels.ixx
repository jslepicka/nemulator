export module nes:apu_channels;
import nemulator.std;
import :callbacks;

class c_envelope
{
  public:
    c_envelope()
    {
        reset();
    }
    ~c_envelope()
    {
    }
    void clock()
    {
        if (reset_flag) {
            reset_flag = 0;
            env_vol = 15;
            counter = period;
        }
        else if (--counter == 0) {
            if (env_vol || loop)
                env_vol = (env_vol - 1) & 0xF;
            counter = period;
        }

        if (enabled)
            output = env_vol;
    }
    void write(unsigned char value)
    {
        period = (value & 0xF) + 1;
        enabled = !(value & 0x10);
        loop = value & 0x20;
        output = enabled ? env_vol : value & 0xF;
    }
    void reset_counter()
    {
        reset_flag = 1;
    }
    void reset()
    {
        reset_flag = 0;
        counter = 1;
        output = 0;
        period = 1;
        enabled = 0;
        loop = 0;
        env_vol = 0;
    }
    int get_output()
    {
        return output;
    }

  private:
    int period;
    int reset_flag;
    int output;
    int counter;
    int enabled;
    int loop;
    int env_vol;
    int reg_vol;
};

class c_timer
{
  public:
    c_timer()
    {
        reset();
    }
    ~c_timer()
    {
    }
    int clock()
    {
        if (--counter == 0) {
            counter = (period + 1);
            return 1;
        }
        else {
            return 0;
        }
    }
    void set_period_lo(int period_lo)
    {
        period = (period & 0x0700) | (period_lo & 0xFF);
    }
    void set_period_hi(int period_hi)
    {
        period = (period & 0xFF) | ((period_hi & 0x7) << 8);
    }
    void set_period(int value)
    {
        period = value;
    }
    int get_period()
    {
        return period;
    }
    void reset()
    {
        period = 1;
        counter = 1;
    }

  private:
    int shift;
    int counter;
    int period;
};

class c_sequencer
{
  public:
    c_sequencer()
    {
        reset();
    }
    ~c_sequencer()
    {
    }
    void clock()
    {
        step = --step & 0x7;
    }
    void set_duty_cycle(int duty)
    {
        duty_cycle = duty & 0x3;
    }
    int get_output()
    {
        return duty_cycle_table[duty_cycle][step];
    }
    void reset_step()
    {
        step = 0;
    }
    void reset()
    {
        duty_cycle = 0;
        step = 0;
        output = 0;
        ticks = 0;
    }

  private:
    static constexpr int duty_cycle_table[4][8] = {
        {0, 1, 0, 0, 0, 0, 0, 0},
        {0, 1, 1, 0, 0, 0, 0, 0},
        {0, 1, 1, 1, 1, 0, 0, 0},
        {1, 0, 0, 1, 1, 1, 1, 1}
    };
    int duty_cycle;
    int step;
    int output;
    int ticks;
};

class c_length
{
  public:
    c_length()
    {
        reset();
    }
    ~c_length()
    {
    }
    void clock()
    {
        if (!halt && counter != 0)
            counter--;
    }
    void set_length(int index)
    {
        if (enabled)
            counter = length_table[index & 0x1F];
    }
    void set_halt(int halt)
    {
        this->halt = halt;
    }
    int get_counter()
    {
        return counter;
    }
    int get_output()
    {
        return counter;
    }
    void reset()
    {
        counter = 0;
        halt = 0;
        length = 0;
        enabled = 0;
    }
    int get_halt()
    {
        return halt;
    }
    void enable()
    {
        enabled = 1;
    }
    void disable()
    {
        enabled = 0;
        counter = 0;
    }

  private:
    int enabled;
    int counter;
    int halt;
    int length;
    static constexpr int length_table[32] = {
        0x0A, 0xFE, 0x14, 0x02, 0x28, 0x04, 0x50, 0x06,
        0xA0, 0x08, 0x3C, 0x0A, 0x0E, 0x0C, 0x1A, 0x0E,
        0x0C, 0x10, 0x18, 0x12, 0x30, 0x14, 0x60, 0x16,
        0xC0, 0x18, 0x48, 0x1A, 0x10, 0x1C, 0x20, 0x1E
    };
};

export class c_square
{
  public:
    c_square()
    {
        reset();
    }
    ~c_square()
    {
    }
    void clock_envelope()
    {
        envelope.clock();
    }
    void clock_length()
    {
        length.clock();
    }
    void clock_length_sweep()
    {
        clock_length();
        int period = timer.get_period();
        if (sweep_period != 0 && --sweep_counter == 0) {
            sweep_counter = sweep_period;
            if (period >= 8) {
                sweep_output = 1;
                int offset = period >> sweep_shift;
                if (sweep_negate) {
                    if (sweep_shift && sweep_enable) {
                        timer.set_period(period + ((-1 * sweep_mode) - offset));
                        update_sweep_silencing();
                    }
                }
                else {
                    if (period + offset < 0x800) {
                        if (sweep_shift && sweep_enable) {
                            timer.set_period(period + offset);
                            update_sweep_silencing();
                        }
                    }
                }
            }
        }
        if (sweep_reset) {
            sweep_reset = 0;
            sweep_counter = sweep_period;
        }
    }
    void clock_timer()
    {
        if (timer.clock())
            sequencer.clock();
    }
    int get_output()
    {
        if (!sweep_silencing && sequencer.get_output() && length.get_output())
            return envelope.get_output();
        return 0;
    }
    int get_output_mmc5()
    {
        //MMC5 squares don't have the sweep unit.  This is faster than turning get_output
        //into a virtual function call.
        if (sequencer.get_output() && length.get_output())
            return envelope.get_output();
        return 0;
    }
    int get_status()
    {
        return length.get_counter() ? 1 : 0;
    }
    void enable()
    {
        length.enable();
    }
    void disable()
    {
        length.disable();
    }
    void write(unsigned short address, unsigned char value)
    {
        int period;
        switch (address & 0x3) {
            case 0x0:
                length.set_halt((value & 0x20) >> 5);
                envelope.write(value);
                sequencer.set_duty_cycle(value >> 6);
                break;
            case 0x1:
                sweep_enable = value & 0x80;
                sweep_negate = value & 0x8;
                sweep_shift = value & 0x7;
                sweep_period = 0;
                sweep_period = ((value & 0x70) >> 4) + 1;
                sweep_reset = 1;
                break;
            case 0x2:
                timer.set_period_lo(value);
                period = timer.get_period();
                update_sweep_silencing();
                break;
            case 0x3:
                sequencer.reset_step();
                timer.set_period_hi(value);
                length.set_length((value >> 3) & 0x1F);
                envelope.reset_counter();
                update_sweep_silencing();
                break;
        }
    }
    void set_sweep_mode(int mode)
    {
        sweep_mode = mode & 0x1;
    }
    void reset()
    {
        sweep_silencing = 1;
        output = 0;
        sweep_reg = 0;
        sweep_period = 0;
        sweep_reset = 0;
        sweep_enable = 0;
        sweep_negate = 0;
        sweep_shift = 0;
        sweep_output = 1;
        sweep_mode = 0;
        sweep_counter = 1;
        envelope.reset();
        timer.reset();
        sequencer.reset();
        length.reset();
    }

  private:
    int sweep_silencing;
    int sweep_mode;
    int sweep_reg;
    int sweep_reset;
    int sweep_period;
    int sweep_enable;
    int sweep_negate;
    int sweep_shift;
    int sweep_output;
    int sweep_counter;
    int output;
    void update_sweep_silencing()
    {
        int period = timer.get_period();
        int offset = period >> sweep_shift;
        sweep_silencing = ((period < 8) || (!sweep_negate && ((period + offset) > 0x7FF)));
    }
    c_envelope envelope;
    c_timer timer;
    c_sequencer sequencer;
    c_length length;
};

class c_triangle
{
  public:
    c_triangle()
    {
        reset();
    }
    ~c_triangle()
    {
    }
    void clock_timer()
    {
        //               +---------+    +---------+
        //               |LinearCtr|    | Length  |
        //               +---------+    +---------+
        //                    |              |
        //                    v              v
        //+---------+        |\             |\         +---------+    +---------+
        //|  Timer  |------->| >----------->| >------->|Sequencer|--->|   DAC   |
        //+---------+        |/             |/         +---------+    +---------+

        if (timer.clock()) {
            if (length.get_output() && linear_counter) {
                output = sequence[sequence_pos];
                sequence_pos = ++sequence_pos & 0x1F;
            }
        }
    }
    void clock_length()
    {
        length.clock();
    }
    void clock_linear()
    {
        if (linear_halt) {
            linear_counter = linear_reload;
        }
        else if (linear_counter != 0) {
            linear_counter--;
        }
        if (!linear_control) {
            linear_halt = 0;
        }
    }
    void enable()
    {
        length.enable();
    }
    void disable()
    {
        length.disable();
    }
    int get_status()
    {
        return length.get_counter() ? 1 : 0;
    }
    int get_output()
    {
        return output;
    }
    void write(unsigned short address, unsigned char value)
    {
        switch (address - 0x4000) {
            case 0x8:
                length_enabled = !(value & 0x80);
                linear_control = (value & 0x80);
                linear_reload = (value & 0x7F);
                length.set_halt((value & 0x80) >> 7);
                break;
            case 0xA:
                timer.set_period_lo(value);
                break;
            case 0xB:
                linear_halt = 1;
                timer.set_period_hi(value & 0x7);
                length.set_length(value >> 3);
                length.set_halt(1);
                break;
            default:
                break;
        }
    }
    void reset()
    {
        output = 0;
        sequence_pos = 0;
        linear_counter = 0;
        linear_reload = 0;
        length_enabled = 0;
        linear_control = 0;
        linear_halt = 0;
        timer.reset();
        length.reset();
    }

  private:
    static constexpr int sequence[32] = {
        0xF, 0xE, 0xD, 0xC, 0xB, 0xA, 0x9, 0x8, 0x7, 0x6, 0x5, 0x4, 0x3, 0x2, 0x1, 0x0,
        0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF
    };
    int sequence_pos;
    int linear_control;
    int linear_halt;
    int output;
    int linear_reload;
    int linear_counter;
    int length_enabled;
    c_timer timer;
    c_length length;
};

class c_noise
{
  public:
    c_noise()
    {
        reset();
    }
    ~c_noise()
    {
    }
    void clock_timer()
    {
        if (timer.clock()) {
            int feedback = (lfsr & 0x1) ^ ((lfsr >> lfsr_shift) & 0x1);
            feedback <<= 14;
            lfsr = (lfsr >> 1) | feedback;
        }
    }
    void clock_length()
    {
        length.clock();
    }
    void clock_envelope()
    {
        envelope.clock();
    }
    void write(unsigned short address, unsigned char value)
    {
        //$400C   --le nnnn   loop env/disable length, env disable, vol/env period
        //$400E   s--- pppp   short mode, period index
        //$400F   llll l---   length index
        switch (address - 0x4000) {
            case 0xC:
                length.set_halt((value & 0x20) >> 5);
                envelope.write(value);
                break;
            case 0xE:
                lfsr_shift = (value & 0x80) ? 6 : 1;
                random_period = random_period_table[value & 0xF];
                timer.set_period(random_period);
                break;
            case 0xF:
                length.set_length(value >> 3);
                envelope.reset_counter();
                break;
        }
    }
    int get_output()
    {
        if (length.get_output() && !(lfsr & 0x1)) {
            return envelope.get_output();
        }
        else {
            return 0;
        }
    }
    void enable()
    {
        length.enable();
    }
    void disable()
    {
        length.disable();
    }
    int get_status()
    {
        return length.get_counter() ? 1 : 0;
    }
    void reset()
    {
        output = 0;
        lfsr_shift = 1;
        random_period = 0;
        random_counter = 0;
        length_enabled = 0;
        lfsr = 0x1;
        length.reset();
        envelope.reset();
        timer.reset();
    }

  private:
    int lfsr;
    static constexpr int random_period_table[16] = {
        0x004, 0x008, 0x010, 0x020, 0x040, 0x060, 0x080, 0x0A0,
        0x0CA, 0x0FE, 0x17C, 0x1FC, 0x2FA, 0x3F8, 0x7F2, 0xFE4
    };
    int output;
    c_length length;
    c_envelope envelope;
    c_timer timer;
    int lfsr_shift;
    int random_period;
    int random_counter;
    int length_enabled;
};

export template <typename Nes> class c_dmc
{
    Nes &nes;

public:
    c_dmc(Nes &nes) : nes(nes)
    {
        reset();
    }
    ~c_dmc()
    {
    }
    int get_output()
    {
        return output_counter;
    }
    void enable()
    {
        if (duration == 0) {
            address = sample_address;
            duration = sample_length;
            fill_sample_buffer();
            enabled = 1;
        }
    }
    void disable()
    {
        duration = 0;
        enabled = 0;
    }
    int get_status()
    {
        return duration ? 1 : 0;
    }
    void clock_timer()
    {
        if (timer.clock()) {
            if (!silence) {
                if (output_shift & 0x1) {
                    if (output_counter < 126)
                        output_counter += 2;
                }
                else {
                    if (output_counter > 1)
                        output_counter -= 2;
                }
            }
            output_shift >>= 1;
            if (--bits_remain == 0) {
                bits_remain = 8;
                if (sample_buffer_empty) {
                    silence = 1;
                }
                else {
                    output_shift = sample_buffer;
                    sample_buffer_empty = 1;
                    fill_sample_buffer();
                    silence = 0;
                }
            }
        }
    }
    void write(unsigned short address, unsigned char value)
    {
        //$4010   il-- ffff   IRQ enable, loop, frequency index
        //$4011   -ddd dddd   DAC
        //$4012   aaaa aaaa   sample address
        //$4013   llll llll   sample length
        switch (address - 0x4000) {
            case 0x10:
                loop = value & 0x40;
                irq_enable = value & 0x80;
                freq_index = value & 0xF;
                timer.set_period(freq_table[freq_index] - 1);
                if (!irq_asserted && irq_enable && irq_flag) {
                    irq_asserted = 1;
                    //nes.on_irq(true);
                    nes.on_irq(true);
                }
                else if (!irq_enable) {
                    if (irq_asserted) {
                        irq_asserted = 0;
                        //nes.on_irq(false);
                        nes.on_irq(false);
                    }
                    irq_flag = 0;
                }
                break;
            case 0x11:
                output_counter = (value & 0x7F);
                break;
            case 0x12:
                sample_address = 0x4000 + value * 0x40;
                break;
            case 0x13:
                sample_length = value * 0x10 + 1;
                break;
        }
    }
 
    void ack_irq()
    {
        irq_flag = 0;
        if (irq_asserted) {
            nes.on_irq(false);
            irq_asserted = 0;
        }
    }
    int get_irq_flag()
    {
        return irq_flag ? 1 : 0;
    }
    void reset()
    {
        cycle = 1;
        silence = 1;
        output_shift = 0;
        output_counter = 0;
        sample_buffer_empty = 1;
        bits_remain = 1;
        duration = 0;
        enabled = 0;
        loop = 0;
        sample_address = 0;
        sample_length = 1;
        irq_enable = 0;
        irq_asserted = 0;
        irq_flag = 0;
        timer.reset();
    }

  private:
    static constexpr int freq_table[16] = {
        0x1AC, 0x17C, 0x154, 0x140, 0x11E, 0x0FE, 0x0E2, 0x0D6,
        0x0BE, 0x0A0, 0x08E, 0x080, 0x06A, 0x054, 0x048, 0x036
    };

    void fill_sample_buffer()
    {
        if (sample_buffer_empty && duration) {
            output_shift = nes.dmc_read(0x8000 + address);
            address = (address + 1) & 0x7FFF;
            sample_buffer_empty = 0;
            if (--duration == 0) {
                if (loop) {
                    address = sample_address;
                    duration = sample_length;
                }
                else {
                    if (irq_enable) {
                        irq_flag = 1;
                        if (irq_flag && !irq_asserted) {
                            //nes.on_irq(true);
                            nes.on_irq(true);
                            irq_asserted = 1;
                        }
                    }
                    else {
                        irq_flag = 0;
                    }
                }
            }
        }
    }
    int irq_enable;
    int loop;
    int freq_index;
    int dac;
    int sample_address;
    int sample_length;

    int dma_counter;
    int dma_remain;

    int sample_buffer;
    int sample_buffer_empty;

    int cycle;

    int silence;

    int output_shift;
    int output_counter;
    int bits_remain;

    int duration;
    int address;

    int enabled;

    int irq_flag;
    int irq_asserted;

    c_timer timer;
};
