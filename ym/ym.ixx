module;

export module ym;
import nemulator.std;

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

    uint8_t addr;
    uint8_t data;
    int channel_idx;
    int operator_idx;

    uint32_t timer_a;
    uint32_t timer_a_reload;
    int timer_a_enabled;
    uint8_t status;
    int busy_counter;

    //we don't really need 256 of these but just keeping it simple
    uint8_t registers[256];

    enum
    {
        GROUP_ONE = 0,
        GROUP_TWO = 1
    };
};
