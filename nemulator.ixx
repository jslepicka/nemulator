module;
#include "Windows.h"
#include "d3d10.h"
#include "d3dx10.h"

#include <vector>
#include <list>
#include <numbers>
#include <filesystem>
#include <chrono>
export module nemulator;
import :qam;
import :stats;
import :nsf_stats;
import :audio_info;
import :status;
import :disk_indicator;
import :system_container;
import :input_config;

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
    //writes a nemulator.ini documenting every setting at its default value
    static bool write_default_config(const std::string &filename);

private:
    float sharpness;
    //used when nemulator.ini doesn't set them, and by reset to defaults in the display menu
    static constexpr float default_sharpness = .8f;
    static constexpr bool default_scanlines = true;
    static constexpr float sharpness_step = .025f;
    static constexpr int default_menu_columns = 8;
    static constexpr double default_menu_delay = 333.0;
    static constexpr bool default_preload = true;
    static constexpr bool default_show_suspend = false;
    static constexpr bool default_limit_sprites = false;
    static constexpr bool default_disk_indicator = true;
    static constexpr const char *default_rom_path = "c:\\roms\\"; //the system's identifier is appended
    static constexpr const char *default_arcade_rom_path = "c:\\roms\\arcade";
    static const GUID nemulator_scheme_guid;
    static DWORD WINAPI game_thread(LPVOID lpParam);
    int init_threads();
    void kill_threads();
    void show_qam();
    void set_system_filter(int filter);
    void add_games_to_panel();
    void do_turbo_press(int button, std::string button_name);
    int splash_done;
    int splash_stage;
    double splash_timer;
    double splash_fade_timer;
    void configure_input();
    void adjust_sharpness(float value);
    void set_sharpness(float value);
    void set_scanlines(bool enabled);
    static std::string format_sharpness(float value);
    void adjust_volume(int value);

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
    void handle_button_volume_up(s_button_handler_params *params);
    void handle_button_volume_down(s_button_handler_params *params);
    void handle_button_scanlines(s_button_handler_params *params);

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
    c_disk_indicator *disk_indicator;
    enum MENUS
    {
        MENU_SELECT = 1,
        MENU_INGAME,
        MENU_INGAME_OPTIONS,
        MENU_QUIT,
        MENU_CHEAT,
        MENU_QAM,
        MENU_SETTINGS,
        MENU_INPUT_CONFIG,
        MENU_DISPLAY,
        MENU_GENERAL,
        MENU_SYSTEM,
        MENU_SYSTEM_OPTIONS
    };

    //each menu opens with the item at selected (or the item for selected_action) highlighted, so returning
    //from a submenu leaves the cursor on the item that opened it
    void show_quit_menu(int selected = 0);
    void show_settings_menu(int selected = 0);
    void show_ingame_menu(int selected_action = INGAME_RESUME);
    enum INGAME_ACTION //in-game menu items, in the order they're listed
    {
        INGAME_RESUME,
        INGAME_SWITCH_DISK,
        INGAME_RESET,
        INGAME_SETTINGS,
        INGAME_RETURN_TO_MENU
    };
    std::vector<int> ingame_menu_actions; //the action for each item in the current in-game menu
    void show_display_menu();
    void save_display_settings();
    void show_general_menu();
    void save_general_settings();
    //saved when the general menu closes, if they changed
    int sync_mode_at_open;
    bool pause_on_lost_focus_at_open;

    //settings > system, for each group of systems that shares an input identifier (e.g., NES and FDS),
    //saved as <identifier>.<setting>.  only systems with a setting to change are listed.
    struct s_system_settings
    {
        std::string identifier;
        std::string name;
        bool has_sprite_limit = false;
        bool limit_sprites = default_limit_sprites;
        bool has_disk_indicator = false;
        bool disk_indicator = default_disk_indicator;
    };
    std::vector<s_system_settings> system_settings;
    int system_settings_index; //the system whose options menu is open
    s_system_settings system_settings_at_open; //settings that differ from these are saved when it closes
    void load_system_settings();
    void show_system_menu(int selected = 0);
    void show_system_options_menu();
    void save_system_settings();
    //applies a system's settings to its games, including any that are running
    void apply_system_settings(const s_system_settings &settings);
    //the game has a disk indicator, and it's turned on in settings > system
    bool disk_indicator_enabled(c_system_container *g);
    std::string startup_message; //shown once the splash screen is done
    //the audio stream paces frames in the audio sync mode, so it runs in the menu as well as in a game
    void update_audio_stream();
    bool app_paused; //the application lost focus, as opposed to the in-game menu pausing a game
    bool settings_in_game; //the settings menu was opened from the in-game menu
    struct s_display_settings
    {
        float sharpness;
        bool scanlines;
        bool fullscreen;
    } display_settings_at_open; //settings that differ from these are saved when the display menu closes
    void start_game();
    void leave_game();
    int menu;
    bool fastscroll;
    double scroll_fade_timer;

    //titles too long for the window scroll left until fully visible, then back, pausing before each scroll
    enum TITLE_SCROLL
    {
        TITLE_SCROLL_WAIT_START,
        TITLE_SCROLL_LEFT,
        TITLE_SCROLL_WAIT_END,
        TITLE_SCROLL_RIGHT
    };
    void update_title_scroll(double dt);
    c_system_container *title_scroll_game; //the game whose title is being scrolled
    int title_scroll_state;
    double title_scroll_timer;
    double title_scroll_offset; //pixels
    int title_overflow; //pixels of the title that don't fit between the margins
    static constexpr double title_margin = .05; //fraction of the client width on either side of the title
    static constexpr double title_scroll_delay = 2000.0; //ms
    static constexpr double title_scroll_speed = .10; //client widths per second
    static constexpr double title_scroll_min_duration = 500.0; //ms, so short scrolls don't look like a jump
    void LoadFonts();
    void DrawText(ID3DX10Font *font, float x, float y, std::string text, D3DXCOLOR color);
    void OnPause(bool paused);
    void GetEvents();

    void RunGames();
    void ProcessInput(double dt);
    int selectedPanel;
    std::vector<c_system_container*> gameList;
    std::vector<std::string> system_filters; //"All" followed by each system that has games
    int system_filter; //index into system_filters of the systems shown in the menu
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
    c_input_bindings input_bindings;

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

    float master_volume;

    using clock = std::chrono::high_resolution_clock;
    std::chrono::steady_clock::time_point start;
};
