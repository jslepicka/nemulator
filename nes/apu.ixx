module;

export module nes:apu;
import nemulator.std;

import dsp;

namespace nes
{

export class c_nes;
export class c_mapper;

export class c_apu
{
  public:
    c_apu();
    virtual ~c_apu();
    void reset();
    void write_byte(unsigned short address, unsigned char value);
    unsigned char read_byte(unsigned short address);
    void clock(int cycles);
    void clock_once();
    void set_nes(c_nes *nes);
    void enable_mixer();
    void disable_mixer();
    int get_buffer(const float **buf);
    void clear_buffer();
    void set_audio_rate(double freq);

    //mmc5 needs to read this
    static float square_lut[31];

    class c_envelope
    {
      public:
        c_envelope();
        ~c_envelope();
        void clock();
        void write(unsigned char value);
        void reset_counter();
        void reset();
        int get_output();

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
        c_timer();
        ~c_timer();
        //void clock(int shift);
        int clock();
        //int clock2x();
        void set_period_lo(int period_lo);
        void set_period_hi(int period_hi);
        void set_period(int value);
        int get_period();
        void reset();

      private:
        int shift;
        int counter;
        int period;
        //int output;
    };

    class c_sequencer
    {
      public:
        c_sequencer();
        ~c_sequencer();
        void clock();
        void set_duty_cycle(int cycle);
        int get_output();
        void reset_step();
        void reset();

      private:
        static const int duty_cycle_table[4][8];
        int duty_cycle;
        int step;
        int output;
        int ticks;
    };

    class c_length
    {
      public:
        c_length();
        ~c_length();
        void clock();
        void set_length(int index);
        void set_halt(int halt);
        int get_counter();
        int get_output();
        void reset();
        int get_halt();
        void enable();
        void disable();

      private:
        int enabled;
        int counter;
        int halt;
        int length;
        static const int length_table[32];
    };

    class c_square
    {
      public:
        c_square();
        ~c_square();
        void clock_envelope();
        void clock_length();
        void clock_length_sweep();
        void clock_timer();
        int get_output();
        int get_output_mmc5();
        int get_status();
        void enable();
        void disable();
        void write(unsigned short address, unsigned char value);
        void set_sweep_mode(int mode);
        void reset();

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
        void update_sweep_silencing();
        c_envelope envelope;
        c_timer timer;
        c_sequencer sequencer;
        c_length length;
    };

    class c_triangle
    {
      public:
        c_triangle();
        ~c_triangle();
        void clock_timer();
        void clock_length();
        void clock_linear();
        void enable();
        void disable();
        int get_status();
        int get_output();
        void write(unsigned short address, unsigned char value);
        void reset();

      private:
        static const int sequence[32];
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
        c_noise();
        ~c_noise();
        void clock_timer();
        void clock_length();
        void clock_envelope();
        void write(unsigned short address, unsigned char value);
        int get_output();
        void enable();
        void disable();
        int get_status();
        void reset();

      private:
        int lfsr;
        static const int random_period_table[16];
        int output;
        c_length length;
        c_envelope envelope;
        c_timer timer;
        int lfsr_shift;
        int random_period;
        int random_counter;
        int length_enabled;
    };

    class c_dmc
    {
      public:
        c_dmc();
        ~c_dmc();
        int get_output();
        void enable();
        void disable();
        int get_status();
        void clock_timer();
        void write(unsigned short address, unsigned char value);
        void set_nes(c_nes *nes);
        void ack_irq();
        int get_irq_flag();
        void reset();

      private:
        c_nes *nes;
        static const int freq_table[16];
        void fill_sample_buffer();
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

  private:
    static const float NES_AUDIO_RATE;

    using lpf_t = dsp::c_biquad4_t<
        0.5086284279823303f, 0.3313708603382111f, 0.1059221103787422f, 0.0055782101117074f,
        -1.9872593879699707f, -1.9750031232833862f, -1.8231037855148315f, -1.9900115728378296f,
        -1.9759204387664795f, -1.9602127075195313f, -1.9470522403717041f, -1.9888486862182617f,
        0.9801648259162903f, 0.9627774357795715f, 0.9480593800544739f, 0.9940192103385925f>;
    using bpf_t = dsp::c_first_order_bandpass<0.5000000000000000f, 0.5000000000000000f, -0.0000000000000001f,
                                            0.9998691174378402f, -0.9998691174378402f, -0.9997382348756805f>;
    using resampler_t = dsp::c_resampler2<1, lpf_t, bpf_t>;
    std::unique_ptr<resampler_t> resampler;
    
    static const int CLOCKS_PER_FRAME_SEQ = 89489;
    int mixer_enabled;
    int square_clock;
    c_nes *nes;
    c_mapper *mapper;
    int frame_irq_enable;
    int frame_irq_flag;
    int frame_irq_asserted;
    int mix_external_audio;
    float *external_audio;

    int ticks;
    int frame_seq_counter;
    void clock_frame_seq();
    void clock_timers();

    void clock_envelopes();
    void clock_length_sweep();

    void mix();

    void build_lookup_tables();

    unsigned char reg[24];
    int frame_seq_step;
    int frame_seq_steps;

    c_square square1;
    c_square square2;
    c_square *squares[2];
    c_triangle triangle;
    c_noise noise;
    c_dmc dmc;

    static std::atomic<int> lookup_tables_built;
    static float tnd_lut[203];
};

} //namespace nes