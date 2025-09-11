module;

export module ym2612;
import nemulator.std;

enum class ADSR_PHASE
{
    ATTACK,
    DECAY,
    SUSTAIN,
    RELEASE
};

uint32_t compute_key_code(uint32_t f_number, uint8_t block);

class c_envelope_generator
{
  public:
    void reset();
    bool clock();
    void key(bool key_on);
    ADSR_PHASE phase;


    uint32_t get_output();
    uint32_t sustain_level;
    uint32_t attack_rate;
    uint32_t decay_rate;
    uint32_t sustain_rate;
    uint32_t release_rate;
    uint32_t total_level;
    uint32_t rate_scaling;
    uint32_t key_scale;
    uint32_t key_scale_rate;

    bool ssg_enabled;
    bool ssg_attack;
    bool ssg_alternate;
    bool ssg_hold;
    bool ssg_invert_output;

    bool ssg_clock();

  private:
    uint32_t divider;
    uint32_t cycle_count;
    uint32_t attenuation;

    const uint8_t attenuation_increments[64][8] = {
        {0, 0, 0, 0, 0, 0, 0, 0},   { 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0, 1, 0, 1, 0, 1, 0, 1 }, { 0, 1, 0, 1, 0, 1, 0, 1 },  // 0-3
        { 0, 1, 0, 1, 0, 1, 0, 1 }, { 0, 1, 0, 1, 0, 1, 0, 1 },
        { 0, 1, 1, 1, 0, 1, 1, 1 }, { 0, 1, 1, 1, 0, 1, 1, 1 },  // 4-7
        { 0, 1, 0, 1, 0, 1, 0, 1 }, { 0, 1, 0, 1, 1, 1, 0, 1 },
        { 0, 1, 1, 1, 0, 1, 1, 1 }, { 0, 1, 1, 1, 1, 1, 1, 1 },  // 8-11
        { 0, 1, 0, 1, 0, 1, 0, 1 }, { 0, 1, 0, 1, 1, 1, 0, 1 },
        { 0, 1, 1, 1, 0, 1, 1, 1 }, { 0, 1, 1, 1, 1, 1, 1, 1 },  // 12-15
        { 0, 1, 0, 1, 0, 1, 0, 1 }, { 0, 1, 0, 1, 1, 1, 0, 1 },
        { 0, 1, 1, 1, 0, 1, 1, 1 }, { 0, 1, 1, 1, 1, 1, 1, 1 },  // 16-19
        { 0, 1, 0, 1, 0, 1, 0, 1 }, { 0, 1, 0, 1, 1, 1, 0, 1 },
        { 0, 1, 1, 1, 0, 1, 1, 1 }, { 0, 1, 1, 1, 1, 1, 1, 1 },  // 20-23
        { 0, 1, 0, 1, 0, 1, 0, 1 }, { 0, 1, 0, 1, 1, 1, 0, 1 },
        { 0, 1, 1, 1, 0, 1, 1, 1 }, { 0, 1, 1, 1, 1, 1, 1, 1 },  // 24-27
        { 0, 1, 0, 1, 0, 1, 0, 1 }, { 0, 1, 0, 1, 1, 1, 0, 1 },
        { 0, 1, 1, 1, 0, 1, 1, 1 }, { 0, 1, 1, 1, 1, 1, 1, 1 },  // 28-31
        { 0, 1, 0, 1, 0, 1, 0, 1 }, { 0, 1, 0, 1, 1, 1, 0, 1 },
        { 0, 1, 1, 1, 0, 1, 1, 1 }, { 0, 1, 1, 1, 1, 1, 1, 1 },  // 32-35
        { 0, 1, 0, 1, 0, 1, 0, 1 }, { 0, 1, 0, 1, 1, 1, 0, 1 },
        { 0, 1, 1, 1, 0, 1, 1, 1 }, { 0, 1, 1, 1, 1, 1, 1, 1 },  // 36-39
        { 0, 1, 0, 1, 0, 1, 0, 1 }, { 0, 1, 0, 1, 1, 1, 0, 1 },
        { 0, 1, 1, 1, 0, 1, 1, 1 }, { 0, 1, 1, 1, 1, 1, 1, 1 },  // 40-43
        { 0, 1, 0, 1, 0, 1, 0, 1 }, { 0, 1, 0, 1, 1, 1, 0, 1 },
        { 0, 1, 1, 1, 0, 1, 1, 1 }, { 0, 1, 1, 1, 1, 1, 1, 1 },  // 44-47
        { 1, 1, 1, 1, 1, 1, 1, 1 }, { 1, 1, 1, 2, 1, 1, 1, 2 },
        { 1, 2, 1, 2, 1, 2, 1, 2 }, { 1, 2, 2, 2, 1, 2, 2, 2 },  // 48-51
        { 2, 2, 2, 2, 2, 2, 2, 2 }, { 2, 2, 2, 4, 2, 2, 2, 4 },
        { 2, 4, 2, 4, 2, 4, 2, 4 }, { 2, 4, 4, 4, 2, 4, 4, 4 },  // 52-55
        { 4, 4, 4, 4, 4, 4, 4, 4 }, { 4, 4, 4, 8, 4, 4, 4, 8 },
        { 4, 8, 4, 8, 4, 8, 4, 8 }, { 4, 8, 8, 8, 4, 8, 8, 8 },  // 56-59
        { 8, 8, 8, 8, 8, 8, 8, 8 }, { 8, 8, 8, 8, 8, 8, 8, 8 },
        { 8, 8, 8, 8, 8, 8, 8, 8 }, { 8, 8, 8, 8, 8, 8, 8, 8 },  // 60-63
    };
};

class c_phase_generator
{
  public:
    void reset();
    void clock(uint8_t lfo_counter, uint8_t fm_level);
    uint32_t get_output();
    void reset_counter();
    uint32_t f_number;
    uint8_t block;
    uint8_t detune;
    uint32_t multiple;

  private:
    uint32_t counter;
    const uint8_t detune_table[32][4] = {
        { 0, 0, 1, 2 },   { 0, 0, 1, 2 },   { 0, 0, 1, 2 },   { 0, 0, 1, 2 },  // Block 0
        { 0, 1, 2, 2 },   { 0, 1, 2, 3 },   { 0, 1, 2, 3 },   { 0, 1, 2, 3 },  // Block 1
        { 0, 1, 2, 4 },   { 0, 1, 3, 4 },   { 0, 1, 3, 4 },   { 0, 1, 3, 5 },  // Block 2
        { 0, 2, 4, 5 },   { 0, 2, 4, 6 },   { 0, 2, 4, 6 },   { 0, 2, 5, 7 },  // Block 3
        { 0, 2, 5, 8 },   { 0, 3, 6, 8 },   { 0, 3, 6, 9 },   { 0, 3, 7, 10 },  // Block 4
        { 0, 4, 8, 11 },  { 0, 4, 8, 12 },  { 0, 4, 9, 13 },  { 0, 5, 10, 14 },  // Block 5
        { 0, 5, 11, 16 }, { 0, 6, 12, 17 }, { 0, 6, 13, 19 }, { 0, 7, 14, 20 },  // Block 6
        { 0, 8, 16, 22 }, { 0, 8, 16, 22 }, { 0, 8, 16, 22 }, { 0, 8, 16, 22 },  // Block 7
    };
    const uint8_t vibrato_table[8][8] = {
        {0,  0,  0,  0,  0,  0,  0,  0},  // Vibrato level 0
        {0,  0,  0,  0,  4,  4,  4,  4},  // Vibrato level 1
        {0,  0,  0,  4,  4,  4,  8,  8},  // Vibrato level 2
        {0,  0,  4,  4,  8,  8, 12, 12},  // Vibrato level 3
        {0,  0,  4,  8,  8,  8, 12, 16},  // Vibrato level 4
        {0,  0,  8, 12, 16, 16, 20, 24},  // Vibrato level 5
        {0,  0, 16, 24, 32, 32, 40, 48},  // Vibrato level 6
        {0,  0, 32, 48, 64, 64, 80, 96},  // Vibrato level 7
    };
};

class c_fm_operator
{
  public:
    void reset();
    void key(bool key_on);
    c_phase_generator phase_generator;
    c_envelope_generator envelope_generator;
    void update_frequency(uint32_t f_number, uint8_t block);
    void update_key_scale(uint32_t key_scale);
    void update_key_scale_rate();
    int32_t current_output;
    int32_t last_output;
    int32_t get_output(int32_t modulation_input);
    uint32_t am_enable;
    uint32_t tremolo_attenuation;

  private:
    uint32_t attenuation_to_amplitude(uint32_t attenuation);
};

class c_fm_channel
{
  public:
    c_fm_channel(uint8_t &lfo_counter) : lfo_counter(lfo_counter) {};
    void reset();
    void clock();
    int32_t get_output();
    c_fm_operator operators[4];
    uint8_t panning;
    uint8_t algorithm;
    uint8_t feedback;
    bool multi_frequency_operators;
    uint32_t f_number;
    uint32_t block;
    uint32_t operator_f_number_hi[4];
    uint32_t operator_f_number[4];
    uint8_t operator_block[4];
    void update_frequency();
    uint8_t fm_level;
    uint8_t am_level;

   private:
    int32_t out_i;
    uint8_t &lfo_counter;
};

export class c_ym2612
{
  public:
    void clock(int cycles);
    void write(uint16_t address, uint8_t data);
    uint8_t read(uint16_t address);
    void reset();
    float out_l;
    float out_r;

  private:
    int ticks;
    int group;

    uint8_t lfo_counter;
    uint8_t lfo_divider;
    uint8_t lfo_freq;
    uint8_t lfo_enabled;

    const uint8_t lfo_freqs[8] = {108, 77, 71, 67, 62, 44, 8, 5};

    uint8_t reg_addr;
    uint8_t reg_value;
    uint8_t dac_output;
    bool dac_enabled;
    int channel_idx;
    int operator_idx;

    uint32_t timer_a;
    uint32_t timer_a_reload;
    bool timer_a_enabled;
    uint8_t status;
    int busy_counter;
    uint32_t timer_b;
    uint32_t timer_b_reload;
    bool timer_b_enabled;
    int timer_b_divider;
    bool timer_a_set_flag;
    bool timer_b_set_flag;

    enum
    {
        GROUP_ONE = 0,
        GROUP_TWO = 1
    };

    void clock_timer_a();
    void clock_timer_b();
    void clock_lfo();

    std::array<c_fm_channel, 6> channels = {
        c_fm_channel(lfo_counter), c_fm_channel(lfo_counter), c_fm_channel(lfo_counter),
        c_fm_channel(lfo_counter), c_fm_channel(lfo_counter), c_fm_channel(lfo_counter),
    };
    void clock_channels();
    int32_t get_dac_sample();
    void mix();

    void write_register();
    void write_operator();
    void write_channel();
    void write_global();
};

