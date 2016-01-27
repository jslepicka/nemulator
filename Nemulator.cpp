#include "Nemulator.h"
#include "menu.h"
#include "PowrProf.h"
#include "time.h"
#include <sstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <xmmintrin.h>
#include "console.h"
#include <algorithm>

extern ID3D10Device *d3dDev;
extern D3DXMATRIX matrixView;
extern D3DXMATRIX matrixProj;
extern int clientHeight;
extern int clientWidth;
extern bool startFullscreen;
extern bool aspectLock;
extern double aspectRatio;
extern HWND hWnd;
extern D3d10App *app;

extern c_config *config;
extern c_input_handler *g_ih;

int pal[64 * 8];
int mode;
int prevMode;
bool updateEvents;

int mem_viewer_active = 0;

HANDLE g_start_event;

c_nemulator::c_nemulator()
{
	prevMode = mode = MODE_MENU;
	startEvents = NULL;
	doneEvents = NULL;
	done_events = NULL;
	runnableCount = 0;
	updateEvents = true;
	selectedPanel = 0;
	inGame = false;
	joy1 = 0;
	joy2 = 0;
	sound = 0;
	font1 = 0;
	font2 = 0;
	font3 = 0;
	max_fps = 60.0;
	for (int i = 0; i < fps_records; i++)
		fps_history[i] = 0.0;
	fps_index = 0;
	ih = 0;
	menu = 0;
	paused = false;
	reset_on_select = false;
	emulation_mode_menu = c_nes::EMULATION_MODE_FAST;
	emulation_mode_ingame = c_nes::EMULATION_MODE_ACCURATE;
	g_start_event = CreateEvent(NULL, TRUE, TRUE, NULL);
	stats = NULL;
	audio_info = NULL;
	mem_viewer = NULL;
	qam = NULL;
	splash_done = 0;
	splash_timer = 2000.0;
#if defined(DEBUG)
	splash_timer = 1.0f;
#endif
	splash_fade_timer = 1.0;
	splash_stage = 0;
	loaded = 0;
	num_games = 0;
	preload = false;
	load_thread_handle = 0;
	input_buffer_index = 0;
	input_buffer_playback = 0;
	input_buffer_end = 0;
	show_suspend = false;

	_MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
	_MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);

}

void c_nemulator::kill_threads()
{
	for (auto &game_thread : game_threads)
	{
		ResumeThread(game_thread->thread_handle);
		game_thread->kill = 1;
		SetEvent(game_thread->start_event);
		WaitForSingleObject(game_thread->thread_handle, INFINITE);
		CloseHandle(game_thread->start_event);
		CloseHandle(game_thread->done_event);
		CloseHandle(game_thread->thread_handle);
		delete game_thread;
	}
}

c_nemulator::~c_nemulator(void)
{
	//if (stats)
	//	delete stats;
	if (startEvents)
		delete[] startEvents;
	if (doneEvents)
		delete[] doneEvents;
	if (done_events)
		delete[] done_events;
	ReleaseCOM(font1);
	delete (mainPanel2);
	if (sound)
		delete sound;
	kill_threads();
	for (auto &game : gameList)
	{
		delete game;
	}
}

void c_nemulator::LoadFonts()
{
	ReleaseCOM(font1);
	ReleaseCOM(font2);
	ReleaseCOM(font3);

	D3DX10_FONT_DESC fontDesc;
	fontDesc.Height = (int)(clientHeight * .08);
	fontDesc.Width = 0;
	fontDesc.Weight = 0;
	fontDesc.MipLevels = 1;
	fontDesc.Italic = false;
	fontDesc.CharSet = DEFAULT_CHARSET;
	fontDesc.OutputPrecision = OUT_DEFAULT_PRECIS;
	fontDesc.Quality = DEFAULT_QUALITY;
	fontDesc.PitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
	strcpy_s(fontDesc.FaceName, "Calibri");
	hr = D3DX10CreateFontIndirect(d3dDev, &fontDesc, &font1);

	fontDesc.Height = (int)(clientHeight * .04);
	fontDesc.Width = 0;
	fontDesc.Weight = 0;
	fontDesc.MipLevels = 1;
	fontDesc.Italic = false;
	fontDesc.CharSet = DEFAULT_CHARSET;
	fontDesc.OutputPrecision = OUT_DEFAULT_PRECIS;
	fontDesc.Quality = DEFAULT_QUALITY;
	fontDesc.PitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
	strcpy_s(fontDesc.FaceName, "Calibri");
	hr = D3DX10CreateFontIndirect(d3dDev, &fontDesc, &font2);

	fontDesc.Height = (int)(clientHeight * .9);
	fontDesc.Width = 0;
	fontDesc.Weight = FW_SEMIBOLD;
	fontDesc.MipLevels = 1;
	fontDesc.Italic = false;
	fontDesc.CharSet = DEFAULT_CHARSET;
	fontDesc.OutputPrecision = OUT_DEFAULT_PRECIS;
	fontDesc.Quality = DEFAULT_QUALITY;
	fontDesc.PitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
	strcpy_s(fontDesc.FaceName, "Calibri");
	hr = D3DX10CreateFontIndirect(d3dDev, &fontDesc, &font3);

}

void c_nemulator::generate_palette()
{
	double saturation = 1.3;
	double hue_tweak = 0.0;
	double contrast = 1.0;
	double brightness = 1.0;

	for (int pixel = 0; pixel < 512; pixel++)
	{
		int color = (pixel & 0xF);
		int level = color < 0xE ? (pixel >> 4) & 0x3 : 1;

		static const double black = .518;
		static const double white = 1.962;
		static const double attenuation = .746;
		static const double levels[8] = {.350, .518, .962, 1.550,
			1.094, 1.506, 1.962, 1.962};
		double lo_and_hi[2] = {levels[level + 4 * (color == 0x0)],
			levels[level + 4 * (color < 0xd)]};
		double y = 0.0;
		double i = 0.0;
		double q = 0.0;

		for (int p = 0; p < 12; ++p)
		{
			auto wave = [](int p, int color){
				return (color + p + 8) % 12 < 6;
			};

			double spot = lo_and_hi[wave(p, color)];
			if (((pixel & 0x40) && wave(p, 12))
				|| ((pixel & 0x80) && wave(p, 4))
				|| ((pixel & 0x100) && wave(p, 8))) spot *= attenuation;
			double v = (spot - black) / (white-black);

			v = (v - .5) * contrast + .5;
			v *= brightness / 12.0;

			y += v;
			i += v * cos((D3DX_PI/6.0) * (p+hue_tweak));
			q += v * sin((D3DX_PI/6.0) * (p+hue_tweak));
		}
		
		i *= saturation;
		q *= saturation;

		auto clamp = [](int v, int min, int max){
			return v < min ? min : v > max ? max : v;
		};

		auto d_clamp = [](double v, double min, double max)
		{
			return v < min ? min : v > max ? max : v;
		};

		//Y'IQ -> NTSC R'G'B'
		//conversion matrix from http://wiki.nesdev.com/w/index.php/NTSC_video
		//Adapted from http://en.wikipedia.org/wiki/YIQ, FCC matrix []^-1
		//double ntsc_r = y +  0.946882 * i +  0.623557 * q;
		//double ntsc_g = y + -0.274788 * i + -0.635691 * q;
		//double ntsc_b = y + -1.108545 * i +  1.709007 * q;
		//double ntsc_r = y + i * 0.946882217090069 + q * 0.623556581986143;
		//double ntsc_g = y + i * -0.274787646298978 + q * -0.635691079187380;
		//double ntsc_b = y + i * -1.10854503464203 + q * 1.70900692840647;
		double ntsc_r = y +  0.956 * i +  0.620 * q;
		double ntsc_g = y + -0.272 * i + -0.647 * q;
		double ntsc_b = y + -1.108 * i +  1.705 * q;

		//NTSC R'G'B' -> linear NTSC RGB
		ntsc_r = pow(d_clamp(ntsc_r, 0.0, 1.0), 2.2);
		ntsc_g = pow(d_clamp(ntsc_g, 0.0, 1.0), 2.2);
		ntsc_b = pow(d_clamp(ntsc_b, 0.0, 1.0), 2.2);

		//NTSC RGB (SMPTE-C) -> CIE XYZ
		//conversion matrix from http://www.brucelindbloom.com/index.html?Eqn_RGB_XYZ_Matrix.html
		double cie_x = ntsc_r * 0.3935891 + ntsc_g * 0.3652497 + ntsc_b * 0.1916313;
		double cie_y = ntsc_r * 0.2124132 + ntsc_g * 0.7010437 + ntsc_b * 0.0865432;
		double cie_z = ntsc_r * 0.0187423 + ntsc_g * 0.1119313 + ntsc_b * 0.9581563;

		//CIE XYZ -> linear sRGB
		//Shader will return sR'G'B'
		//conversion matrix from http://www.brucelindbloom.com/index.html?Eqn_RGB_XYZ_Matrix.html
		double srgb_r2 = cie_x * 3.2404542 + cie_y * -1.5371385 + cie_z * -0.4985314;
		double srgb_g2 = cie_x * -0.9692660 + cie_y * 1.8760108 + cie_z * 0.0415560;
		double srgb_b2 = cie_x * 0.0556434 + cie_y * -0.2040259 + cie_z * 1.0572252;

		double srgb_r = ntsc_r * 0.939555319482800 + ntsc_g * 0.0501723952684700 + ntsc_b * 0.0102725646454400;
		double srgb_g = ntsc_r * 0.0177757796807600 + ntsc_g * 0.965792853854560 + ntsc_b * 0.0164314174435600;
		double srgb_b = ntsc_r * -0.00162232670898000 + ntsc_g * -0.00437074564609001 + ntsc_b * 1.00599294870830;

		srgb_r = d_clamp(srgb_r, 0.0, 1.0);
		srgb_g = d_clamp(srgb_g, 0.0, 1.0);
		srgb_b = d_clamp(srgb_b, 0.0, 1.0);

		auto gammafix = [](double f){
			float gamma = 2.2;
			return f;
			//return f < 0.0 ? 0.0 : pow(f, 2.2 / gamma);
			return f < 0.0 ? 0.0 : pow(f, gamma);
		};


		int rgb = 0x000001 * (int)(255.0 * srgb_r)
				+ 0x000100 * (int)(255.0 * srgb_g)
				+ 0x010000 * (int)(255.0 * srgb_b);

		pal[pixel] = rgb | 0xFF000000;
	}
}

void c_nemulator::init(void *params)
{
	Init();
	init_threads();
}

DWORD WINAPI c_nemulator::game_thread(LPVOID lpParam)
{
	_MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
	_MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);

	s_game_thread *t = (s_game_thread*)lpParam;
	while(true)
	{
		WaitForSingleObject(t->start_event, INFINITE);
		if (t->kill)
			break;
		for (auto &g : t->game_list)
		{
			if (g->console && g->console->is_loaded())
				g->console->emulate_frame();
		}
		SetEvent(t->done_event);
	}

	return 0;
};

int c_nemulator::init_threads()
{
	SYSTEM_INFO sysinfo;
	GetSystemInfo( &sysinfo );
	DWORD num_procs = sysinfo.dwNumberOfProcessors;
	num_threads = 0;
	for (int i = 0; i < (int)(num_procs * 2) && i < 64; i++)
	{
		s_game_thread *t = new s_game_thread;
		t->start_event = CreateEvent(NULL, FALSE, FALSE, NULL);
		t->done_event = CreateEvent(NULL, FALSE, FALSE, NULL);
		t->kill = 0;
		t->thread_handle = CreateThread(NULL, 0, c_nemulator::game_thread, t, 0, NULL);
		num_threads++;
		game_threads.push_back(t);
	}

	return 0;
}
void c_nemulator::Init()
{
	status = new c_status();
	status->add_message("");
	add_task(status, NULL);

	//mem_viewer = new c_mem_viewer();
	//add_task(mem_viewer, NULL);

	LoadFonts();
	sound = new Sound(hWnd);
	sound->Init();

	generate_palette();
	fastscroll = false;
	scroll_fade_timer = 0.0;
	static const float tile_width = 2.7f;

	int panel_columns = config->get_int("menu_columns", 7);
	if (panel_columns < 3)
		panel_columns = 3;
	int panel_rows = (int)((panel_columns-1) * tile_width / ((double)clientWidth/clientHeight) * .82 / 2.04);

	mainPanel2 = new TexturePanel(panel_rows, panel_columns);
	mainPanel2->x = 0.0f;
	mainPanel2->y = 0.0f;
	mainPanel2->z = 0.0f;
	mainPanel2->selected = true;
	mainPanel2->stretch_to_fit = config->get_bool("app.stretch_to_fit", false);
	sharpness = config->get_double("sharpness", .8);
	if (sharpness < 0.0f)
	{
		sharpness = 0.0f;
	}
	else if (sharpness > 1.0f)
	{
		sharpness = 1.0f;
	}
	mainPanel2->set_sharpness(sharpness);
	texturePanels[0] = mainPanel2;


	QueryPerformanceFrequency(&liFreq);


	eye_x = (float)(((panel_columns - 1) * tile_width) / 2);
	eye_y = (float)((((panel_rows - 1) * 2.04) / 2) * -1.2);

	fov_h = (float)(2.0 * atan(((double)clientWidth/clientHeight) * tan( (D3DX_PI/4)/2.0) ));
	eye_z = -(eye_x/tan(fov_h/2));

	mainPanel2->zoomDestX = eye_x;
	mainPanel2->zoomDestY = eye_y;
	mainPanel2->zoomDestZ = (float)(eye_z + (1.0 / tan( (D3DX_PI/4)/2.0 )));
	mainPanel2->camera_distance = eye_z;

	D3DXMatrixLookAtLH(
		&matrixView,
		&D3DXVECTOR3(eye_x, eye_y, eye_z),
		&D3DXVECTOR3(eye_x, eye_y, 0.0f),
		&D3DXVECTOR3(0.0f, 1.0f, 0.0f));

	reset_on_select = config->get_bool("reset_on_select", true);
	menu_delay = config->get_double("menu_delay", 333.0);

	preload = config->get_bool("preload", true);

	show_suspend = config->get_bool("show_suspend", false);

	std::string s = config->get_string("emulation_mode_menu", "fast");
	if (s == "accurate")
	{
		emulation_mode_menu = c_nes::EMULATION_MODE_ACCURATE;
	}
	else
	{
		emulation_mode_menu = c_nes::EMULATION_MODE_FAST;
	}

	s = config->get_string("emulation_mode_ingame", "accurate");
	if (s == "accurate")
	{
		emulation_mode_ingame = c_nes::EMULATION_MODE_ACCURATE;
	}
	else
	{
		emulation_mode_ingame = c_nes::EMULATION_MODE_FAST;
	}

	if (menu_delay < 1.0)
		menu_delay = 1.0;

	configure_input();

	QueryPerformanceCounter(&liLast);
	OnResize();

}

void c_nemulator::configure_input()
{
	struct s_button_map
	{
		BUTTONS button;
		std::string config_base;
		std::string config_name;
		int default_key;
		int repeat_mode;
	};
	static const int button_mask = 0x80000000;
	s_button_map button_map[] =
	{
		{ BUTTON_1LEFT,         "joy1",     "left",    VK_LEFT,                     1 },
		{ BUTTON_1RIGHT,        "joy1",     "right",   VK_RIGHT,                    1 },
		{ BUTTON_1UP,           "joy1",     "up",      VK_UP,                       1 },
		{ BUTTON_1DOWN,         "joy1",     "down",    VK_DOWN,                     1 },
		{ BUTTON_1A,            "joy1",     "a",       88,                          0 },
		{ BUTTON_1A_TURBO,      "joy1",     "a_turbo", 0x53,                        0 },
		{ BUTTON_1B,            "joy1",     "b",       90,                          0 },
		{ BUTTON_1B_TURBO,      "joy1",     "b_turbo", 0x41,                        0 },
		{ BUTTON_1SELECT,       "joy1",     "select",  VK_OEM_4,                    0 },
		{ BUTTON_1START,        "joy1",     "start",   VK_OEM_6,                    0 },
		{ BUTTON_SMS_PAUSE,     "joy1.sms", "pause",   BUTTON_1START | button_mask, 0 },

		{ BUTTON_2LEFT,         "joy2",     "left",    0,                           1 },
		{ BUTTON_2RIGHT,        "joy2",     "right",   0,                           1 },
		{ BUTTON_2UP,           "joy2",     "up",      0,                           1 },
		{ BUTTON_2DOWN,         "joy2",     "down",    0,                           1 },
		{ BUTTON_2A,            "joy2",     "a",       0,                           0 },
		{ BUTTON_2A_TURBO,      "joy2",     "a_turbo", 0,                           0 },
		{ BUTTON_2B,            "joy2",     "b",       0,                           0 },
		{ BUTTON_2B_TURBO,      "joy2",     "b_turbo", 0,                           0 },
		{ BUTTON_2SELECT,       "joy2",     "select",  0,                           0 },
		{ BUTTON_2START,        "joy2",     "start",   0,                           0 },

		{ BUTTON_INPUT_REPLAY,   "",        "",        VK_F12,                      0 },
		{ BUTTON_INPUT_SAVE,     "",        "",        VK_F11,                      0 },
		{ BUTTON_STATS,          "",        "",        VK_F9,                       0 },
		{ BUTTON_MASK_SIDES,     "",        "",        VK_F8,                       0 },
		{ BUTTON_SPRITE_LIMIT,   "",        "",        VK_F7,                       0 },
		{ BUTTON_SCREENSHOT,     "",        "",        VK_F6,                       0 },
		{ BUTTON_MEM_VIEWER,     "",        "",        VK_F5,                       0 },
		{ BUTTON_AUDIO_INFO,     "",        "",        VK_F4,                       0 },
		{ BUTTON_EMULATION_MODE, "",        "",        VK_F3,                       0 },
		{ BUTTON_RESET,          "",        "",        VK_F2,                       0 },
		{ BUTTON_LEFT_SHIFT,     "",        "",        VK_LSHIFT,                   0 },
		{ BUTTON_RIGHT_SHIFT,    "",        "",        VK_RSHIFT,                   0 },
		{ BUTTON_ESCAPE,         "",        "",        VK_ESCAPE,                   0 },
		{ BUTTON_RETURN,         "",        "",        VK_RETURN,                   0 },
		{ BUTTON_DEC_SHARPNESS,  "",        "",        0x39,                        0 },
		{ BUTTON_INC_SHARPNESS,  "",        "",        0x30,                        0 },
	};

	int num_buttons = sizeof(button_map) / sizeof(s_button_map);
	g_ih = new c_nes_input_handler(BUTTON_COUNT);

	for (int i = 0; i < num_buttons; i++)
	{
		int j = i;
		int default_key = button_map[i].default_key;
		int button = button_map[i].button;
		std::string key = button_map[i].config_base + "." + button_map[i].config_name;
		//int d = key == "." ? default_key : config->get_int(key, default_key);
		if (default_key & button_mask)
		{
			default_key &= button_mask - 1;
			for (int k = 0; k < num_buttons; k++)
			{
				if (button_map[k].button == default_key)
				{
					j = k;
					default_key = button_map[k].default_key;
					break;
				}
			}
		}

		std::string joy_base = button_map[j].config_base + ".joy";
		std::string joy_key = joy_base +  "." + button_map[j].config_name;
		int repeat_mode = button_map[j].repeat_mode;
		int d = key == "." ? default_key : config->get_int(key, default_key);
		g_ih->set_button_keymap(button, d);
		g_ih->set_repeat_mode(button, repeat_mode);
		g_ih->set_button_joymap(button, config->get_int(joy_base, -1), config->get_int(joy_key, -1));
		g_ih->set_button_type(button, config->get_int(joy_key + ".type", 0));
	}
}

void c_nemulator::resize()
{
	OnResize();
}
void c_nemulator::OnResize()
{
	LoadFonts();

	float aspect = (float)clientWidth/clientHeight;
	D3DXMatrixPerspectiveFovLH(&matrixProj, .25f*(float)D3DX_PI, aspect, 1.0f, 1000.0f);
	texturePanels[selectedPanel]->OnResize();
}

void c_nemulator::RunGames()
{
	if (!inGame)
	{
		if (texturePanels[selectedPanel]->get_num_items() > 0)
		{
			int x = 0;
			for (auto &game_thread : game_threads)
			{
				SetEvent(game_thread->start_event);
			}
			WaitForMultipleObjects(game_threads.size(), done_events, TRUE, INFINITE);
		}
	}
	else
	{
		//c_console *n = ((Game*)texturePanels[selectedPanel]->GetSelected())->console;
		Game *g = (Game*)texturePanels[selectedPanel]->GetSelected();
		switch (g->type)
		{
		case GAME_NES:
			g->console->set_input(
				((c_nes_input_handler*)g_ih)->get_nes_byte(0) |
				(((c_nes_input_handler*)g_ih)->get_nes_byte(1) << 8));
			break;
		case GAME_SMS:
			g->console->set_input(((c_nes_input_handler*)g_ih)->get_sms_input());
			break;
		default:
			break;
		}

		g->console->emulate_frame();
	}

}

void c_nemulator::ProcessInput(double dt)
{
	//Process NES input
	if (inGame)
	{
		if (texturePanels[selectedPanel]->state != TexturePanel::STATE_ZOOMING)
		{

			if (joy1)
			{
				if (input_buffer_playback)
				{
					*joy1 = input_buffer[input_buffer_index];
					input_buffer_index = ++input_buffer_index % input_buffer_size;
					if (input_buffer_index == input_buffer_end)
					{
						input_buffer_playback = 0;
						//input_buffer_index = 0;
						status->add_message("input replay end");
					}
				}
				else
				{
					*joy1 = ((c_nes_input_handler*)g_ih)->get_nes_byte(0);
					input_buffer[input_buffer_index] = *joy1;
					input_buffer_index = ++input_buffer_index % input_buffer_size;
				}

			}
			if (joy2)
				*joy2 = ((c_nes_input_handler*)g_ih)->get_nes_byte(1);
		}
		else if (!input_buffer_playback)
		{
			input_buffer[input_buffer_index] = 0;
			input_buffer_index = ++input_buffer_index % input_buffer_size;
		}
	}

	//these keys work anytime
	if (g_ih->get_result(BUTTON_RESET, true) & c_input_handler::RESULT_DOWN)
	{
		Game *g = (Game*)texturePanels[selectedPanel]->GetSelected();
		if (g->console->is_loaded())
		{
			g->console->reset();
			input_buffer_index = 0;
			input_buffer_playback = 0;
			status->add_message("reset");
		}
	}
	else if (g_ih->get_result(BUTTON_INPUT_SAVE, true) & c_input_handler::RESULT_DOWN)
	{
		Game *g = (Game*)texturePanels[selectedPanel]->GetSelected();
		if (g->console->is_loaded())
		{
			std::ofstream file;
			std::string filename = g->console->pathFile;
			filename += ".input";
			file.open(filename, std::ios_base::trunc | std::ios_base::binary);
			if (file.is_open())
			{
				for (int i = 0; i < input_buffer_index; i++)
				{
					file.write((char*)&input_buffer[i], sizeof(char));
				}
				file.close();
				status->add_message("saved input");
			}
		}
	}
	else if (g_ih->get_result(BUTTON_INPUT_REPLAY, true) & c_input_handler::RESULT_DOWN)
	{
		Game *g = (Game*)texturePanels[selectedPanel]->GetSelected();
		if (g->console->is_loaded())
		{
			//load input from file
			char *buf = new char[65536];
			std::ifstream file;
			std::string filename = g->console->pathFile;
			filename += ".input";
			file.open(filename, std::ios_base::in | std::ios_base::binary);
			int i = 0;
			if (file.is_open())
			{
				while (!file.eof())
				{
					file.read(buf, 65536);
					int num_bytes = (int)file.gcount();
					memcpy(&input_buffer[i], buf, num_bytes);
					i += (num_bytes/sizeof(char));
				}
				g->console->reset();

				input_buffer_end = i;
				input_buffer_index = 0;
				input_buffer_playback = 1;
				file.close();
				status->add_message("input replay start");
			}

		}
	}
	else if (g_ih->get_result(BUTTON_EMULATION_MODE, true) & c_input_handler::RESULT_DOWN)
	{
		Game *g = (Game*)texturePanels[selectedPanel]->GetSelected();
		if (g->console->is_loaded())
		{
			if (g->console->get_emulation_mode() == c_nes::EMULATION_MODE_ACCURATE)
			{
				g->console->set_emulation_mode(c_nes::EMULATION_MODE_FAST);
				status->add_message("fast emulation mode");
			}
			else
			{
				g->console->set_emulation_mode(c_nes::EMULATION_MODE_ACCURATE);
				status->add_message("accurate emulation mode");
			}
			g->console->reset();
			input_buffer_index = 0;
			input_buffer_playback = 0;
		}
	}
	else if (g_ih->get_result(BUTTON_AUDIO_INFO, true) & c_input_handler::RESULT_DOWN)
	{
		if (audio_info)
		{
			audio_info->dead = true;
			audio_info = NULL;
		}
		else
		{
			audio_info = new c_audio_info();
			audio_info->x = eye_x;
			audio_info->y = eye_y;
			audio_info->z = (float)(eye_z + (1.0 / tan(fov_h/2)));
			add_task(audio_info, NULL);
		}
	}
#ifdef MEM_VIEWER
	else if (g_ih->get_result(BUTTON_MEM_VIEWER, true) & c_input_handler::RESULT_DOWN)
	{
		if (mem_viewer)
		{
			mem_viewer->size *= 2;
			if (mem_viewer->size > 32)
			{
				mem_viewer->dead = true;
				mem_viewer = NULL;
				mem_viewer_active = 0;
			}
		}
		else
		{
			mem_viewer = new c_mem_viewer();
			mem_viewer->x = eye_x;
			mem_viewer->y = eye_y;
			mem_viewer->z = (float)(eye_z + (1.0 / tan(fov_h/2)));
			add_task(mem_viewer, NULL);
			mem_viewer_active = 1;
		}
	}
#endif
	else if (g_ih->get_result(BUTTON_STATS, true) & c_input_handler::RESULT_DOWN)
	{
		if (stats)
		{
			stats->dead = true;
			stats = NULL;
			//status->add_message("stats disabled");
		} else
		{
			stats = new c_stats();
			add_task(stats, NULL);
			//status->add_message("stats enabled");
		}
	}
	else if (g_ih->get_result(BUTTON_MASK_SIDES, true) & c_input_handler::RESULT_DOWN)
	{
		Game *g = (Game*)texturePanels[selectedPanel]->GetSelected();
		g->mask_sides = !g->mask_sides;
		if (g->mask_sides)
			status->add_message("side mask enabled");
		else
			status->add_message("side mask disabled");
	}
	else if (g_ih->get_result(BUTTON_SCREENSHOT) & c_input_handler::RESULT_DOWN)
	{
		if (take_screenshot() == 0)
			status->add_message("saved screenshot");
		else
			status->add_message("screenshot failed");
	}
	else if (g_ih->get_result(BUTTON_SPRITE_LIMIT) & c_input_handler::RESULT_DOWN)
	{
		Game *g = (Game*)texturePanels[selectedPanel]->GetSelected();
		if (g->type == GAME_NES)
		{
			g->console->set_sprite_limit(!g->console->get_sprite_limit());
		}

		if (g->console->get_sprite_limit())
			status->add_message("sprites limited");
		else
			status->add_message("sprites unlimited");
	}
	else if (g_ih->get_result(BUTTON_DEC_SHARPNESS) &
		(c_input_handler::RESULT_DOWN | c_input_handler::RESULT_REPEAT_SLOW | c_input_handler::RESULT_REPEAT_FAST | c_input_handler::RESULT_REPEAT_EXTRAFAST))
	{
		if (sharpness > 0.0f)
		{
			sharpness -= .025f;
			if (sharpness < 0.0f + .0001f)
				sharpness = 0.0f;
			mainPanel2->set_sharpness(sharpness);
			char buf[6];
			sprintf_s(buf, sizeof(buf), "%.3f", sharpness);
			status->add_message("set sharpness to " + std::string(buf));
		}
	}
	else if (g_ih->get_result(BUTTON_INC_SHARPNESS) &
		(c_input_handler::RESULT_DOWN | c_input_handler::RESULT_REPEAT_SLOW | c_input_handler::RESULT_REPEAT_FAST | c_input_handler::RESULT_REPEAT_EXTRAFAST))
	{
		if (sharpness < 1.0f)
		{
			sharpness += .025f;
			if (sharpness + .0001f > 1.0f)
				sharpness = 1.0f;
			mainPanel2->set_sharpness(sharpness);
			char buf[6];
			sprintf_s(buf, sizeof(buf), "%.3f", sharpness);
			status->add_message("set sharpness to " + std::string(buf));
		}
	}

	//these keys work in the menu
	if (!inGame && splash_done)
	{
		int result_mask = c_input_handler::RESULT_DOWN | c_input_handler::RESULT_REPEAT_SLOW | c_input_handler::RESULT_REPEAT_FAST | c_input_handler::RESULT_REPEAT_EXTRAFAST;
		if (fastscroll)
		{
			if ((!g_ih->get_result(BUTTON_1RIGHT)
				&& !g_ih->get_result(BUTTON_1LEFT))
				|| texturePanels[selectedPanel]->is_first_col()
				|| texturePanels[selectedPanel]->is_last_col())
			{
				fastscroll = false;
				scroll_fade_timer = 333.0;
				//texturePanels[selectedPanel]->load_items();
			}		
		}

		if (int r = g_ih->get_result(BUTTON_1RIGHT) & result_mask)
		{
			if (r & c_input_handler::RESULT_REPEAT_EXTRAFAST && !texturePanels[selectedPanel]->is_last_col())
			{
				texturePanels[selectedPanel]->set_scroll_duration(20.0);
				fastscroll = true;
			}
			else if (r & c_input_handler::RESULT_REPEAT_FAST && !texturePanels[selectedPanel]->is_last_col())
			{
				texturePanels[selectedPanel]->set_scroll_duration(50.0);
				fastscroll = true;
			}
			else
				texturePanels[selectedPanel]->set_scroll_duration(180.0);
			texturePanels[selectedPanel]->NextColumn();
		}
		else if (int r = g_ih->get_result(BUTTON_1LEFT) & result_mask)
		{
			if (r & c_input_handler::RESULT_REPEAT_EXTRAFAST && !texturePanels[selectedPanel]->is_first_col())
			{
				texturePanels[selectedPanel]->set_scroll_duration(20.0);
				fastscroll = true;
			}
			else if (r & c_input_handler::RESULT_REPEAT_FAST && !texturePanels[selectedPanel]->is_first_col())
			{
				texturePanels[selectedPanel]->set_scroll_duration(50.0);
				fastscroll = true;
			}
			else
				texturePanels[selectedPanel]->set_scroll_duration(180.0);
			texturePanels[selectedPanel]->PrevColumn();
		}
		else if (int r = g_ih->get_result(BUTTON_1UP) & result_mask)
		{
			if (texturePanels[selectedPanel]->PrevRow())
			{
				show_qam();
			}
		}
		else if (int r = g_ih->get_result(BUTTON_1DOWN) & result_mask)
		{
			if (texturePanels[selectedPanel]->NextRow())
			{
				int c = texturePanels[selectedPanel]->GetSelectedColumn();
				selectedPanel++;
				texturePanels[selectedPanel]->move_to_column(c);
			}
		}
		else if ((g_ih->get_result(BUTTON_1B) & result_mask) ||
			(g_ih->get_result(BUTTON_ESCAPE) & c_input_handler::RESULT_DOWN))
		{
			for (int i = 0; i < num_texture_panels; i++)
				texturePanels[i]->dim = true;
			c_menu::s_menu_items mi;

			char *m[] = {"quit nemulator", "suspend computer"};
			mi.num_items = show_suspend ? 2 : 1;
			mi.items = m;
			add_task(new c_menu(), (void*)&mi);
			menu = MENU_QUIT;
		}
		else if (g_ih->get_result(BUTTON_1A, true) & result_mask ||
			g_ih->get_result(BUTTON_1START, true) & result_mask ||
			g_ih->get_result(BUTTON_RETURN, true) & c_input_handler::RESULT_DOWN)
		{
			start_game();
		}
		else if (g_ih->get_result(BUTTON_1SELECT, true) & c_input_handler::RESULT_DOWN)
		{
			show_qam();
		}
	}
	//these keys work in game
	else
	{
		int result_mask = c_input_handler::RESULT_DOWN | c_input_handler::RESULT_REPEAT_SLOW | c_input_handler::RESULT_REPEAT_FAST | c_input_handler::RESULT_REPEAT_EXTRAFAST;

		if (g_ih->get_result(BUTTON_1A_TURBO) & c_input_handler::RESULT_DOWN)
		{
			do_turbo_press(BUTTON_1A, "1 A");
		}
		else if (g_ih->get_result(BUTTON_1B_TURBO) & c_input_handler::RESULT_DOWN)
		{
			do_turbo_press(BUTTON_1B, "1 B");
		}
		else if (g_ih->get_result(BUTTON_2A_TURBO) & c_input_handler::RESULT_DOWN)
		{
			do_turbo_press(BUTTON_1B, "2 A");
		}
		else if (g_ih->get_result(BUTTON_2B_TURBO) & c_input_handler::RESULT_DOWN)
		{
			do_turbo_press(BUTTON_1B, "2 B");
		}


		if ((g_ih->get_result(BUTTON_ESCAPE) & c_input_handler::RESULT_DOWN) ||
			(g_ih->get_hold_time(BUTTON_1START) >= menu_delay && g_ih->get_hold_time(BUTTON_1SELECT) >= menu_delay))
		{
			g_ih->ack_button(BUTTON_1START);
			g_ih->ack_button(BUTTON_1SELECT);
			for (int i = 0; i < num_texture_panels; i++)
				texturePanels[i]->dim = true;
			c_menu::s_menu_items mi;
			char *m[] = {"resume", "reset", "return to menu"};
			mi.num_items = 3;
			mi.items = m;
			add_task(new c_menu(), (int*)&mi);
			menu = MENU_INGAME;
			paused = true;
			sound->Stop();
		}
	}
}

void c_nemulator::show_qam()
{
	if (qam == NULL)
	{
		qam = new c_qam();
		qam->set_valid_chars(texturePanels[selectedPanel]->get_valid_chars());
		add_task(qam, NULL);

	}
	for (int i = 0; i < num_texture_panels; i++)
		texturePanels[i]->dim = true;

	qam->set_char(texturePanels[selectedPanel]->GetSelected()->get_description().c_str()[0]);

	qam->activate();
	menu = MENUS::MENU_QAM;
}

void c_nemulator::do_turbo_press(int button, std::string button_name)
{

	if (g_ih->get_result(BUTTON_LEFT_SHIFT) & c_input_handler::RESULT_DEPRESSED)
	{
		((c_nes_input_handler*)g_ih)->set_turbo_rate(button,
			((c_nes_input_handler*)g_ih)->get_turbo_rate(button) + 2);
	}
	else if (g_ih->get_result(BUTTON_RIGHT_SHIFT) & c_input_handler::RESULT_DEPRESSED)
	{
		((c_nes_input_handler*)g_ih)->set_turbo_rate(button,
			((c_nes_input_handler*)g_ih)->get_turbo_rate(button) - 2);
	}
	else
	{
		int turbo_state = ((c_nes_input_handler*)g_ih)->get_turbo_state(button);
		((c_nes_input_handler*)g_ih)->set_turbo_state(button, !turbo_state);
		status->add_message(button_name + " turbo " + (turbo_state ? "disabled" : "enabled"));
		return;
	}
	char message[64];
	sprintf_s(message, "%s turbo rate: %d/s", button_name.c_str(),
		60/((c_nes_input_handler*)g_ih)->get_turbo_rate(button));
	status->add_message(message);
}

void c_nemulator::leave_game()
{
	if (joy1 != NULL)
		*joy1 = 0;
	if (joy2 != NULL)
		*joy2 = 0;
	joy1 = NULL;
	joy2 = NULL;
	sound->Stop();
	c_console *n = ((Game*)texturePanels[selectedPanel]->GetSelected())->console;
	n->disable_mixer();
	inGame = false;
	for (int i = 0; i < num_texture_panels; i++)
		texturePanels[i]->Zoom();
	for (auto &game_thread : game_threads)
	{
		ResumeThread(game_thread->thread_handle);
	}
}

void c_nemulator::start_game()
{
	//todo: come back and fix has played logic
	Game *g = (Game*)texturePanels[selectedPanel]->GetSelected();
	c_console *n = g->console;
	if (n && n->is_loaded())
	{
		if (reset_on_select && !g->played)
		{
			n->set_emulation_mode(emulation_mode_ingame);
			n->reset();
			input_buffer_index = 0;
			input_buffer_playback = 0;
		}
		g->played = 1;
		//joy1 = n->GetJoy1();
		//joy2 = n->GetJoy2();
		//sound->Reset();
		sound->Clear();
		sound->Play();
		inGame = true;
		n->enable_mixer();
		for (int i = 0; i < num_texture_panels; i++)
			texturePanels[i]->Zoom();
		for (auto &game_thread : game_threads)
		{
			SuspendThread(game_thread->thread_handle);
		}
	}
}

int c_nemulator::update(double dt, int child_result, void *params)
{
	if (menu)
	{
		switch(menu)
		{
		case MENU_QAM:
			if (child_result == c_task::TASK_RESULT_RETURN)
			{
				texturePanels[selectedPanel]->move_to_char(*(char*)params);
				menu = 0;
			}
			else if (child_result == c_task::TASK_RESULT_CANCEL)
			{
				menu = 0;
			}
			if (menu == 0)
			{
				for (int i = 0; i < num_texture_panels; i++)
					texturePanels[i]->dim = false;
			}
			break;
		case MENU_QUIT:
			if (child_result == c_task::TASK_RESULT_RETURN)
			{
				switch (*(int*)params)
				{
				case 0: //quit
					dead = true;
					if (status)
						status->dead = true;
					if (stats)
						stats->dead = true;
					if (audio_info)
						audio_info->dead = true;
					if (mem_viewer)
						mem_viewer->dead = true;
					if (qam)
						qam->dead = true;
					return 0;
				case 1: //suspend
					SetSuspendState(FALSE, TRUE, FALSE);
					break;
				}
				menu = 0;
				for (int i = 0; i < num_texture_panels; i++)
					texturePanels[i]->dim = false;
			}
			else if (child_result == c_task::TASK_RESULT_CANCEL)
			{
				menu = 0;
				for (int i = 0; i < num_texture_panels; i++)
					texturePanels[i]->dim = false;
			}
			break;
		case MENU_SELECT:
			if (child_result == c_task::TASK_RESULT_RETURN)
			{
				switch (*(int*)params)
				{
				case 0: //play
					start_game();
					menu = 0;
					break;
				case 1: //quit
					dead = true;
					return 0;
				case 2: // return to menu
					break;
				}
				menu = 0;
				for (int i = 0; i < num_texture_panels; i++)
					texturePanels[i]->dim = false;
			} 
			else if (child_result == c_task::TASK_RESULT_CANCEL)
			{
				menu = 0;
				for (int i = 0; i < num_texture_panels; i++)
					texturePanels[i]->dim = false;
			}
			break;
		case MENU_INGAME:
			if (child_result == c_task::TASK_RESULT_RETURN)
			{
				switch (*(int*)params)
				{
				case 0: //resume
					break;
				case 1: //reset
					{
						c_console *n = ((Game*)texturePanels[selectedPanel]->GetSelected())->console;
						n->reset();
						input_buffer_index = 0;
						input_buffer_playback = 0;
						status->add_message("reset");
					}
					break;
				case 2: //return to main menu
					leave_game();
					break;
				}
				for (int i = 0; i < num_texture_panels; i++)
					texturePanels[i]->dim = false;
				paused = false;
				menu = 0;
				if (*(int*)params != 2)
					sound->Play();
			}
			else if (child_result == c_task::TASK_RESULT_CANCEL)
			{
				paused = false;
				for (int i = 0; i < num_texture_panels; i++)
					texturePanels[i]->dim = false;
				menu = 0;
				sound->Play();
			}
			break;
		}
	}
	if (splash_done && gameList.size() != 0) ProcessInput(dt);
	UpdateScene(dt);
	return 0;
}

void c_nemulator::UpdateScene(double dt)
{
	if (!splash_done)
	{
		switch (splash_stage)
		{
		case 0: //show title
			splash_stage++;
			break;
		case 1: //load games
			if (preload)
				load_thread_handle = CreateThread(NULL, 0, c_nemulator::load_thread, this, 0, NULL);
			else
				LoadGames();
			splash_stage++;
			break;
		case 2:
			splash_timer -= dt;
			if (splash_timer < 1000.0)
				splash_timer = 1000.0;
			if (loaded)
			{
				if (preload)
				{
					CloseHandle(load_thread_handle);
					load_thread_handle = 0;
				}
				if (splash_timer < 1000.0)
					splash_timer = 1000.0;
				splash_stage++;
			}
			break;
		case 3: //fade out
			splash_timer -= dt;
			if (splash_timer <= 0.0)
				splash_done = 1;
			break;
		default:
			break;
		}

		return;
	}

	static double elapsed = 0.0f;
	static int s = 0;

	bool changed = false;
	for (int i = 0; i < num_texture_panels; i++)
	{
		if (texturePanels[i]->Changed())
		{
			changed = true;
			break;
		}
	}

	if (changed)
	{
		GetEvents();
		updateEvents = false;
	}
	if (scroll_fade_timer > 0.0)
	{
		scroll_fade_timer -= dt;
		if (scroll_fade_timer < 0.0)
			scroll_fade_timer = 0.0;
	}
	if (!paused)
	{
		if (inGame)
		{
			c_console *nes = ((Game*)texturePanels[selectedPanel]->GetSelected())->console;
			//sound->Copy(nes->GetSoundBuf(), nes->GetSoundSamples());
			const short *sound_buf;
			int x = nes->get_sound_buf(&sound_buf);
			sound->Copy((short*)sound_buf, x);
			s = sound->Sync();
			nes->set_audio_freq(sound->get_requested_freq());
		}
		RunGames();

	}
#ifdef MEM_VIEWER
	if (mem_viewer)
	{
		c_nes *nes = ((Game*)texturePanels[selectedPanel]->GetSelected())->nes;
		if (nes->loaded)
			mem_viewer->set_mem_access_log(nes->mem_access_log);
		else
			mem_viewer->set_mem_access_log(NULL);
	}
#endif

	for (int i = 0; i < num_texture_panels; i++)
		texturePanels[i]->Update(dt);

	static int framesDrawn = 0;
	framesDrawn++;

	elapsed += dt;

	if (audio_info != NULL && elapsed >= 10.0)
	{
		if (inGame)
			audio_info->report((int)sound->GetFreq(), s, 0, sound->max_freq, sound->min_freq);
	}

	if (elapsed >= 250.0)
	{
		double fps = framesDrawn / (elapsed / 1000.0);
		fps_history[fps_index++ % fps_records] = fps;
		max_fps = 60.0;
		for (int i = 0; i < fps_records; i++)
			if (fps_history[i] > 60.0 && fps_history[i] > max_fps)
				max_fps = fps_history[i];
		//char x[32];
		//sprintf(x, "%.2f %.2f", fps, 1.0/fps*1000.0);
		//SetWindowText(hWnd, x);

		if (stats)
		{
			c_console *console = ((Game*)texturePanels[selectedPanel]->GetSelected())->console;
			stats->report_stat("fps", fps);
			//stats->report_stat("mapper #", nes->get_mapper_number());
			//stats->report_stat("mapper name", nes->get_mapper_name());
			stats->report_stat("freq", sound->GetFreq());
			stats->report_stat("audio position", s);
			stats->report_stat("audio resets", sound->resets);
			stats->report_stat("emulation mode", console->get_emulation_mode() == c_nes::EMULATION_MODE_ACCURATE ? "accurate" : "fast");
			//stats->report_stat("sprite limit", nes->get_sprite_limit() ? "limited" : "unlimited");
			stats->report_stat("audio.slope", sound->slope);
			std::ostringstream s;
			s << std::hex << std::uppercase << console->get_crc();
			stats->report_stat("CRC", s.str());

			//switch (nes->get_mirroring_mode())
			//{
			//case 0:
			//	stats->report_stat("mirroring", "horizontal");
			//	break;
			//case 1:
			//	stats->report_stat("mirroring", "vertical");
			//	break;
			//case 2:
			//case 3:
			//	stats->report_stat("mirroring", "one screen");
			//	break;
			//case 4:
			//	stats->report_stat("mirroring", "four screen");
			//	break;
			//default:
			//	break;
			//}

		}
		framesDrawn = 0;
		elapsed = 0.0;
	}
}

void c_nemulator::DrawText(ID3DX10Font *font, float x, float y, std::string text, D3DXCOLOR color)
{
	RECT r = {(LONG)(clientWidth * x), (LONG)(clientHeight * y), 0, 0};
	ID3D10DepthStencilState *state;
	int oldref;
	d3dDev->OMGetDepthStencilState(&state, (UINT *)&oldref);
	font->DrawText(NULL, text.c_str(), -1, &r, DT_NOCLIP, color);
	d3dDev->OMSetDepthStencilState(state, oldref);
}

void c_nemulator::draw()
{
	DrawScene();
}

void c_nemulator::DrawScene()
{
	if (!splash_done)
	{
		double alpha = 1.0f;
		if (splash_timer <= 1000.0)
		{
			alpha = (splash_timer / 1000.0) * 1.0;
		}
		RECT r = {0, 0, clientWidth, (long)(clientHeight*.8)};
		ID3D10DepthStencilState *state;
		int oldref;
		d3dDev->OMGetDepthStencilState(&state, (UINT *)&oldref);
		font1->DrawText(NULL, "nemulator", -1, &r, DT_NOCLIP | DT_CENTER | DT_VCENTER, D3DXCOLOR(1.0f, 1.0f, 1.0f, (float)alpha));
		if (!loaded && preload)
		{
			RECT r2 = {0, 0, clientWidth, (long)(clientHeight * 1.95)};
			char buf[256];
			sprintf_s(buf, 256, "Loading: %d", num_games);
			font2->DrawText(NULL, buf, -1, &r2, DT_NOCLIP | DT_CENTER | DT_VCENTER, D3DXCOLOR(.7f, .7f, .7f, 1.0));
		}
		d3dDev->OMSetDepthStencilState(state, oldref);
		return;
	}

	if (texturePanels[selectedPanel]->get_num_items() > 0)
	{
		Game *g = (Game*)texturePanels[selectedPanel]->GetSelected();
		for (int i = 0; i < num_texture_panels; i++)
			texturePanels[i]->Draw();

		if (texturePanels[selectedPanel]->state == TexturePanel::STATE_MENU || texturePanels[selectedPanel]->state == TexturePanel::STATE_SCROLLING)
		{
			double dim = mainPanel2->dim ? .25 : 1.0;
			DrawText(font1, .05f, .85f, g->title, D3DXCOLOR((float)(1.0f * dim), 0.0f, 0.0f, 1.0f));

			char subtitle[256];
			switch (g->type)
			{
			case GAME_NES:
				sprintf(subtitle, "Nintendo NES");
				break;
			case GAME_SMS:
				sprintf(subtitle, "Sega Master System");
				break;
			default:
				sprintf(subtitle, "");
				break;
			}

			DrawText(font2, .0525f, .925f, subtitle, D3DXCOLOR((float)(.5f * dim), (float)(.5f * dim), (float)(.5f * dim), 1.0f));
			RECT r = { 0, 0, clientWidth, (LONG)(clientHeight*1.95) };
			ID3D10DepthStencilState *state;
			int oldref;
			d3dDev->OMGetDepthStencilState(&state, (UINT *)&oldref);
			font2->DrawText(NULL, "www.nemulator.com", -1, &r, DT_NOCLIP | DT_CENTER | DT_VCENTER, D3DXCOLOR((float)(.1f*dim), (float)(.1f*dim), (float)(.1f*dim), 1.0f));
			d3dDev->OMSetDepthStencilState(state, oldref);
		}

		if (!inGame && !menu && (fastscroll || scroll_fade_timer > 0.0))
		{
			char s[2];
			sprintf_s(s, "%c", toupper(g->title[0]));
			RECT r = { 0, 0, clientWidth, (long)(clientHeight*.8) };
			ID3D10DepthStencilState *state;
			int oldref;
			d3dDev->OMGetDepthStencilState(&state, (UINT *)&oldref);
			font3->DrawText(NULL, s, -1, &r, DT_NOCLIP | DT_CENTER | DT_VCENTER, D3DXCOLOR(1.0f, 1.0f, 1.0f, .8f));
			d3dDev->OMSetDepthStencilState(state, oldref);
		}

		if (inGame && g->type == GAME_NES && g->console->get_crc() == 0x0B0E128F)
		{
			char time[6];
			//int nwc_time = n->get_nwc_time();
			int nwc_time = 0;
			sprintf_s(time, "%2d:%.2d", nwc_time / 60, nwc_time % 60);
			RECT r = { (LONG)(clientWidth * .02), 0, clientWidth, (LONG)(clientHeight*1.95) };
			ID3D10DepthStencilState *state;
			int oldref;
			d3dDev->OMGetDepthStencilState(&state, (UINT *)&oldref);
			font2->DrawText(NULL, time, -1, &r, DT_NOCLIP | DT_VCENTER, D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f));
			d3dDev->OMSetDepthStencilState(state, oldref);
		}
	}
	else
	{
		DrawText(font1, .05f, .5f, "No roms found\nCheck paths in nemulator.ini\nDefault NES path: c:\\roms\\nes\nDefault SMS path: c:\\roms\\sms", D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f));
	}
}
void c_nemulator::on_pause(bool paused)
{
	OnPause(paused);
}
void c_nemulator::OnPause(bool paused)
{
	if (inGame)
	{
		if (paused)
			sound->Stop();
		else if (menu != MENU_INGAME)
			sound->Play();
	}
}

void c_nemulator::GetEvents()
{
	if (done_events)
	{
		delete[] done_events;
		done_events = 0;
	}

	std::list<TexturePanelItem*> panelItems;
	for (int i = 0; i < num_texture_panels; i++)
		texturePanels[i]->GetActive(&panelItems);
	panelItems.sort();
	panelItems.unique();

	if (panelItems.size() > 0)
	{
		done_events = new HANDLE[game_threads.size()];
		int x = 0;
		int y = 0;
		for (auto &game_thread : game_threads)
		{
			game_thread->game_list.clear();
			done_events[y++] = game_thread->done_event;
		}

		y = 0;
		for (auto &i : panelItems)
		{
			game_threads[y % game_threads.size()]->game_list.push_back((Game*)i);
			y++;
		}
	}
}

DWORD WINAPI c_nemulator::load_thread(LPVOID param)
{
	c_nemulator *n = (c_nemulator *)param;
	n->LoadGames();
	return 0;
}

void c_nemulator::LoadGames()
{
	struct s_loadinfo
	{
		GAME_TYPE type;
		char *extension;
		char *rom_path;
		char *save_path;
	};

	char nes_rom_path[MAX_PATH];
	char nes_save_path[MAX_PATH];
	char sms_rom_path[MAX_PATH];
	char sms_save_path[MAX_PATH];
	
	strcpy_s(nes_rom_path, MAX_PATH, config->get_string("nes.rom_path", "c:\\roms\\nes").c_str());
	strcpy_s(nes_save_path, MAX_PATH, config->get_string("nes.save_path", nes_rom_path).c_str());

	strcpy_s(sms_rom_path, MAX_PATH, config->get_string("sms.rom_path", "c:\\roms\\sms").c_str());
	strcpy_s(sms_save_path, MAX_PATH, config->get_string("sms.save_path", sms_rom_path).c_str());

	s_loadinfo loadinfo[] = 
	{
		{ GAME_NES, "nes", nes_rom_path, nes_save_path },
		{ GAME_SMS, "sms", sms_rom_path, sms_save_path }
	};

//	char *extensions[] = { "nes", "sms" };
	_finddata64i32_t fd;
	intptr_t f;


	if (!dir_exists(nes_save_path))
	{
		strcpy_s(nes_save_path, MAX_PATH, nes_rom_path);
	}
	if (!dir_exists(sms_save_path))
	{
		strcpy_s(sms_save_path, MAX_PATH, sms_rom_path);
	}

	bool global_mask_sides = config->get_bool("mask_sides", false);
	bool global_limit_sprites = config->get_bool("limit_sprites", false);

	for (auto &li : loadinfo)
	{
		char searchPath[MAX_PATH];
		sprintf_s(searchPath, MAX_PATH, "%s\\*.%s", li.rom_path, li.extension);
		if ((f = _findfirst(searchPath, &fd)) != -1)
		{
			int find_result = 0;
			while (find_result == 0)
			{
				num_games++;
				char filename[MAX_PATH];
				sprintf_s(filename, MAX_PATH, "%s\\%s", li.rom_path, fd.name);

				if (preload)
				{
					char *buf = new char[65536];
					std::ifstream file;
					file.open(filename, std::ios_base::in | std::ios_base::binary);
					if (file.is_open())
					{
						while (file.read(buf, 65536));
						file.close();
					}
					delete[] buf;
				}

				Game *g = new Game(li.type, li.rom_path, fd.name, li.save_path);
				g->set_description(fd.name);
				g->emulation_mode = emulation_mode_menu;
				std::string s;
				s += "\"";
				s += fd.name;
				s += "\".";
				bool mask_sides = config->get_bool(s + "mask_sides", global_mask_sides);
				bool limit_sprites = config->get_bool(s + "limit_sprites", global_limit_sprites);
				int submapper = config->get_int(s + "mapper_variant", 0);
				g->submapper = submapper;
				g->mask_sides = mask_sides;
				g->limit_sprites = limit_sprites;
				gameList.push_back(g);
				find_result = _findnext(f, &fd);
			}
		}

		_findclose(f);
	}
	std::sort(gameList.begin(), gameList.end(), [](const Game* a, const Game* b) {
		std::string aa = a->filename;
		std::string bb = b->filename;
		for (auto & a_upper : aa) a_upper = toupper(a_upper);
		for (auto & b_upper : bb) b_upper = toupper(b_upper);
		return aa < bb;
	});
	for (auto &game : gameList)
	{
		mainPanel2->AddItem(game);
	}

	loaded = 1;
	
}

int c_nemulator::dir_exists(char *path)
{
	char temp[MAX_PATH];
	strcpy_s(temp, MAX_PATH, path);
	int len = strlen(temp);
	char *c = temp + len - 1;
	while (*c == '\\')
		*c-- = 0;

	struct stat s;

	if (0 == stat(path, &s))
		return 1;
	else
		return 0;
}

int c_nemulator::take_screenshot()
{
	bool mask_sides = ((Game*)texturePanels[selectedPanel]->GetSelected())->mask_sides;
	Game *g = (Game*)texturePanels[selectedPanel]->GetSelected();
	c_console *n = g->console;
	if (n->is_loaded())
	{
		std::string screenshot_path = config->get_string("screenshot_path", "c:\\roms\\screenshots");
		_finddata64i32_t fd;
		int f;
		char search_path[MAX_PATH];
		sprintf_s(search_path, MAX_PATH, (screenshot_path + "\\" + n->filename + ".???.bmp").c_str());

		int file_number;
		if ((f=(int)_findfirst(search_path, &fd)) != -1)
		{
			int find_result = 0;
			std::list<std::string> file_list;
			while(find_result == 0)
			{
				file_list.push_back(fd.name);
				find_result = _findnext(f, &fd);
			}
			file_list.sort();
			int length = strlen(file_list.back().c_str());
			char temp[8];
			strcpy_s(temp, file_list.back().c_str() + length-7);
			temp[3] = '\0';
			file_number = atoi(temp) + 1;
			if (file_number > 999)
				return 1;
			_findclose(f);
		}
		else
			file_number = 0;

		char temp[4];
		sprintf_s(temp, 4, "%03d", file_number);

		std::string filename = screenshot_path + "\\" + n->filename + "." + temp + ".bmp";

		int *buf = new int[256*240];
		int *b_ptr = buf;
		int *fb = n->get_video();
		for (int y = 0; y < 240; y++)
		{
			for (int x = 0; x < 256; x++)
			{
				int col = 0;
				if (mask_sides && (x < 8 || x > 247))
				{
					fb++;
					col = 0;
				}
				else
					switch (g->type)
					{
					case GAME_NES:
						col = pal[*fb++ & 0x1FF];
						break;
					case GAME_SMS:
						col = *fb++;
						break;
					}
					//col = pal[*fb++ & 0x1FF];
				{
					int r = col & 0xFF;
					int g = (col >> 8) & 0xFF;
					int b = (col >> 16) & 0xFF;
					r = 255.0 * pow(r/255.0, 1.0 / 2.2);
					g = 255.0 * pow(g/255.0, 1.0 / 2.2);
					b = 255.0 * pow(b/255.0, 1.0 / 2.2);
					col = 0xFF000000 | (b << 16) | (g << 8) | r;
				}
				*b_ptr++ = col;
			}
		}

		int ret = 0;
		switch (g->type)
		{
		case GAME_NES:
			ret = c_bmp_writer::write_bmp(buf + 256 * 8, 256, 224, filename);
			break;
		case GAME_SMS:
			ret = c_bmp_writer::write_bmp(buf + 256 * 8, 256, 192, filename);
			break;
		}
		delete[] buf;
		if (ret == 0)
			return 0;
		else
			return 1;
	}
	return 1;
}