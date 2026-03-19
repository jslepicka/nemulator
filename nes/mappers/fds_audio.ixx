module;
#include <cassert>
export module nes:fds_audio;
import nemulator.std;
import dsp;
import :apu; //for NES_AUDIO_RATE

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
        lowpass.reset();
        op.master_speed = 0xFF;
    }

    void clock()
    {
        uint32_t clock_before = clock_divider;
        clock_divider++;
        uint32_t clock_xor = clock_before ^ clock_divider;
        if (clock_xor & 0x8) {
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

            if (clock_xor & 0x10) {
                if (mod.clock_mod()) {
                    mod.update_output(freq);
                }

                update_output();
                if (!wave.halt) {
                    uint32_t counter_before = wave.counter;
                    wave.counter += mod.output;
                    wave.index = ((wave.counter >> 18) & 0x3F);
                }
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

        output = sample * volume[master_vol];
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
                    wave.counter = 0;
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
                    mod.mod_accum &= ~0x1FFF;
                }
                mod.reload_period();
                break;
            case 0x4088: //mod table write
                if (!mod.disabled) {
                    return;
                }
                mod.data[(mod.mod_accum >> 13) & 0x1F] = value & 0x7;
                mod.mod_accum = (mod.mod_accum + 0x2000) & 0x3FFFF;
                break;
            case 0x4089: //wav write / master volume
                master_vol = value & 0x3;
                wave.writable = value & 0x80;
                break;
            case 0x408A: //env speed
                op.master_speed = value;
                mod.master_speed = value;
                break;
            default:
                assert(0);
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
                break;
            default:
                break;
        }
        assert(0);
        return 0;
    }

    float get_sample()
    {
        return lowpass.process(!enabled ? 0.0f : output);
    }

    void enable(int value)
    {
        enabled = value;
    }

  private:
    bool enabled;
    bool envelopes_enabled;
    uint8_t master_vol;
    uint32_t clock_divider;
    float output;

    static constexpr float fs = c_apu::NES_AUDIO_RATE;
    static constexpr float fc = 2190.0f;
    dsp::c_one_pole_lowpass<fs, fc> lowpass;

    struct s_operator
    {
        bool clock_envelope();
        void reload_period();

        uint8_t master_speed;
        uint8_t speed;
        uint8_t gain;
        uint8_t direction;
        uint8_t envelope_enabled;
        uint16_t frequency;
        uint32_t period;
        uint32_t accum;
    } op;

    struct s_modulator : s_operator
    {
        bool enabled();
        bool clock_mod();
        void update_output(uint16_t pitch);
        void update_counter(int8_t value);

        bool disabled;
        int8_t counter;
        uint32_t mod_accum;
        int32_t output;
        uint8_t data[32];
        uint8_t index;

    } mod;

    struct s_waveform
    {
        bool halt;
        bool writable;
        uint8_t data[64];
        uint8_t index;
        uint32_t counter;
    } wave;

};


bool c_fds_audio::s_operator::clock_envelope()
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

void c_fds_audio::s_operator::reload_period()
{
    period = (1 + speed) * (master_speed + 1);
}

bool c_fds_audio::s_modulator::clock_mod()
{
    if (enabled()) {
        uint32_t accum_before = mod_accum;
        mod_accum = (mod_accum + frequency) & 0x3FFFF;
        if ((mod_accum ^ accum_before) & 0b1'0000'0000'0000) {
            static const int8_t lookup[8] = {0, 1, 2, 4, -8, -4, -2, -1};
            int32_t offset = lookup[data[index]];
            update_counter(offset == -8 ? 0 : counter + offset);
            index = (mod_accum >> 13) & 0x1F;
            return true;
        }
    }
    return false;
}

void c_fds_audio::s_modulator::update_output(uint16_t pitch)
{
    int32_t temp = counter * gain;
    if ((temp & 0x0F) && !(temp & 0x800)) {
        temp += 0x20;
    }
    temp += 0x400;
    temp = (temp >> 4) & 0xFF;
    output = (pitch * temp) & 0xFFFFF;
}

void c_fds_audio::s_modulator::update_counter(int8_t value)
{
    counter = value;
    if (counter >= 64) {
        counter -= 128;
    }
    else if (counter < -64) {
        counter += 128;
    }
}

bool c_fds_audio::s_modulator::enabled()
{
    return !disabled && frequency > 0;
}

} //namespace nes