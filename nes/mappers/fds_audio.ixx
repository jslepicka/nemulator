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
        std::memset(wavetable_ram, 0, sizeof(wavetable_ram));
        std::memset(mod_table, 0, sizeof(mod_table));
        mod_table_pos = 0;
        vol_envelope = 0;
        freq_low = 0;
        freq_high = 0;
        mod_envelope = 0;
        mod_counter = 0;
        mod_freq_low = 0;
        mod_freq_high = 0;
        mod_table_write = 0;
        master_vol = 0;
        env_speed = 0;
        vol_gain = 0;
        wav_accumulator = 0;
        mod_gain = 0;
        mod_accumulator = 0;
        mod_result = 0;
        mod_increment = 0;
        wav_value = 0;
        mod_value = 0;

        wav_clock = 0;

        _wav_accum = 0;
        _mod_accum = 0;

        output = 0;
        clock_divider = 0;

        vol_envelope_counter = 0;
        vol_envelope_reload = 0;
        _next_vol_gain = 0;
        _vol_gain = 0;

        mod_envelope_counter = 0;
        mod_envelope_reload = 0;
        _next_mod_gain = 0;
        _mod_gain = 0;
    }

    void clock()
    {
        if (++clock_divider == 3) {
            clock_divider = 0;
            if (++wav_clock == 16) {
                wav_clock = 0;

                //envelopes

                if (env_speed) {
                    if (vol_envelope_counter && --vol_envelope_counter == 0) {
                        if (vol_envelope & 0x40) {
                            if (_next_vol_gain < 32) {
                                _next_vol_gain += 1;
                            }
                        }
                        else {
                            if (_next_vol_gain > 0) {
                                _next_vol_gain -= 1;
                            }
                        }
                        vol_envelope_counter = vol_envelope_reload;
                    }

                    if (mod_envelope_counter && --mod_envelope_counter == 0) {
                        if (mod_envelope & 0x40) {
                            if (_mod_gain < 32) {
                                _mod_gain += 1;
                            }
                        }
                        else {
                            if (_mod_gain > 0) {
                                _mod_gain -= 1;
                            }
                        }
                        mod_envelope_counter = mod_envelope_reload;
                    }
                }

                if (!(mod_freq_high & 0x80)) {
                    _mod_accum += mod_freq_low | ((mod_freq_high & 0xF) << 8);
                }

                uint8_t mi = (_mod_accum >> 13) & 0x1F;
                if (mi == 0) {
                    _vol_gain = _next_vol_gain;
                }

                uint8_t mod_index = mod_table[(_mod_accum >> 13) & 0x1F];
                const int8_t mod_values[8] = {0, 1, 2, 4, 0, -4, -2, -1};
                int8_t c = mod_values[mod_index];
                mod_counter += c;
                if (mod_counter > 63) {
                    mod_counter -= 128;
                }
                else if (mod_counter < -64) {
                    mod_counter += 128;
                }
                //modulation
                int32_t temp = mod_counter * _mod_gain;
                if ((temp & 0x0f) && !(temp & 0x800))
                    temp += 0x20;
                temp += 0x400;
                temp = (temp >> 4) & 0xff;

                uint32_t pitch = freq_low | ((freq_high & 0xF) << 8);
                uint32_t wave_pitch = (pitch * temp) & 0xFFFFF;

                //wave output
                _wav_accum += wave_pitch;
                if (!(master_vol & 0x80)) {
                    

                    output = wavetable_ram[(_wav_accum >> 18) & 0x3F];
                }

            
            }
        }
    }

    void write_byte(uint16_t address, uint8_t value)
    {
        ods("fds audio write: %04X = %02X\n", address, value);
        if (address >= 0x4040 && address < 0x4080) {
            if (master_vol & 0x80) {
                wavetable_ram[address - 0x4040] = value;
            }
            return;
        }
        int x;
        switch (address) {
            case 0x4080: //vol envelope
                vol_envelope = value;
                if (vol_envelope & 0x80) {
                    int x = 1;
                    if ((value & 0x3F) == 0) {
                        _vol_gain = 0;
                    }
                    _next_vol_gain = value & 0x3F;
                }
                vol_envelope_reload = 8 * ((vol_envelope & 0x3F) + 1) * (env_speed + 1);
                vol_envelope_counter = vol_envelope_reload;
                break;
            case 0x4082: //freq low
                freq_low = value;
                break;
            case 0x4083: //freq high
                freq_high = value;
                break;
            case 0x4084: //mod envelope
                mod_envelope = value;
                if (mod_envelope & 0x80) {
                    int x = 1;
                    if ((value & 0x3F) == 0) {
                        _mod_gain = 0;
                    }
                    _next_mod_gain = value & 0x3F;
                    //todo: does this get set immediately?
                    _mod_gain = _next_mod_gain;
                }
                mod_envelope_reload = 8 * ((mod_envelope & 0x3F) + 1) * (env_speed + 1);
                mod_envelope_counter = mod_envelope_reload;
                break;
            case 0x4085: //mod counter
                mod_counter = (value & 0x7F);
                if (mod_counter & 0x40) {
                    mod_counter |= 0x80;
                }
                break;
            case 0x4086: //mod freq low
                mod_freq_low = value;
                break;
            case 0x4087: //mod freq high
                mod_freq_high = value;
                break;
            case 0x4088: //mod table write
                if (mod_freq_high & 0x80) {
                    mod_table[mod_table_pos] = value & 7;
                    mod_table_pos = (mod_table_pos + 1) % 32;
                }
                //todo: Writing $4088 also increments the address (bits 13-17 of wave accumulator) when $4087.7=1.
                break;
            case 0x4089: //wav write / master volume
                master_vol = value;
                break;
            case 0x408A: //env speed
                env_speed = value;
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
        if (address >= 0x4040 && address < 0x4080) {
            return 0x40 | wavetable_ram[address - 0x4040];
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
        return 0;
    }

    float get_sample()
    {
        if (_vol_gain > 32) {
            _vol_gain = 32;
        }
        float sample = (float)((uint32_t)output * _vol_gain) / (63.0f * 63.0f);
        const float volume[4] = {1.0f, 2.0f / 3.0f, 2.0f / 4.0f, 2.0f / 5.0f};
        
        return sample * volume[master_vol & 0x3];
    }

    void enable(int value)
    {
        enabled = value;
    }

  private:
    int enabled = 0;
    uint8_t wavetable_ram[64];
    uint8_t vol_envelope;
    uint8_t freq_low;
    uint8_t freq_high;
    uint8_t mod_envelope;
    int8_t mod_counter;
    uint8_t mod_freq_low;
    uint8_t mod_freq_high;
    uint8_t mod_table_write;
    uint8_t master_vol;
    uint8_t env_speed;
    uint8_t vol_gain;
    uint8_t wav_accumulator;
    uint8_t mod_gain;
    uint8_t mod_accumulator;
    uint8_t mod_result;
    uint8_t mod_increment;
    uint8_t wav_value;
    uint8_t mod_value;
    uint8_t wav_clock;
    uint32_t _wav_accum;
    uint32_t _mod_accum;
    uint8_t output;

    uint8_t mod_table[32];
    uint8_t mod_table_pos;

    int clock_divider;

    uint32_t vol_envelope_counter;
    uint32_t vol_envelope_reload;
    uint32_t _next_vol_gain;
    uint32_t _vol_gain;

    uint32_t mod_envelope_counter;
    uint32_t mod_envelope_reload;
    uint32_t _next_mod_gain;
    uint32_t _mod_gain;

    uint32_t _mod_counter;



};
} //namespace nes