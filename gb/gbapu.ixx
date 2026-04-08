module;

export module gb:apu;
import nemulator.std;
import :apu_channels;

import dsp;

namespace gb
{
export class c_gb;

class c_gbapu
{
  public:
    c_gbapu()
    {
        mixer_enabled = 0;
        resampler = resampler_t::create((float)(GB_AUDIO_RATE / 48000.0));
    }

    void reset()
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

    void clock()
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

    void write_byte(uint16_t address, uint8_t data)
    {
        int x = 1;
        registers[address & 0x3F] = data;
        if (!(NR52 & 0x80) && address != 0xFF26) {
            //NR52 controls power to the sound hardware. When powered off, all registers (NR10-NR51) are instantly
            //written with zero and any writes to those registers are ignored while power remains off (except on the
            //DMG, where length counters are unaffected by power and can still be written while off)
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

    uint8_t read_byte(uint16_t address)
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

    void mix()
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

        left_sample = ((square1_dac & enable_1_l) ? square1_out : 7.5f) +
                      ((square2_dac & enable_2_l) ? square2_out : 7.5f) + ((wave_dac & enable_w_l) ? wave_out : 7.5f) +
                      ((noise_dac & enable_n_l) ? noise_out : 7.5f);

        right_sample = ((square1_dac & enable_1_r) ? square1_out : 7.5f) +
                       ((square2_dac & enable_2_r) ? square2_out : 7.5f) + ((wave_dac & enable_w_r) ? wave_out : 7.5f) +
                       ((noise_dac & enable_n_r) ? noise_out : 7.5f);

        //divide by 60 (max 15/channel * 4), then normalize to -1..1
        left_sample = (left_sample / 60.0f) * 2.0f - 1.0f;
        right_sample = (right_sample / 60.0f) * 2.0f - 1.0f;

        right_sample *= right_vol;
        left_sample *= left_vol;

        resampler->process({left_sample, right_sample});
    }

    void enable_mixer()
    {
        mixer_enabled = 1;
    }

    void disable_mixer()
    {
        mixer_enabled = 0;
    }

    int get_buffer(const float **buf)
    {
        return resampler->get_output_buf(buf);
    }

    void clear_buffers()
    {
        resampler->clear_buf();
    }

    void set_audio_rate(double freq)
    {
        double x = GB_AUDIO_RATE / freq;
        resampler->set_m((float)x);
    }

  private:
    void power_on()
    {
        frame_seq_step = 0;
        frame_seq_counter = CLOCKS_PER_FRAME_SEQ;
        square1.power_on();
        square2.power_on();
        noise.power_on();
        wave.power_on();
    }

    void power_off()
    {
        for (int i = 0xFF10; i <= 0xFF25; i++) {
            write_byte(i, 0);
        }
    }


  private:
    using lpf_t = dsp::c_biquad4_t<
        0.5071556568145752f,0.3305769860744476f,0.1126436218619347f,0.0055792131461203f,
       -1.9635004997253418f,-1.9287968873977661f,-1.5293598175048828f,-1.9713480472564697f,
       -1.9545004367828369f,-1.9304054975509644f,-1.9105833768844604f,-1.9750583171844482f,
        0.9666246175765991f,0.9376937150955200f,0.9134329557418823f,0.9898924231529236f>;

    using bpf_t = dsp::c_first_order_bandpass<>;
    using resampler_t = dsp::c_resampler<2, lpf_t, bpf_t>;

    std::unique_ptr<resampler_t> resampler;

    int mixer_enabled;
    int ticks;
    int frame_seq_counter;
    int frame_seq_step;
    static const int CLOCKS_PER_FRAME_SEQ = 8192 / 2;
    uint8_t NR50;
    uint8_t NR51;
    uint8_t NR52;
    float left_vol;
    float right_vol;

    int enable_n_l;
    int enable_n_r;
    int enable_w_l;
    int enable_w_r;
    int enable_2_l;
    int enable_2_r;
    int enable_1_l;
    int enable_1_r;

    uint32_t registers[64];
    uint32_t tick;

    static constexpr double GB_AUDIO_RATE = (456.0f * 154.0f * 60.0f) / 4.0f;

    c_square square1;
    c_square square2;
    c_noise noise;
    c_wave wave;
};

} //namespace gb