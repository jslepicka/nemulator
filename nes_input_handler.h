#include "input_handler.h"

class c_nes_input_handler :	public c_input_handler
{
public:
	c_nes_input_handler(int buttons);
	~c_nes_input_handler();
	enum buttons
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
		BUTTON_SCREENSHOT,
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
		BUTTON_INC_SHARPNESS
	};
	unsigned char get_nes_byte(int controller);
	void set_turbo_state(int button, int turbo_enabled);
	int get_turbo_state(int button);
	void set_turbo_rate(int button, int rate);
	int get_turbo_rate(int button);
private:
	int prev_temp[2];
	int mask[2];
	int *turbo_state;
	int *turbo_rate;
	static const int turbo_rate_max = 20;
	static const int turbo_rate_min = 2;
};