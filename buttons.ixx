module;

export module nemulator.buttons;
import nemulator.std;


export enum BUTTONS
{
    BUTTON_1A,
    BUTTON_1B,
    BUTTON_1SELECT,
    BUTTON_1START,
    BUTTON_1UP,
    BUTTON_1DOWN,
    BUTTON_1LEFT,
    BUTTON_1RIGHT,
    BUTTON_2A,
    BUTTON_2B,
    BUTTON_2SELECT,
    BUTTON_2START,
    BUTTON_2UP,
    BUTTON_2DOWN,
    BUTTON_2LEFT,
    BUTTON_2RIGHT,
    BUTTON_INC_PPUCYCLES,
    BUTTON_DEC_PPUCYCLES,
    BUTTON_INC_MMC3,
    BUTTON_DEC_MMC3,
    BUTTON_STATS,
    BUTTON_MASK_SIDES,
    BUTTON_AUDIO_INFO,
    BUTTON_RESET,
    BUTTON_1A_TURBO,
    BUTTON_1B_TURBO,
    BUTTON_1C_TURBO,
    BUTTON_2A_TURBO,
    BUTTON_2B_TURBO,
    BUTTON_2C_TURBO,
    BUTTON_LEFT_SHIFT,
    BUTTON_RIGHT_SHIFT,
    BUTTON_SPRITE_LIMIT,
    BUTTON_ESCAPE,
    BUTTON_RETURN,
    BUTTON_DEC_SHARPNESS,
    BUTTON_INC_SHARPNESS,
    BUTTON_SMS_PAUSE,
    BUTTON_SWITCH_DISK,
    BUTTON_1C,
    BUTTON_2C,
    BUTTON_VOLUME_UP,
    BUTTON_VOLUME_DOWN,
    BUTTON_HOME,
    BUTTON_SCANLINES,
    BUTTON_MENU_UP,
    BUTTON_MENU_DOWN,
    BUTTON_MENU_LEFT,
    BUTTON_MENU_RIGHT,
    //generic button aliases
    BUTTON_UP,
    BUTTON_DOWN,
    BUTTON_LEFT,
    BUTTON_RIGHT,
    BUTTON_OK,
    BUTTON_CANCEL,

    BUTTON_COUNT,
};


export struct s_button_map
{
    uint32_t button;
    uint32_t mask;
    const char *name = nullptr; //label shown in input configuration
};

//a button's auto-fire button, or BUTTON_COUNT if it doesn't have one.  BUTTON_1A is 0, so 0 can't
//stand for "no button".
export constexpr uint32_t get_turbo_button(uint32_t button)
{
    switch (button) {
    case BUTTON_1A: return BUTTON_1A_TURBO;
    case BUTTON_1B: return BUTTON_1B_TURBO;
    case BUTTON_1C: return BUTTON_1C_TURBO;
    case BUTTON_2A: return BUTTON_2A_TURBO;
    case BUTTON_2B: return BUTTON_2B_TURBO;
    case BUTTON_2C: return BUTTON_2C_TURBO;
    }
    return BUTTON_COUNT;
}

//the button an auto-fire button fires, or BUTTON_COUNT
export constexpr uint32_t get_turbo_target(uint32_t turbo_button)
{
    switch (turbo_button) {
    case BUTTON_1A_TURBO: return BUTTON_1A;
    case BUTTON_1B_TURBO: return BUTTON_1B;
    case BUTTON_1C_TURBO: return BUTTON_1C;
    case BUTTON_2A_TURBO: return BUTTON_2A;
    case BUTTON_2B_TURBO: return BUTTON_2B;
    case BUTTON_2C_TURBO: return BUTTON_2C;
    }
    return BUTTON_COUNT;
}

//player 2's equivalent of a player 1 button, or BUTTON_COUNT
export constexpr uint32_t get_player2_button(uint32_t button)
{
    switch (button) {
    case BUTTON_1A: return BUTTON_2A;
    case BUTTON_1B: return BUTTON_2B;
    case BUTTON_1C: return BUTTON_2C;
    }
    return BUTTON_COUNT;
}