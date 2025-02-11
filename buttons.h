#pragma once
enum BUTTONS
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
    BUTTON_EMULATION_MODE,
    BUTTON_1A_TURBO,
    BUTTON_1B_TURBO,
    BUTTON_2A_TURBO,
    BUTTON_2B_TURBO,
    BUTTON_LEFT_SHIFT,
    BUTTON_RIGHT_SHIFT,
    BUTTON_SPRITE_LIMIT,
    BUTTON_MEM_VIEWER,
    BUTTON_INPUT_SAVE,
    BUTTON_INPUT_REPLAY,
    BUTTON_ESCAPE,
    BUTTON_RETURN,
    BUTTON_DEC_SHARPNESS,
    BUTTON_INC_SHARPNESS,
    BUTTON_DEC_SOUND_DELAY,
    BUTTON_INC_SOUND_DELAY,
    BUTTON_SMS_PAUSE,
    BUTTON_1COIN,
    BUTTON_COUNT
};


struct s_button_map
{
    uint32_t button;
    uint32_t mask;
};