#include "input_handler.h"
#include <memory>

class c_nes_input_handler : public c_input_handler
{
public:
    c_nes_input_handler(int buttons);
    ~c_nes_input_handler();

    unsigned char get_nes_byte(int controller);
    void set_turbo_state(int button, int turbo_enabled);
    int get_turbo_state(int button);
    void set_turbo_rate(int button, int rate);
    int get_turbo_rate(int button);
private:
    int prev_temp[2];
    int mask[2];
    std::unique_ptr<int[]> turbo_state;
    std::unique_ptr<int[]> turbo_rate;
    static const int turbo_rate_max = 20;
    static const int turbo_rate_min = 2;
};