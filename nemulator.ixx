module;
#include "Windows.h"
#include "d3d10.h"
#include "d3dx10.h"

#include <vector>
#include <list>
#include <numbers>
#include <filesystem>
export module nemulator;
import :qam;
import :stats;
import :nsf_stats;
import :audio_info;
import :status;
import :system_container;

import D3d10App;
import task;
import config;
import texture_panel;
import input_handler;
import sound;

export class c_nemulator :
    public c_task
{
public:
    c_nemulator();
    ~c_nemulator();

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

private:
    float sharpness;
    static const GUID nemulator_scheme_guid;
    static DWORD WINAPI game_thread(LPVOID lpParam);
    int init_threads();
    void kill_threads();
    void show_qam();
    void do_turbo_press(int button, std::string button_name);
    int splash_done;
    int splash_stage;
    double splash_timer;
    double splash_fade_timer;
    void configure_input();
    void adjust_sharpness(float value);

    struct s_button_handler_params {
        int button;
        int result;
    };
    
    void handle_button_reset(s_button_handler_params *params);
    void handle_button_audio_info(s_button_handler_params* params);
    void handle_button_stats(s_button_handler_params* params);
    void handle_button_mask_sides(s_button_handler_params* params);
    void handle_button_sprite_limit(s_button_handler_params* params);
    void handle_button_dec_sharpness(s_button_handler_params* params);
    void handle_button_inc_sharpness(s_button_handler_params* params);
    void handle_button_menu_right(s_button_handler_params* params);
    void handle_button_menu_left(s_button_handler_params* params);
    void handle_button_menu_up(s_button_handler_params* params);
    void handle_button_menu_down(s_button_handler_params* params);
    void handle_button_menu_cancel(s_button_handler_params* params);
    void handle_button_menu_ok(s_button_handler_params* params);
    void handle_button_show_qam(s_button_handler_params* params);
    void handle_button_turbo(s_button_handler_params* params);
    void handle_button_leave_game(s_button_handler_params* params);
    void handle_button_switch_disk(s_button_handler_params *params);

    float fov_h;
    float eye_x;
    float eye_y;
    float eye_z;

    int loaded;
    int num_games;
    int rom_count;

    bool preload;

    static DWORD WINAPI load_thread(LPVOID param);
    void *load_thread_handle;

    c_stats *stats;
    c_nsf_stats* nsf_stats;
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

    void start_game();
    void leave_game();
    int menu;
    bool fastscroll;
    double scroll_fade_timer;
    void LoadFonts();
    void DrawText(ID3DX10Font *font, float x, float y, std::string text, D3DXCOLOR color);
    void OnPause(bool paused);
    void GetEvents();

    void RunGames();
    void ProcessInput(double dt);
    int selectedPanel;
    std::vector<c_system_container*> gameList;
    double menu_delay;

    bool inGame;
    std::unique_ptr<c_sound> sound;

    std::unique_ptr<c_texture_panel> mainPanel2;
    static const int num_texture_panels = 1;
    c_texture_panel *texturePanels[num_texture_panels];

    HRESULT hr;

    D3DXMATRIX matrixWorld;
    
    ID3D10EffectShaderResourceVariable *varTex;
    ID3D10ShaderResourceView *texRv;

    ID3D10Texture2D *tex;

    ID3DX10Font *font1;
    ID3DX10Font *font2;
    ID3DX10Font *font3;

    LARGE_INTEGER liFreq;
    LARGE_INTEGER liCurrent;
    LARGE_INTEGER liLast;


    double max_fps;
    static const int fps_records = 4;
    double fps_history[fps_records];
    int fps_index;

    bool paused;

    c_audio_info *audio_info;
    c_qam *qam;

    struct s_game_thread
    {
        HANDLE thread_handle;
        HANDLE start_event;
        HANDLE done_event;
        int kill;
        std::vector<c_system_container*> game_list;
    };
    std::vector<std::unique_ptr<s_game_thread>> game_threads;
    int num_threads;
    std::unique_ptr<HANDLE[]> done_events;

    bool show_suspend;

    typedef void (c_nemulator::* button_handler_func)(s_button_handler_params*);
    struct s_button_handler {
        int scope;
        //int button;
        std::vector<int> button_list;
        bool ack;
        int mask;
        button_handler_func func;
    };
    static const int RESULT_DOWN = c_input_handler::RESULT_DOWN;
    static const int RESULT_DOWN_OR_REPEAT = c_input_handler::RESULT_DOWN | c_input_handler::RESULT_REPEAT_SLOW | c_input_handler::RESULT_REPEAT_FAST | c_input_handler::RESULT_REPEAT_EXTRAFAST;
    enum SCOPE {
        ANYTIME = 1,
        IN_MENU = 2,
        IN_GAME = 4,
        GAMES_LOADED = 8,
        NO_GAMES_LOADED = 16
    };
    static const s_button_handler button_handlers[];

    const double SPLASH_TIMER_TOTAL_DURATION = 1000.0;
    const double SPLASH_TIMER_FADE_DURATION = 500.0;
    unsigned int benchmark_frame_count = 0;

    static constexpr double fovy = std::numbers::pi / 4.0;
};
