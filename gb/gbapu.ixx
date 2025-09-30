module;

export module gb:apu;
import nemulator.std;

import dsp;

namespace gb
{
export class c_gb;

class c_gbapu
{
  public:
    c_gbapu(c_gb *gb);
    ~c_gbapu();
    void reset();
    void clock();
    void write_byte(uint16_t address, uint8_t data);
    uint8_t read_byte(uint16_t data);
    void mix();
    void enable_mixer();
    void disable_mixer();
    int get_buffers(const float **buf_l, const float **buf_r);
    void clear_buffers();
    void set_audio_rate(double freq);

  private:
    c_gb *gb;
    //c_resampler* resampler_l;
    //c_resampler* resampler_r;
    //c_biquad4* lpf_l;
    //c_biquad* post_filter_l;
    //c_biquad4* lpf_r;
    //c_biquad* post_filter_r;

    using lpf_t = dsp::c_biquad4_t<
        0.5071556568145752f,0.3305769860744476f,0.1126436218619347f,0.0055792131461203f,
       -1.9635004997253418f,-1.9287968873977661f,-1.5293598175048828f,-1.9713480472564697f,
       -1.9545004367828369f,-1.9304054975509644f,-1.9105833768844604f,-1.9750583171844482f,
        0.9666246175765991f,0.9376937150955200f,0.9134329557418823f,0.9898924231529236f>;

       // using lpf_t = dsp::c_biquad4_parallel<
       // -0.0104663218473384f,0.0412466846458584f,-0.0611703577312065f,0.0303667988016696f,
       //0.0109635994428656f,-0.0394608897575032f,0.0538673291681218f,-0.0252099492143820f,
       //-1.9750584980742110f,-1.9544997755414977f,-1.9304062150951757f,-1.9105830852444732f,
       // 0.9898926865980721f,0.9666239762788166f,0.9376943597913852f,0.9134327287044153f,
       // 0.0001285611844049f>;

    std::unique_ptr<dsp::c_resampler> resampler_l;
    std::unique_ptr<dsp::c_resampler> resampler_r;
    std::unique_ptr<lpf_t> lpf_l;
    std::unique_ptr<dsp::c_first_order_bandpass> post_filter_l;
    std::unique_ptr<lpf_t> lpf_r;
    std::unique_ptr<dsp::c_first_order_bandpass> post_filter_r;
    std::unique_ptr<int32_t[]> sound_buffer;

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

    void power_on();
    void power_off();

    uint32_t registers[64];
    uint32_t tick;

    static const double GB_AUDIO_RATE;

    class c_timer
    {
      public:
        c_timer();
        ~c_timer();
        void reset();
        void set_period(int period);
        int clock();

      private:
        int counter;
        int period;
        int mask;
    };

    class c_duty
    {
      public:
        c_duty();
        ~c_duty();
        int get_output();
        void set_duty_cycle(int duty);
        void clock();
        void reset();
        void reset_step();

      private:
        static const int duty_cycle_table[4][8];
        int duty_cycle;
        int step;
    };

    class c_length
    {
      public:
        c_length();
        ~c_length();
        int get_output();
        void set_length(int length);
        void set_enable(int enable);
        void reset();
        int clock();
        int counter;

      private:
        int enabled;
    };

    class c_envelope
    {
      public:
        c_envelope();
        ~c_envelope();
        void reset();
        int get_output();
        void set_volume(int volume);
        void set_period(int period);
        void set_mode(int mode);
        void clock();

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
        c_square();
        ~c_square();
        void clock_timer();
        void clock_length();
        void clock_sweep();
        void clock_envelope();
        int get_output();
        void write(int reg, uint8_t data);
        void reset();
        void power_on();
        void trigger();
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
        int calc_sweep();
        int clock_divider;
    };

    class c_noise
    {
      public:
        c_noise();
        ~c_noise();
        void reset();
        int get_output();
        void trigger();
        void write(uint16_t address, uint8_t data);
        void clock_timer();
        void clock_length();
        void clock_envelope();
        void power_on();
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
        c_wave();
        ~c_wave();
        void reset();
        void clock();
        int get_output();
        void trigger();
        void write(uint16_t address, uint8_t data);
        void clock_timer();
        void clock_length();
        void power_on();
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

    c_square square1;
    c_square square2;
    c_noise noise;
    c_wave wave;
};

} //namespace gb