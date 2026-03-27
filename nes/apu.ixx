module;

export module nes:apu;
import nemulator.std;
import :callbacks;
import :apu_channels;

import dsp;

namespace nes
{

constexpr int CLOCKS_PER_MIX = 1;
export constexpr float NES_AUDIO_RATE = (341.0f / 3.0f * 262.0f * 60.0f) / (float)CLOCKS_PER_MIX;

inline constexpr std::array<float, 203> tnd_lut = [] {
    std::array<float, 203> ret;
    for (int i = 0; i < 203; i++) {
        ret[i] = i == 0 ? 0.0f : (float)(163.67 / (24329.0 / i + 100));
    }
    return ret;
}();

export inline constexpr std::array<float, 31> square_lut = [] {
    std::array<float, 31> ret;
    for (int i = 0; i < 31; i++) {
        ret[i] = i == 0 ? 0.0f : (float)(95.52 / (8128.0 / i + 100));
    }
    return ret;
}();

export template <typename Nes>
class c_apu
{
    Nes &nes;
  public:
    c_apu(Nes &nes):
        nes(nes)
    {
        squares[0] = &square1;
        squares[1] = &square2;
        mixer_enabled = 1;
#ifdef AUDIO_LOG
        file.open("c:\\log\\audio.out", std::ios_base::trunc | std::ios_base::binary);
#endif
        resampler = resampler_t::create(NES_AUDIO_RATE / 48000.0f);
    }
    virtual ~c_apu()
    {
#ifdef AUDIO_LOG
        file.close();
#endif
    }
    unsigned char read_byte(unsigned short address)
    {
        //can only read 0x4015
        if (address == 0x4015) {
            int retval = 0;
            retval |= square1.get_status();
            retval |= (square2.get_status() << 1);
            retval |= (triangle.get_status() << 2);
            retval |= (noise.get_status() << 3);
            retval |= (dmc.get_status() << 4);
            retval |= (dmc.get_irq_flag() << 7);
            retval |= ((frame_irq_flag ? 1 : 0) << 6);

            if (frame_irq_flag) {
                frame_irq_flag = 0;
                if (frame_irq_asserted) {
                    nes.irq(false);
                    frame_irq_asserted = 0;
                }
            }

            return retval;
        }
        return reg[address - 0x4000];
    }

    void clock()
    {
        frame_seq_counter -= 12;
        if (frame_seq_counter < 0) {
            frame_seq_counter += CLOCKS_PER_FRAME_SEQ;
            clock_frame_seq();
        }
        clock_timers();
        if (++mix_clock == CLOCKS_PER_MIX) {
            mix_clock = 0;
            if (mixer_enabled) {
                mix();
            }
        }
    }

    void enable_mixer()
    {
        mixer_enabled = 1;
    }

    void disable_mixer()
    {
        mixer_enabled = 0;
    }

    void reset()
    {
        ticks = 0;
        frame_seq_counter = CLOCKS_PER_FRAME_SEQ;
        int x = sizeof(reg);
        for (int i = 0; i < sizeof(reg); i++)
            reg[i] = 0;
        frame_seq_step = 0;
        frame_seq_steps = 4;
        square1.set_sweep_mode(1);
        frame_irq_enable = 0;
        frame_irq_flag = 0;
        frame_irq_asserted = 0;
        square1.reset();
        square2.reset();
        triangle.reset();
        noise.reset();
        dmc.reset();
        square_clock = 0;
        mix_clock = 0;
    }

    void write_byte(unsigned short address, unsigned char value)
    {
        reg[address - 0x4000] = value;
        switch (address - 0x4000) {
            case 0x0:
            case 0x1:
            case 0x2:
            case 0x3:
                squares[(address & 0x4) >> 2]->write(address, value);
                break;
            case 0x4:
            case 0x5:
            case 0x6:
            case 0x7:
                squares[(address & 0x4) >> 2]->write(address, value);
                break;
            case 0x8:
            case 0xA:
            case 0xB:
                triangle.write(address, value);
                break;
            case 0xC:
            case 0xE:
            case 0xF:
                noise.write(address, value);
                break;
            case 0x10:
            case 0x11:
            case 0x12:
            case 0x13:
                dmc.write(address, value);
                break;
            case 0x15:
                if (value & 0x1)
                    square1.enable();
                else
                    square1.disable();
                if (value & 0x2)
                    square2.enable();
                else
                    square2.disable();
                if (value & 0x4)
                    triangle.enable();
                else
                    triangle.disable();
                if (value & 0x8)
                    noise.enable();
                else
                    noise.disable();
                if (value & 0x10)
                    dmc.enable();
                else
                    dmc.disable();
                dmc.ack_irq();
                break;
            case 0x17: {
                frame_irq_enable = !(value & 0x40);
                if (!frame_irq_enable && frame_irq_flag) {
                    if (frame_irq_asserted) {
                        nes.irq(false);
                        frame_irq_asserted = 0;
                    }
                }
                else if (frame_irq_enable && frame_irq_flag) {
                    if (!frame_irq_asserted) {
                        nes.irq(true);
                        frame_irq_asserted = 1;
                    }
                }
                frame_seq_step = 0;
                frame_seq_counter = CLOCKS_PER_FRAME_SEQ;
                if (value & 0x80) {
                    frame_seq_steps = 5;
                    clock_frame_seq();
                }
                else {
                    frame_seq_steps = 4;
                }
            } break;
            default:
                break;
        }
    }

    int get_buffer(const float **buf)
    {
        return resampler->get_output_buf(buf);
    }

    void clear_buffer()
    {
        resampler->clear_buf();
    }

    void set_audio_rate(double freq)
    {
        double x = NES_AUDIO_RATE / freq;
        resampler->set_m((float)x);
    }

  private:
    //1.79MHz sample rate
    using lpf_t = dsp::c_biquad4_t<
        0.5086284279823303f, 0.3313708603382111f, 0.1059221103787422f, 0.0055782101117074f,
        -1.9872593879699707f, -1.9750031232833862f, -1.8231037855148315f, -1.9900115728378296f,
        -1.9759204387664795f, -1.9602127075195313f, -1.9470522403717041f, -1.9888486862182617f,
        0.9801648259162903f, 0.9627774357795715f, 0.9480593800544739f, 0.9940192103385925f>;
    
    ////37Hz - 12kHz
    //using bpf_t = dsp::c_first_order_bandpass<0.5000000000000000f, 0.5000000000000000f, -0.0000000000000001f,
    //                                          0.9975842011460978f, -0.9975842011460978f, -0.9951684022921957f>;
    //

    //37Hz - 10kHz
    //using bpf_t = dsp::c_first_order_bandpass<0.4341737512063021f, 0.4341737512063021f, -0.1316524975873958f, 0.9975842011460978f, -0.9975842011460978f, -0.9951684022921957f>;
    //8kHz
    using bpf_t = dsp::c_first_order_bandpass<0.3660254037844386f, 0.3660254037844386f, -0.2679491924311228f,
                                              0.9975842011460978f, -0.9975842011460978f, -0.9951684022921957f>;
    
    using resampler_t = dsp::c_resampler<1, lpf_t, bpf_t>;
    std::unique_ptr<resampler_t> resampler;
    
    static const int CLOCKS_PER_FRAME_SEQ = 89489;
    int mixer_enabled;
    int mix_clock;
    int square_clock;
    int frame_irq_enable;
    int frame_irq_flag;
    int frame_irq_asserted;

    int ticks;
    int frame_seq_counter;
    unsigned char reg[24];
    int frame_seq_step;
    int frame_seq_steps;

    c_square square1;
    c_square square2;
    c_square *squares[2];
    c_triangle triangle;
    c_noise noise;
    c_dmc<Nes> dmc = c_dmc(nes);


    void mix()
    {
        int square_vol = square1.get_output() + square2.get_output();
        float square_out = square_lut[square_vol];

        int triangle_vol = triangle.get_output();
        int noise_vol = noise.get_output();
        int dmc_vol = dmc.get_output();
        float tnd_out = tnd_lut[3 * triangle_vol + 2 * noise_vol + dmc_vol];

        float sample = square_out + tnd_out;
        /*if (nes->mapper->has_expansion_audio())*/
        sample = nes.mix_mapper_audio(sample);
        resampler->process({sample});
    }

    void clock_timers()
    {
        if (square_clock) {
            square1.clock_timer();
            square2.clock_timer();
        }
        square_clock ^= 1;
        triangle.clock_timer();
        noise.clock_timer();
        dmc.clock_timer();
    }

    void clock_envelopes()
    {
        square1.clock_envelope();
        square2.clock_envelope();
        triangle.clock_linear();
        noise.clock_envelope();
    }

    void clock_length_sweep()
    {
        square1.clock_length_sweep();
        square2.clock_length_sweep();
        triangle.clock_length();
        noise.clock_length();
    }

    void clock_frame_seq()
    {
        if (frame_seq_steps == 5) {
            switch (frame_seq_step) {
                case 0:
                    clock_envelopes();
                    clock_length_sweep();
                    frame_seq_step = 1;
                    break;
                case 1:
                    clock_envelopes();
                    frame_seq_step = 2;
                    break;
                case 2:
                    clock_envelopes();
                    clock_length_sweep();
                    frame_seq_step = 3;
                    break;
                case 3:
                    clock_envelopes();
                    frame_seq_step = 4;
                    break;
                case 4:
                    frame_seq_step = 0;
                    break;
            }
            //frame_seq_step = ++frame_seq_step % 5;
        }
        else {
            switch (frame_seq_step) {
                case 0:
                    clock_envelopes();
                    frame_seq_step = 1;
                    break;
                case 1:
                    clock_envelopes();
                    clock_length_sweep();
                    frame_seq_step = 2;
                    break;
                case 2:
                    clock_envelopes();
                    frame_seq_step = 3;
                    break;
                case 3:
                    clock_envelopes();
                    clock_length_sweep();
                    //if irq flag, and !irq disable, execute interrupt
                    frame_irq_flag = 1;
                    if (frame_irq_enable && !frame_irq_asserted) {
                        frame_irq_asserted = 1;
                        nes.irq(true);
                    }
                    frame_seq_step = 0;
                    break;
            }
            //frame_seq_step = ++frame_seq_step & 0x3;
        }
    }


};

} //namespace nes