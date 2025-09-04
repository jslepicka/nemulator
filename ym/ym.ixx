module;

export module ym;
import nemulator.std;

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
};

class c_fm_operator
{
  public:
    c_fm_operator();
    void reset();
    void key(bool on);
    bool key_on;
    c_phase_generator phase_generator;
};

class c_fm_channel
{
  public:
    c_fm_channel();
    void reset();
    void clock();
    float output();
    c_fm_operator operators[4];

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

