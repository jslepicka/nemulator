module;
#pragma once
#pragma comment (lib, "winmm.lib")
#include "windows.h"
#include "mmsystem.h"

export module input_handler;
import nemulator.std;

import nemulator.buttons;

export class c_input_handler
{
public:
    c_input_handler(int buttons);
    virtual ~c_input_handler();
    void poll(double dt, int ignore_input);
    void ack();
    int get_result(int button, bool ack = false);
    double get_hold_time(int button);
    void set_repeat_mode(int button, int mode);
    void set_button_keymap(int button, int key);
    void set_button_joymap(int button, int joy, int joy_button);
    void ack_button(int button);
    void set_button_type(int button, int type);
    void set_button_group(int group, std::vector<int> buttons);
    uint32_t get_console_input(const std::vector<s_button_map> &button_map);
    
    //do something better here
    void enable_extrafast() { extrafast_enabled = 1; }
    void disable_extrafast() { extrafast_enabled = 0; }

    void set_turbo_state(int button, int turbo_enabled);
    int get_turbo_state(int button);
    void set_turbo_rate(int button, int rate);
    int get_turbo_rate(int button);
    enum results
    {
        RESULT_NONE = 0,
        RESULT_DEPRESSED = 1,
        RESULT_UP = 2,
        RESULT_DOWN = 4,
        RESULT_REPEAT = 8,
        RESULT_REPEAT_SLOW = 16,
        RESULT_REPEAT_FAST = 32,
        RESULT_REPEAT_EXTRAFAST = 64
    };
    
    enum type
    {
        TYPE_BUTTON,
        TYPE_AXIS,
        TYPE_POV
    };

    enum axis
    {
        AXIS_X,
        AXIS_Y,
        AXIS_Z
    };
    void set_pair(int button1, int button2)
    {
        pairs.push_back({button1, button2});
    }

  protected:
    bool ackd;
    int num_buttons;
    struct s_state
    {
        int type;
        int state_cur;      //button depressed?
        int state_prev;     //previously depressed?
        int state_result;   //result of poll
        int repeat_count;   //number of slow repeats
        int repeat_enable;  //repeating enabled?
        int depressed_count; //number of samples that the key has been depressed;
        int button_key;     //virtual key code assigned to button
        double hold_time;   //how long has the button been depressed?
        double repeat_time; //next repeat event
        int joy;
        int joy_button;
        int ack;
        int turbo_enabled;
        int turbo_rate;
        int pair_mask;
    };

    struct s_pair
    {
        int button1;
        int button2;
        int prev = 0;
        int mask = 2;
    };
    std::vector<s_pair> pairs;
    std::unique_ptr<s_state[]> state;

    int get_key_state(int key);
    int get_joy_state(int joy, int joy_button);
    int get_joy_pov(int joy, int low, int high);
    int get_joy_axis(int joy, int axis);

    double repeat_start;
    double repeat_rate_slow;
    double repeat_rate_fast;
    double repeat_rate_extrafast;

    JOYINFOEX joyInfoEx[8];
    int joy_poll_result[8];
    double joy_suppressed[8];
    inline static const double JOY_SUPPRESSED_TIME = 2000.0; //if joystick polling fails, wait 2 seconds until trying again
    unsigned char joymask;
    int repeat_mask;
    int extrafast_enabled;

    std::map<int, std::vector<int>> groups;
};