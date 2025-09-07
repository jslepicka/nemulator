module;

export module ym;
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
    c_envelope_generator();
    void reset();
    void clock();
    void set_key_on(bool k);
    ADSR_PHASE phase;
    bool key_on;
    uint32_t level;
    uint32_t sustain_level;
    uint32_t output();

    uint32_t attack_rate;
    uint32_t decay_rate;
    uint32_t sustain_rate;
    uint32_t release_rate;
    uint32_t total_level;
    uint32_t rate_scaling;
    uint32_t am_enable;
    uint32_t key_scale;
    uint32_t key_scale_rate;

  private:
    int divider;
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
    c_phase_generator();
    void reset();
    void clock();
    uint32_t output();
    void reset_counter();

    uint32_t counter;
    uint32_t f_number;
    uint8_t block;
    uint8_t detune;
    uint32_t multiple;

    private:
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
};

class c_fm_operator
{
  public:
    c_fm_operator();
    void reset();
    void key(bool on);
    bool key_on;
    c_phase_generator phase_generator;
    c_envelope_generator envelope_generator;
    void update_frequency(uint32_t f_number, uint8_t block);
    void update_key_scale(uint32_t key_scale);
    void update_key_scale_rate();
    int32_t current_output;
    int32_t last_output;
    int32_t output(int32_t modulation_input);

  private:
    uint32_t attenuation_to_amplitude(uint32_t attenuation);
};

class c_fm_channel
{
  public:
    c_fm_channel();
    void reset();
    void clock();
    float output();
    c_fm_operator operators[4];
    uint8_t panning;
    uint8_t algorithm;
    uint8_t feedback;

  private:
    float out;
};

export class c_ym
{
  public:
    c_ym();
    ~c_ym();
    void clock(int cycles);
    void write(uint16_t address, uint8_t data);
    uint8_t read(uint16_t address);
    void reset();
    float out;

  private:
    int ticks;
    int group;

    uint8_t reg_addr;
    uint8_t data;
    int channel_idx;
    int operator_idx;

    uint32_t timer_a;
    uint32_t timer_a_reload;
    int timer_a_enabled;
    uint8_t status;
    int busy_counter;
    uint32_t timer_b;
    uint32_t timer_b_reload;
    int timer_b_enabled;
    int timer_b_divider;

    //we don't really need 256 of these but just keeping it simple
    uint8_t registers[256];

    enum
    {
        GROUP_ONE = 0,
        GROUP_TWO = 1
    };

    void clock_timer_a();
    void clock_timer_b();

    uint8_t freq_hi[6];

    c_fm_channel channels[6];
    void clock_channels();
    float get_dac_sample();
    float mix();
};

