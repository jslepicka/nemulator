#pragma once
//#include "d3d10app.h"
#include "nes\nes.h"
#include "nes\palette.h"
#include "sms\sms.h"
#include "game.h"
#include <vector>
#include <list>
#include <stack>
#include <fstream>
#include <io.h>
#include "constants.h"
#include "TexturePanel.h"
#include "TexturePanelItem.h"
#include "sound.h"
#include "nes_input_handler.h"
#include "config.h"
#include "task.h"
#include <deque>
#include "task1.h"
#include "d3d10app.h"
#include "stats.h"
#include "status.h"
#include "audio_info.h"
#include "bmp_writer.h"
#include "mem_viewer.h"
#include "qam.h"


#define KEYDOWN(key) GetAsyncKeyState(key) & 0x8000 ? 1 : 0

class Nemulator :
	public c_task
{
public:
	Nemulator();
	~Nemulator(void);

	void init(void *params);
	void draw();
	int update(double dt, int child_result, void *params);
	void Init();
	void OnResize();
	void UpdateScene(double dt);
	void DrawScene();
	void resize();
	void on_pause(bool paused);
	void LoadGames();
	//void OnKeyUp(WPARAM wParam);

private:
	float sharpness;
	static const GUID nemulator_scheme_guid;
	static DWORD WINAPI game_thread(LPVOID lpParam);
	int init_threads();
	void kill_threads();
	void show_qam();
	static const int input_buffer_size = 60*60*240;
	char input_buffer[input_buffer_size];
	int input_buffer_index;
	int input_buffer_playback;
	int input_buffer_end;
	void do_turbo_press(int button, std::string button_name);
	int take_screenshot();
	int splash_done;
	int splash_stage;
	double splash_timer;
	double splash_fade_timer;
	void configure_input();

	float fov_h;
	float eye_x;
	float eye_y;
	float eye_z;

	int loaded;
	int num_games;

	bool preload;

	static DWORD WINAPI load_thread(LPVOID param);
	void *load_thread_handle;

	c_stats *stats;
	c_status *status;
	enum MENUS
	{
		MENU_SELECT = 1,
		MENU_INGAME,
		MENU_INGAME_OPTIONS,
		MENU_QUIT,
		MENU_CHEAT,
		MENU_QAM
	};

	std::deque<int> menu_stack;

	void start_game();
	void leave_game();
	int menu;
	c_nes_input_handler *ih;
	char romPath[MAX_PATH];
	bool fastscroll;
	double scroll_fade_timer;
	void LoadFonts();
	void DrawText(ID3DX10Font *font, float x, float y, std::string text, D3DXCOLOR color);
	void OnPause(bool paused);
	void GetEvents();

	//void InitPalette();
	void generate_palette();
	int wave(int p, int color);
	int clamp(int v, int min, int max);
	double gammafix(double f);
	void RunGames();
	void ProcessInput(double dt);
	int selectedPanel;
	std::vector<Game*> gameList;
	bool reset_on_select;
	int emulation_mode_menu;
	int emulation_mode_ingame;
	double menu_delay;

	bool inGame;
	Sound *sound;
	//bool loaded;

	unsigned char *joy1, *joy2;

	TexturePanel *mainPanel2;
	TexturePanel *favoritesPanel2;
	static const int num_texture_panels = 1;
	TexturePanel *texturePanels[num_texture_panels];

	HRESULT hr;

	D3DXMATRIX matrixWorld;
	c_nes *nes;

	ID3D10EffectShaderResourceVariable *varTex;
	ID3D10ShaderResourceView *texRv;

	ID3D10Texture2D *tex;


	ID3DX10Font *font1;
	ID3DX10Font *font2;
	ID3DX10Font *font3;

	LARGE_INTEGER liFreq;
	LARGE_INTEGER liCurrent;
	LARGE_INTEGER liLast;



	HANDLE *startEvents;
	HANDLE *doneEvents;
	int runnableCount;

	double max_fps;
	static const int fps_records = 4;
	double fps_history[fps_records];
	int fps_index;

	//static const unsigned char Palette[64*3];
	bool paused;

	static DWORD WINAPI NesThread(LPVOID lpParam);

	c_audio_info *audio_info;
	c_mem_viewer *mem_viewer;
	c_qam *qam;

	bool disable_throttle;
	bool changed_power_scheme;
	GUID *power_scheme;
	GUID duplicate_scheme;
	GUID *dup_scheme;
	DWORD prev_throttle_ac;
	DWORD prev_throttle_dc;

	void change_power_scheme();
	void revert_power_scheme();

	int dir_exists(char *path);

	struct s_game_thread
	{
		HANDLE thread_handle;
		HANDLE start_event;
		HANDLE done_event;
		int kill;
		std::vector<Game*> game_list;
	};
	std::vector<s_game_thread*> game_threads;
	int num_threads;
	HANDLE *done_events;
	int num_items_this_frame;

	bool show_suspend;
};
