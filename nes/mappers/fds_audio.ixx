module;

export module nes:fds_audio;
import nemulator.std;

namespace nes
{
class c_fds_audio
{
  public:
    c_fds_audio()
    {
    }

    ~c_fds_audio()
    {
    }

    void reset()
    {
        enabled = false;
        envelopes_enabled = false;
        master_vol = 0;
        clock_divider = 0;
        output = 0.0f;
        std::memset(&op, 0, sizeof(op));
        std::memset(&mod, 0, sizeof(mod));
        std::memset(&wave, 0, sizeof(wave));
    }

    void clock()
    {
        if (!enabled) {
            return;
        }
        uint32_t freq = op.frequency;
        if (envelopes_enabled && !wave.halt) {
            op.clock_envelope();
            if (mod.clock_envelope()) {
                mod.update_output(freq);
            }
        }

        if (mod.clock_mod()) {
            mod.update_output(freq);
        }

        update_output();
        if (!wave.halt && freq + mod.output > 0) {
            wave.overflow += freq + mod.output;
            if (wave.overflow < freq + mod.output) {
                wave.index = (wave.index + 1) & 0x3F;
            }
        }
    }

    void update_output()
    {
        if (wave.writable) {
            return;
        }

        uint8_t g = std::min(op.gain, (uint8_t)32);
        float sample = (float)((uint32_t)wave.data[wave.index] * g) / (63.0f * 32.0f);
        const float volume[4] = {1.0f, 2.0f / 3.0f, 2.0f / 4.0f, 2.0f / 5.0f};

        output = sample * volume[master_vol & 0x3];
    }

    void write_byte(uint16_t address, uint8_t value)
    {
        if (!enabled) {
            return;
        }
        //ods("fds audio write: %04X = %02X\n", address, value);
        if (address >= 0x4040 && address < 0x4080) {
            if (wave.writable) {
                wave.data[address - 0x4040] = value & 0x3F;
            }
            return;
        }
        int x;
        switch (address) {
            case 0x4080: //vol envelope
                op.speed = value & 0x3F;
                op.direction = value & 0x40;
                op.envelope_enabled = !(value & 0x80);
                if (!op.envelope_enabled) {
                    op.gain = op.speed;
                }
                op.reload_period();
                break;
            case 0x4082: //freq low
                op.frequency = (op.frequency & 0x0F00) | value;
                mod.update_output(op.frequency);
                break;
            case 0x4083: //freq high
                op.frequency = (op.frequency & 0xFF) | ((value & 0xF) << 8);
                envelopes_enabled = !(value & 0x40);
                wave.halt = value & 0x80;
                if (wave.halt) {
                    wave.index = 0;
                }
                if (!envelopes_enabled) {
                    op.reload_period();
                    mod.reload_period();
                }
                mod.update_output(op.frequency);
                break;
            case 0x4084: //mod envelope
                mod.speed = value & 0x3F;
                mod.direction = value & 0x40;
                mod.envelope_enabled = !(value & 0x80);
                if (!mod.envelope_enabled) {
                    mod.gain = mod.speed;
                }
                mod.reload_period();
                mod.update_output(op.frequency);
                break;
            case 0x4085: //mod counter
                mod.update_counter(value & 0x7F);
                mod.update_output(op.frequency);
                break;
            case 0x4086: //mod freq low
                mod.frequency = (mod.frequency & 0x0F00) | value;
                break;
            case 0x4087: //mod freq high
                mod.frequency = (mod.frequency & 0xFF) | ((value & 0xF) << 8);
                mod.disabled = value & 0x80;
                if (mod.disabled) {
                    mod.overflow = 0;
                }
                mod.reload_period();
                break;
            case 0x4088: //mod table write
                if (!mod.disabled) {
                    return;
                }
                mod.data[mod.index] = value & 0x7;
                mod.index = (mod.index + 1) & 0x3F;
                mod.data[mod.index] = value & 0x7;
                mod.index = (mod.index + 1) & 0x3F;
                break;
            case 0x4089: //wav write / master volume
                master_vol = value & 0x3;
                wave.writable = value & 0x80;
                break;
            case 0x408A: //env speed
                op.master_speed = value;
                mod.master_speed = value;
                break;
            case 0x4090: //vol gain
                break;
            case 0x4091: //wave accumulator
                break;
            case 0x4092: //mod gain
                break;
            case 0x4093: //mod table address accumulator
                break;
            case 0x4094: //mod counter * gain result
                break;
            case 0x4095: //mod counter increment
                break;
            case 0x4096: //wavetable value
                break;
            case 0x4097: //mod counter value
                x = 1;
                break;
            default:
                break;
        }
    }

    uint8_t read_byte(uint16_t address)
    {
        if (!enabled) {
            return 0; //what should this return?
        }
        if (address >= 0x4040 && address < 0x4080) {
            return wave.data[address - 0x4040];
        }
        switch (address) {
            case 0x4080: //vol envelope
                break;
            case 0x4082: //freq low
                break;
            case 0x4083: //freq high
                break;
            case 0x4084: //mod envelope
                break;
            case 0x4085: //mod counter
                break;
            case 0x4086: //mod freq low
                break;
            case 0x4087: //mod freq high
                break;
            case 0x4088: //mod table write
                break;
            case 0x4089: //wav write / master volume
                break;
            case 0x408A: //env speed
                break;
            case 0x4090: //vol gain
                return op.gain;
                break;
            case 0x4091: //wave accumulator
                break;
            case 0x4092: //mod gain
                return mod.gain;
                break;
            case 0x4093: //mod table address accumulator
                break;
            case 0x4094: //mod counter * gain result
                break;
            case 0x4095: //mod counter increment
                break;
            case 0x4096: //wavetable value
                break;
            case 0x4097: //mod counter value
                break;
            default:
                break;
        }
        return 0;
    }

    float get_sample()
    {
        if (!enabled) {
            return 0.0f;
        }
        return output;
    }

    void enable(int value)
    {
        enabled = value;
    }

    //operator

  private:
    bool enabled;
    bool envelopes_enabled;
    uint8_t master_vol;
    uint32_t clock_divider;
    float output;

    struct s_operator
    {
        bool clock_envelope();
        void reload_period();

        uint8_t master_speed = 0xFF;
        uint8_t speed;
        uint8_t gain;
        uint8_t direction;
        uint8_t envelope_enabled;
        uint16_t frequency;
        uint32_t period;
    } op;

    struct s_modulator : s_operator
    {
        bool enabled();
        bool clock_mod();
        void update_output(uint16_t pitch);
        void update_counter(int8_t value);

        bool disabled;
        int8_t counter;
        uint16_t overflow;
        int32_t output;

        uint8_t data[64];
        uint8_t index;

    } mod;

    struct s_waveform
    {
        bool halt;
        bool writable;
        uint16_t overflow;
        uint8_t data[64];
        uint8_t index;
    } wave;

};


bool nes::c_fds_audio::s_operator::clock_envelope()
{
    if (envelope_enabled && master_speed > 0 && !--period) {
        reload_period();
        if (direction) {
            if (gain < 32) {
                gain++;
            }
        }
        else {
            if (gain > 0) {
                gain--;
            }
        }
        return true;
    }
    return false;
}

void nes::c_fds_audio::s_operator::reload_period()
{
    period = 8 * (1 + speed) * master_speed;
}

bool nes::c_fds_audio::s_modulator::clock_mod()
{
    if (enabled()) {
        overflow += frequency;
        if (overflow < frequency) {
            static const int8_t lookup[8] = {0, 1, 2, 4, -8, -4, -2, -1};
            int32_t offset = lookup[data[index]];
            update_counter(offset == -8 ? 0 : counter + offset);
            index = (index + 1) & 0x3F;
            return true;
        }
    }
    return false;
}

void nes::c_fds_audio::s_modulator::update_output(uint16_t pitch)
{
    int32_t temp = counter * gain;
    int32_t remainder = temp & 0xF;
    temp >>= 4;
    if (remainder > 0 && !(temp & 0x80)) {
        temp += counter < 0 ? -1 : 2;
    }

    if (temp >= 192) {
        temp -= 256;
    }
    else if (temp < -64) {
        temp += 256;
    }

    temp *= pitch;
    remainder = temp & 0x3F;
    temp >>= 6;
    if (remainder >= 32) {
        temp++;
    }
    output = temp;
}

void nes::c_fds_audio::s_modulator::update_counter(int8_t value)
{
    counter = value;
    if (counter >= 64) {
        counter -= 128;
    }
    else if (counter < -64) {
        counter += 128;
    }
}

bool nes::c_fds_audio::s_modulator::enabled()
{
    return !disabled && frequency > 0;
}

} //namespace nes