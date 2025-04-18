module;
#include "Windows.h"
#include "PowrProf.h"
#include "time.h"
#include <sstream>
#include <xmmintrin.h>
#include <algorithm>
#include <pmmintrin.h>
#include "benchmark.h"
#include <string_view>
#include <ranges>
#include "d3d10.h"
#include "D3DX10.h"
#include <filesystem>

#define ReleaseCOM(x) { if(x) {x->Release(); x = 0; } }

module nemulator;

import system;
import nemulator.menu;
import nes;

extern ID3D10Device *d3dDev;
extern D3DXMATRIX matrixView;
extern D3DXMATRIX matrixProj;
extern int clientHeight;
extern int clientWidth;
extern bool startFullscreen;
extern bool aspectLock;
extern double aspectRatio;
extern HWND hWnd;
extern const char *app_title;

extern c_config *config;
extern std::unique_ptr<c_input_handler> g_ih;

HANDLE g_start_event;

c_nemulator::c_nemulator()
{
    done_events = NULL;
    selectedPanel = 0;
    inGame = false;
    sound = 0;
    font1 = 0;
    font2 = 0;
    font3 = 0;
    max_fps = 60.0;
    for (int i = 0; i < fps_records; i++)
        fps_history[i] = 0.0;
    fps_index = 0;
    menu = 0;
    paused = false;
    g_start_event = CreateEvent(NULL, TRUE, TRUE, NULL);
    stats = NULL;
    nsf_stats = NULL;
    audio_info = NULL;
    qam = NULL;
    splash_done = 0;
    splash_timer = SPLASH_TIMER_TOTAL_DURATION;
#if defined(DEBUG)
    splash_timer = 1.0f;
#endif
    splash_fade_timer = 1.0;
    splash_stage = 0;
    loaded = 0;
    num_games = 0;
    rom_count = 0;
    preload = false;
    load_thread_handle = 0;
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
    }
    game_threads.clear();
}

c_nemulator::~c_nemulator()
{
    ReleaseCOM(font1);
    ReleaseCOM(font2);
    ReleaseCOM(font3);
    kill_threads();
    for (auto &game : gameList)
    {
        delete game;
    }
}

void c_nemulator::LoadFonts()
{
    struct s_fonts {
        ID3DX10Font **font;
        double scale;
        unsigned int weight = 0;
    };

    s_fonts fonts[] = {
        {&font1, .08},
        {&font2, .04},
        {&font3, .9, FW_SEMIBOLD}
    };


    D3DX10_FONT_DESC fontDesc;

    for (auto f : fonts) {
        ReleaseCOM((*f.font));
        fontDesc = { 0 };
        fontDesc.Height = (int)(clientHeight * f.scale);
        fontDesc.Width = 0;
        fontDesc.Weight = f.weight;
        fontDesc.MipLevels = 1;
        fontDesc.Italic = false;
        fontDesc.CharSet = DEFAULT_CHARSET;
        fontDesc.OutputPrecision = OUT_DEFAULT_PRECIS;
        fontDesc.Quality = DEFAULT_QUALITY;
        fontDesc.PitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
        strcpy_s(fontDesc.FaceName, "Calibri");
        hr = D3DX10CreateFontIndirect(d3dDev, &fontDesc, f.font);
        if (hr != S_OK) {
            MessageBox(hWnd, "Unable to load fonts", "Error", MB_OK);
            exit(1);
        }
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
            if (g->system && g->system->is_loaded())
                g->system->emulate_frame();
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
        //s_game_thread *t = new s_game_thread;
        auto t = std::make_unique<s_game_thread>();
        t->start_event = CreateEvent(NULL, FALSE, FALSE, NULL);
        t->done_event = CreateEvent(NULL, FALSE, FALSE, NULL);
        t->kill = 0;
        t->thread_handle = CreateThread(NULL, 0, c_nemulator::game_thread, t.get(), 0, NULL);
        num_threads++;
        game_threads.push_back(std::move(t));
    }

    return 0;
}
void c_nemulator::Init()
{
    status = new c_status();
    add_task(status, NULL);

    LoadFonts();
    sound = std::make_unique<c_sound>();
    if (!sound->init()) {
        MessageBox(NULL, "Sound initialization failed", "Error", MB_OK);
        exit(1);
    }

    fastscroll = false;
    scroll_fade_timer = 0.0;

    int panel_columns = config->get_int("menu_columns", 8);
    if (panel_columns < 3)
        panel_columns = 3;
    int panel_rows = (int)((panel_columns-1) * c_texture_panel::tile_width / ((double)clientWidth/clientHeight) * .82 / c_texture_panel::tile_height); //not sure where .82 comes from...

    mainPanel2 = std::make_unique<c_texture_panel>(panel_rows, panel_columns);
    mainPanel2->x = 0.0f;
    mainPanel2->y = 0.0f;
    mainPanel2->z = 0.0f;
    mainPanel2->in_focus = true;
    sharpness = (float)std::clamp(config->get_double("sharpness", .8), 0.0, 1.0);
    mainPanel2->set_sharpness(sharpness);
    texturePanels[0] = mainPanel2.get();


    QueryPerformanceFrequency(&liFreq);

    //center x, y on panel center
    eye_x = (float)(((panel_columns - 1) * c_texture_panel::tile_width) / 2);
    eye_y = (float)((((panel_rows - 1) * c_texture_panel::tile_height) / 2) * -1.2);  //y is offset -20% to shift panel up on display


    //given a vertical field of view of 45 degrees, and clientWidth/clientHeight aspect ratio,
    //determine the horizontal field of view
    fov_h = (float)(2.0 * atan(((double)clientWidth/clientHeight) * tan( fovy/2.0) ));

    //using the horizontal field of view, determine how far back to place the camera
    eye_z = -(eye_x/tan(fov_h/2));

    mainPanel2->zoomDestX = eye_x;
    mainPanel2->zoomDestY = eye_y;
    mainPanel2->zoomDestZ = (float)(eye_z + (1.0 / tan( fovy/2.0 )));
    auto eye = D3DXVECTOR3(eye_x, eye_y, eye_z);
    auto at = D3DXVECTOR3(eye_x, eye_y, 0.0f);
    auto up = D3DXVECTOR3(0.0f, 1.0f, 0.0f);
    D3DXMatrixLookAtLH(
        &matrixView,
        &eye,
        &at,
        &up);

    menu_delay = config->get_double("menu_delay", 333.0);

    preload = config->get_bool("preload", true);

    show_suspend = config->get_bool("show_suspend", false);

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
        unsigned int default_key;
        int repeat_mode;
    };
    static const unsigned int BUTTON = 0x80000000;
    s_button_map button_map[] =
    {
        { BUTTON_1LEFT,         "joy1",     "left",    VK_LEFT,                1 },
        { BUTTON_1RIGHT,        "joy1",     "right",   VK_RIGHT,               1 },
        { BUTTON_1UP,           "joy1",     "up",      VK_UP,                  1 },
        { BUTTON_1DOWN,         "joy1",     "down",    VK_DOWN,                1 },
        { BUTTON_1A,            "joy1",     "a",       88,                     0 },
        { BUTTON_1A_TURBO,      "joy1",     "a_turbo", 0x53,                   0 },
        { BUTTON_1B,            "joy1",     "b",       90,                     0 },
        { BUTTON_1B_TURBO,      "joy1",     "b_turbo", 0x41,                   0 },
        { BUTTON_1SELECT,       "joy1",     "select",  VK_OEM_4,               0 },
        { BUTTON_1START,        "joy1",     "start",   VK_OEM_6,               0 },
        { BUTTON_SMS_PAUSE,     "joy1.sms", "pause",   BUTTON | BUTTON_1START, 0 },

        { BUTTON_2LEFT,         "joy2",     "left",    0,                      1 },
        { BUTTON_2RIGHT,        "joy2",     "right",   0,                      1 },
        { BUTTON_2UP,           "joy2",     "up",      0,                      1 },
        { BUTTON_2DOWN,         "joy2",     "down",    0,                      1 },
        { BUTTON_2A,            "joy2",     "a",       0,                      0 },
        { BUTTON_2A_TURBO,      "joy2",     "a_turbo", 0,                      0 },
        { BUTTON_2B,            "joy2",     "b",       0,                      0 },
        { BUTTON_2B_TURBO,      "joy2",     "b_turbo", 0,                      0 },
        { BUTTON_2SELECT,       "joy2",     "select",  0,                      0 },
        { BUTTON_2START,        "joy2",     "start",   0,                      0 },


        { BUTTON_STATS,          "",        "",        VK_F9,                  0 },
        { BUTTON_MASK_SIDES,     "",        "",        VK_F8,                  0 },
        { BUTTON_SPRITE_LIMIT,   "",        "",        VK_F7,                  0 },
        { BUTTON_AUDIO_INFO,     "",        "",        VK_F4,                  0 },
        { BUTTON_RESET,          "",        "",        VK_F2,                  0 },
        { BUTTON_LEFT_SHIFT,     "",        "",        VK_LSHIFT,              0 },
        { BUTTON_RIGHT_SHIFT,    "",        "",        VK_RSHIFT,              0 },
        { BUTTON_ESCAPE,         "",        "",        VK_ESCAPE,              0 },
        { BUTTON_RETURN,         "",        "",        VK_RETURN,              0 },
        { BUTTON_DEC_SHARPNESS,  "",        "",        0x39,                   1 },
        { BUTTON_INC_SHARPNESS,  "",        "",        0x30,                   1 },

        { BUTTON_1COIN,          "",        "",        0x31,                   0 },
        { BUTTON_SWITCH_DISK,    "",        "",        VK_F5,                  0 },
    };

    int num_buttons = sizeof(button_map) / sizeof(s_button_map);
    g_ih = std::make_unique<c_input_handler>(BUTTON_COUNT);
    g_ih->set_pair(BUTTON_1LEFT, BUTTON_1RIGHT);
    g_ih->set_pair(BUTTON_1UP, BUTTON_1DOWN);
    g_ih->set_pair(BUTTON_2LEFT, BUTTON_2RIGHT);
    g_ih->set_pair(BUTTON_2UP, BUTTON_2DOWN);
    
    for (int i = 0; i < num_buttons; i++)
    {
        int j = i;
        int default_key = button_map[i].default_key;
        int button = button_map[i].button;
        std::string key = button_map[i].config_base + "." + button_map[i].config_name;
        if (default_key & BUTTON)
        {
            default_key &= BUTTON - 1;
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
    D3DXMatrixPerspectiveFovLH(&matrixProj, (float)fovy, aspect, 1.0f, 1000.0f);
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
            WaitForMultipleObjects((DWORD)game_threads.size(), done_events.get(), TRUE, INFINITE);
        }
    }
    else {
        c_system_container *g = (c_system_container *)texturePanels[selectedPanel]->GetSelected();
        auto ih = (c_input_handler *)g_ih.get();

        auto &button_map = g->get_button_map();
        g->system->set_input(ih->get_console_input(button_map));

        g->system->emulate_frame();
        if (benchmark_mode) {
            if (++benchmark_frame_count == benchmark_frames) {
                exit(0);
            }
        }
    }
}

void c_nemulator::handle_button_reset(s_button_handler_params* params)
{
    c_system_container* g = (c_system_container*)texturePanels[selectedPanel]->GetSelected();
    if (g == NULL)
        return;
    if (g->system->is_loaded())
    {
        g->system->reset();
        status->add_message("reset");
    }
}

void c_nemulator::handle_button_audio_info(s_button_handler_params *params)
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
        audio_info->z = (float)(eye_z + (1.0 / tan(fov_h / 2)));
        add_task(audio_info, NULL);
    }
}

void c_nemulator::handle_button_stats(s_button_handler_params *params)
{
    if (stats)
    {
        stats->dead = true;
        stats = NULL;
    }
    else
    {
        stats = new c_stats();
        add_task(stats, NULL);
    }
}

void c_nemulator::handle_button_mask_sides(s_button_handler_params *params)
{
    c_system_container* g = (c_system_container*)texturePanels[selectedPanel]->GetSelected();
    g->mask_sides = !g->mask_sides;
    if (g->mask_sides) {
        status->add_message("side mask enabled");
    }
    else {
        status->add_message("side mask disabled");
    }
}

void c_nemulator::handle_button_sprite_limit(s_button_handler_params *params)
{
    c_system_container* g = (c_system_container*)texturePanels[selectedPanel]->GetSelected();
    if (g->is_nes)
    {
        nes::c_nes *n = ((nes::c_nes *)g->system.get());
        n->set_sprite_limit(!n->get_sprite_limit());
        if (n->get_sprite_limit())
            status->add_message("sprites limited");
        else
            status->add_message("sprites unlimited");
    }
}

void c_nemulator::adjust_sharpness(float value)
{
    if (value < 0.0f && sharpness <= 0.0f)
        return;
    if (value > 0.0f && sharpness >= 1.0f)
        return;
    sharpness += value;
    if (sharpness < 0.0f + .0001f)
        sharpness = 0.0f;
    if (sharpness + .0001f > 1.0f)
        sharpness = 1.0f;
    mainPanel2->set_sharpness(sharpness);
    char buf[6];
    sprintf_s(buf, sizeof(buf), "%.3f", sharpness);
    status->add_message("set sharpness to " + std::string(buf));
}

void c_nemulator::handle_button_dec_sharpness(s_button_handler_params *params)
{
    adjust_sharpness(-0.025f);
}

void c_nemulator::handle_button_inc_sharpness(s_button_handler_params *params)
{
    adjust_sharpness(.025f);
}


void c_nemulator::handle_button_menu_right(s_button_handler_params *params)
{
    if (params->result & c_input_handler::RESULT_REPEAT_EXTRAFAST && !texturePanels[selectedPanel]->is_last_col())
    {
        texturePanels[selectedPanel]->set_scroll_duration(20.0);
        fastscroll = true;
    }
    else if (params->result & c_input_handler::RESULT_REPEAT_FAST && !texturePanels[selectedPanel]->is_last_col())
    {
        texturePanels[selectedPanel]->set_scroll_duration(50.0);
        fastscroll = true;
    }
    else
        texturePanels[selectedPanel]->set_scroll_duration(180.0);
    texturePanels[selectedPanel]->NextColumn();
}

void c_nemulator::handle_button_menu_left(s_button_handler_params *params)
{
    if (params->result & c_input_handler::RESULT_REPEAT_EXTRAFAST && !texturePanels[selectedPanel]->is_first_col())
    {
        texturePanels[selectedPanel]->set_scroll_duration(20.0);
        fastscroll = true;
    }
    else if (params->result & c_input_handler::RESULT_REPEAT_FAST && !texturePanels[selectedPanel]->is_first_col())
    {
        texturePanels[selectedPanel]->set_scroll_duration(50.0);
        fastscroll = true;
    }
    else
        texturePanels[selectedPanel]->set_scroll_duration(180.0);
    texturePanels[selectedPanel]->PrevColumn();
}

void c_nemulator::handle_button_menu_up(s_button_handler_params *params)
{
    if (texturePanels[selectedPanel]->PrevRow())
    {
        show_qam();
    }
}

void c_nemulator::handle_button_menu_down(s_button_handler_params *params)
{
    if (texturePanels[selectedPanel]->NextRow())
    {
        int c = texturePanels[selectedPanel]->GetSelectedColumn();
        selectedPanel++;
        texturePanels[selectedPanel]->move_to_column(c);
    }
}

void c_nemulator::handle_button_menu_cancel(s_button_handler_params *params)
{
    for (int i = 0; i < num_texture_panels; i++)
        texturePanels[i]->dim = true;
    c_menu::s_menu_items mi;

    const char* m[] = { "quit nemulator", "suspend computer" };
    mi.num_items = show_suspend ? 2 : 1;
    mi.items = (char**)m;
    add_task(new c_menu(), (void*)&mi);
    menu = MENU_QUIT;
}

void c_nemulator::handle_button_menu_ok(s_button_handler_params *params)
{
    start_game();
}

void c_nemulator::handle_button_show_qam(s_button_handler_params *params)
{
    show_qam();
}

void c_nemulator::handle_button_turbo(s_button_handler_params* params)
{
    int button = 0;
    const char* button_names[] = { "1 A", "1 B", "2 A", "2 B" };
    const char* button_name;
    switch (params->button) {
    case BUTTON_1A_TURBO:
        button = BUTTON_1A;
        button_name = button_names[0];
        break;
    case BUTTON_1B_TURBO:
        button = BUTTON_1B;
        button_name = button_names[1];
        break;
    case BUTTON_2A_TURBO:
        button = BUTTON_2A;
        button_name = button_names[2];
        break;
    case BUTTON_2B_TURBO:
        button = BUTTON_2B;
        button_name = button_names[3];
        break;
    default:
        return;
    }
    do_turbo_press(button, button_name);
}

void c_nemulator::handle_button_leave_game(s_button_handler_params* params)
{
    g_ih->ack_button(BUTTON_1START);
    g_ih->ack_button(BUTTON_1SELECT);
    c_system_container *g = (c_system_container *)texturePanels[selectedPanel]->GetSelected();
    for (int i = 0; i < num_texture_panels; i++)
        texturePanels[i]->dim = true;
    c_menu::s_menu_items mi;
    if (g->get_system_name() == "Nintendo FDS") {
        const char *m[] = {"resume", "switch disk", "reset", "return to menu"};
        mi.num_items = 4;
        mi.items = (char **)m;
        add_task(new c_menu(), (int *)&mi);
    }
    else {
        const char *m[] = {"resume", "reset", "return to menu"};
        mi.num_items = 3;
        mi.items = (char **)m;
        add_task(new c_menu(), (int *)&mi);
    }
    menu = MENU_INGAME;
    paused = true;
    sound->stop();
}

void c_nemulator::handle_button_switch_disk(s_button_handler_params *params)
{
    c_system_container *g = (c_system_container *)texturePanels[selectedPanel]->GetSelected();
    if (g->get_system_name() == "Nintendo FDS") {
        nes::c_nes *n = ((nes::c_nes *)g->system.get());
        int side = n->mapper->switch_disk();
        if (side >= 0) {
            status->add_message(
                "Switch to disk " + std::to_string(side / 2 + 1) + " side " + (side % 2 ? 'B' : 'A')
            );
        }
    }
}

const c_nemulator::s_button_handler c_nemulator::button_handlers[] =
{
    { SCOPE::GAMES_LOADED, {BUTTON_RESET}, true, RESULT_DOWN, &c_nemulator::handle_button_reset },
    { SCOPE::GAMES_LOADED, {BUTTON_AUDIO_INFO}, true, RESULT_DOWN, &c_nemulator::handle_button_audio_info },
    { SCOPE::GAMES_LOADED, {BUTTON_STATS}, true, RESULT_DOWN, &c_nemulator::handle_button_stats },
    { SCOPE::GAMES_LOADED, {BUTTON_MASK_SIDES}, true, RESULT_DOWN ,&c_nemulator::handle_button_mask_sides },
    { SCOPE::GAMES_LOADED, {BUTTON_SPRITE_LIMIT}, true, RESULT_DOWN, &c_nemulator::handle_button_sprite_limit },
    { SCOPE::GAMES_LOADED, {BUTTON_DEC_SHARPNESS}, true, RESULT_DOWN_OR_REPEAT, &c_nemulator::handle_button_dec_sharpness },
    { SCOPE::GAMES_LOADED, {BUTTON_INC_SHARPNESS}, true, RESULT_DOWN_OR_REPEAT, &c_nemulator::handle_button_inc_sharpness },
    { SCOPE::IN_MENU, {BUTTON_1RIGHT}, false, RESULT_DOWN_OR_REPEAT, &c_nemulator::handle_button_menu_right },
    { SCOPE::IN_MENU, {BUTTON_1LEFT}, false, RESULT_DOWN_OR_REPEAT, &c_nemulator::handle_button_menu_left },
    { SCOPE::IN_MENU, {BUTTON_1UP}, false, RESULT_DOWN_OR_REPEAT, &c_nemulator::handle_button_menu_up },
    { SCOPE::IN_MENU, {BUTTON_1DOWN}, false, RESULT_DOWN_OR_REPEAT, &c_nemulator::handle_button_menu_down },
    { SCOPE::IN_MENU | SCOPE::NO_GAMES_LOADED, {BUTTON_1B, BUTTON_ESCAPE}, false, RESULT_DOWN_OR_REPEAT, &c_nemulator::handle_button_menu_cancel },
    { SCOPE::IN_MENU, {BUTTON_1A, BUTTON_1START, BUTTON_RETURN}, true, RESULT_DOWN_OR_REPEAT, &c_nemulator::handle_button_menu_ok },
    { SCOPE::IN_MENU, {BUTTON_1SELECT}, true, RESULT_DOWN, &c_nemulator::handle_button_show_qam },
    { SCOPE::IN_GAME, {BUTTON_1A_TURBO, BUTTON_1B_TURBO, BUTTON_2A_TURBO, BUTTON_2B_TURBO}, false, RESULT_DOWN, &c_nemulator::handle_button_turbo },
    { SCOPE::IN_GAME, {BUTTON_ESCAPE}, false, RESULT_DOWN, &c_nemulator::handle_button_leave_game },
    { SCOPE::IN_GAME, {BUTTON_SWITCH_DISK}, true, RESULT_DOWN, &c_nemulator::handle_button_switch_disk },
};


void c_nemulator::ProcessInput(double dt)
{
    int current_scope = 0;
    if (gameList.size() == 0) {
        current_scope |= SCOPE::NO_GAMES_LOADED;
    }
    else {
        current_scope |= SCOPE::GAMES_LOADED;
        if (inGame)
            current_scope |= SCOPE::IN_GAME;
        else
            current_scope |= SCOPE::IN_MENU;
    }

    //if in menu and fastscrolling, and we've released left/right or reached the ends,
    //disable fastscrolling and fade the postition letter.
    if (current_scope | IN_MENU) {
        if (fastscroll)
        {
            if ((!g_ih->get_result(BUTTON_1RIGHT)
                && !g_ih->get_result(BUTTON_1LEFT))
                || texturePanels[selectedPanel]->is_first_col()
                || texturePanels[selectedPanel]->is_last_col())
            {
                fastscroll = false;
                scroll_fade_timer = 333.0;
            }
        }
    }

    //this is a lambda so we can 'break' from nested for loops by using return
    auto check_handlers = [&]() [[msvc::forceinline]] {
        for (auto &bh : button_handlers) {
            if (current_scope & bh.scope) {
                for (auto button : bh.button_list) {
                    if (int result = g_ih->get_result(button, bh.ack) & bh.mask) {
                        s_button_handler_params p = { button, result };
                        std::invoke(bh.func, this, &p);
                        return;
                    }
                }
            }
        }
    };

    check_handlers();

    //check start+select after button check.  If ESC is pressed at the same time,
    //handle_button_leave_game() will have already been called and ack'd these buttons,
    //so there is no chance of that function running twice in the same frame.
    if (g_ih->get_hold_time(BUTTON_1START) >= menu_delay && g_ih->get_hold_time(BUTTON_1SELECT) >= menu_delay) {
        handle_button_leave_game(NULL); //params not used
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
        g_ih->set_turbo_rate(button, g_ih->get_turbo_rate(button) + 2);
    }
    else if (g_ih->get_result(BUTTON_RIGHT_SHIFT) & c_input_handler::RESULT_DEPRESSED)
    {
        g_ih->set_turbo_rate(button, g_ih->get_turbo_rate(button) - 2);
    }
    else
    {
        int turbo_state = g_ih->get_turbo_state(button);
        g_ih->set_turbo_state(button, !turbo_state);
        status->add_message(button_name + " turbo " + (turbo_state ? "disabled" : "enabled"));
        return;
    }
    char message[64];
    sprintf_s(message, "%s turbo rate: %.1f/s", button_name.c_str(),
        60.0/g_ih->get_turbo_rate(button));
    status->add_message(message);
}

void c_nemulator::leave_game()
{
    sound->stop();
    c_system *n = ((c_system_container *)texturePanels[selectedPanel]->GetSelected())->system.get();
    n->disable_mixer();
    inGame = false;
    for (int i = 0; i < num_texture_panels; i++)
        texturePanels[i]->Zoom();
    for (auto &game_thread : game_threads)
    {
        ResumeThread(game_thread->thread_handle);
    }
    c_system_container* g = (c_system_container*)texturePanels[selectedPanel]->GetSelected();
    if (g->is_nes)
    {
        nes::c_nes *n = (nes::c_nes *)g->system.get();
        if (n->get_mapper_number() == 258) { //NFS
            nsf_stats->dead = true;
            nsf_stats = NULL;
        }
    }
    SetWindowText(hWnd, app_title);
}

void c_nemulator::start_game()
{
    //todo: come back and fix has played logic
    c_system_container *g = (c_system_container*)texturePanels[selectedPanel]->GetSelected();
    c_system *n = g->system.get();
    if (n && n->is_loaded())
    {
        sound->play();
        inGame = true;
        n->enable_mixer();
        for (int i = 0; i < num_texture_panels; i++)
            texturePanels[i]->Zoom();
        for (auto &game_thread : game_threads)
        {
            SuspendThread(game_thread->thread_handle);
        }

        if (g->is_nes)
        {
            nes::c_nes *n = (nes::c_nes *)g->system.get();
            if (n->get_mapper_number() == 258) { //NFS
                nsf_stats = new c_nsf_stats();
                nsf_stats->x = eye_x;
                nsf_stats->y = eye_y;
                nsf_stats->z = (float)(eye_z + (1.0 / tan(fov_h / 2)));
                add_task(nsf_stats, g->system.get());
            }
        }
        
        std::string title = g->title;
        std::string system = g->get_system_name();

        SetWindowText(
            hWnd,
            (std::string(app_title) + " - " + title + " (" + system + ")").c_str());
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
                    if (nsf_stats)
                        nsf_stats->dead = true;
                    if (audio_info)
                        audio_info->dead = true;
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
                c_system_container *g = (c_system_container *)texturePanels[selectedPanel]->GetSelected();
                bool fds = g->get_system_name() == "Nintendo FDS";
                int p = *(int*)params;
                if (fds) {
                    if (p == 1) {
                        p = 3;
                    }
                    else if (p > 1) {
                        p -= 1;
                    }
                }
                switch (p)
                {
                case 0: //resume
                    break;
                case 1: //reset
                    {
                    c_system *n = ((c_system_container *)texturePanels[selectedPanel]->GetSelected())->system.get();
                        n->reset();
                        status->add_message("reset");
                    }
                    break;
                case 2: //return to main menu
                    leave_game();
                    break;
                case 3: //fds only, switch disk
                    handle_button_switch_disk(nullptr);
                    break;
                }
                for (int i = 0; i < num_texture_panels; i++)
                    texturePanels[i]->dim = false;
                paused = false;
                menu = 0;
                if (p != 2)
                    sound->play();
            }
            else if (child_result == c_task::TASK_RESULT_CANCEL)
            {
                paused = false;
                for (int i = 0; i < num_texture_panels; i++)
                    texturePanels[i]->dim = false;
                menu = 0;
                sound->play();
            }
            break;
        }
    }
    if (splash_done) ProcessInput(dt);
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
            if (splash_timer < SPLASH_TIMER_FADE_DURATION)
                splash_timer = SPLASH_TIMER_FADE_DURATION;
            if (loaded)
            {
                if (preload)
                {
                    CloseHandle(load_thread_handle);
                    load_thread_handle = 0;
                }
                if (splash_timer < SPLASH_TIMER_FADE_DURATION)
                    splash_timer = SPLASH_TIMER_FADE_DURATION;
                splash_stage++;
                if (benchmark_mode || disable_splash) {
                    splash_done = 1;
                    if (gameList.size() > 0 && benchmark_mode) {
                        start_game();
                    }
                }
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
            c_system *system = ((c_system_container *)texturePanels[selectedPanel]->GetSelected())->system.get();

            const short* buf_l;
            const short* buf_r;

            int num_samples = system->get_sound_bufs(&buf_l, &buf_r);
            
            if (!benchmark_mode && !paused) {
                sound->copy(buf_l, buf_r, num_samples);
                s = sound->sync();
            }
            system->set_audio_freq(sound->get_requested_freq());
        }
        RunGames();

    }

    for (int i = 0; i < num_texture_panels; i++)
        texturePanels[i]->Update(dt);

    static int framesDrawn = 0;
    framesDrawn++;

    elapsed += dt;

    if (audio_info != NULL && elapsed >= 10.0)
    {
        if (inGame)
            audio_info->report((int)sound->get_freq(), s, 0, (int)sound->max_freq, (int)sound->min_freq);
    }

    if (elapsed >= 250.0)
    {
        double fps = framesDrawn / (elapsed / 1000.0);
        if (benchmark_mode) {
            fps_history[fps_index++ % fps_records] = fps;
            max_fps = 60.0;
            for (int i = 0; i < fps_records; i++)
                if (fps_history[i] > 60.0 && fps_history[i] > max_fps)
                    max_fps = fps_history[i];
            char x[64];
            sprintf(x, "%.2f [%.2f] %.2f", fps, max_fps, 1.0 / fps * 1000.0);
            SetWindowText(hWnd, x);
        }

        if (stats)
        {
            stats->clear();
            c_system_container* game = (c_system_container*)texturePanels[selectedPanel]->GetSelected();
            c_system *system = game->system.get();
            stats->report_stat("fps", fps);
            stats->report_stat("freq", sound->get_freq());
            stats->report_stat("audio position", s);
            stats->report_stat("audio bytes buffered", sound->get_buffered_length());
            stats->report_stat("audio buffer length (ms)", (double)sound->get_buffered_length() / (48000.0 * 2 * 2) * 1000.0);
            stats->report_stat("audio resets", sound->resets);
            stats->report_stat("audio buffer wait count", (int)sound->buffer_wait_count);
            stats->report_stat("audio.slope", sound->slope);
            stats->report_stat("audio_frequency", sound->audio_frequency);
            stats->report_stat("audio_position", sound->audio_position);
            stats->report_stat("audio_position_diff", sound->audio_position_diff);
            stats->report_stat("audio state", sound->state);
            std::ostringstream s;
            s << std::hex << std::uppercase << system->get_crc();
            stats->report_stat("CRC", s.str());

            if (game->is_nes)
            {
                nes::c_nes *n = (nes::c_nes *)system;
                stats->report_stat("mapper #", n->get_mapper_number());
                stats->report_stat("mapper name", n->get_mapper_name());
                stats->report_stat("sprite limit", n->get_sprite_limit() ? "limited" : "unlimited");
                switch (n->get_mirroring_mode())
                {
                case 0:
                    stats->report_stat("mirroring", "horizontal");
                    break;
                case 1:
                    stats->report_stat("mirroring", "vertical");
                    break;
                case 2:
                case 3:
                    stats->report_stat("mirroring", "one screen");
                    break;
                case 4:
                    stats->report_stat("mirroring", "four screen");
                    break;
                default:
                    break;
                }
            }
        }
        if (nsf_stats) { //NSF
            c_system_container* game = (c_system_container*)texturePanels[selectedPanel]->GetSelected();
            nes::c_nes *n = (nes::c_nes *)game->system.get();
            nsf_stats->report_stat("Song #", n->read_byte(0x54F7) + 1);
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
        if (!loaded && preload && rom_count)
        {
            RECT r2 = {0, 0, clientWidth, (long)(clientHeight * 1.95)};
            char buf[256];
            sprintf_s(buf, 256, "Loading: %d%%", (int)((double)num_games / rom_count * 100.0));
            font2->DrawText(NULL, buf, -1, &r2, DT_NOCLIP | DT_CENTER | DT_VCENTER, D3DXCOLOR(.46f, .46f, .46f, 1.0));
        }
        d3dDev->OMSetDepthStencilState(state, oldref);
        return;
    }

    if (texturePanels[selectedPanel]->get_num_items() > 0)
    {
        c_system_container *g = (c_system_container*)texturePanels[selectedPanel]->GetSelected();
        for (int i = 0; i < num_texture_panels; i++)
            texturePanels[i]->Draw();

        if (texturePanels[selectedPanel]->state == c_texture_panel::STATE_MENU || texturePanels[selectedPanel]->state == c_texture_panel::STATE_SCROLLING)
        {
            double dim = mainPanel2->dim ? .25 : 1.0;
            DrawText(font1, .05f, .85f, g->title, D3DXCOLOR((float)(1.0f * dim), 0.0f, 0.0f, 1.0f));
            DrawText(font2, .0525f, .925f, g->get_system_name(),
                     D3DXCOLOR((float)(.22f * dim), (float)(.22f * dim), (float)(.22f * dim), 1.0f));
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

        if (inGame && g->is_nes && g->system->get_crc() == 0x0B0E128F)
        {
            char time[6];
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
            sound->stop();
        else if (menu != MENU_INGAME)
            sound->play();
    }
}

void c_nemulator::GetEvents()
{
    if (done_events)
    {
        done_events.reset();
    }

    std::list<c_texture_panel_item*> panelItems;
    for (int i = 0; i < num_texture_panels; i++)
        texturePanels[i]->GetActive(&panelItems);
    panelItems.sort();
    panelItems.unique();

    if (panelItems.size() > 0)
    {
        done_events = std::make_unique<HANDLE[]>(game_threads.size());
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
            game_threads[y % game_threads.size()]->game_list.push_back((c_system_container*)i);
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
    std::string arcade_path = config->get_string("arcade.rom_path", "c:\\roms\\arcade");
    struct s_loadinfo
    {
        std::string rom_path_key;
        std::string save_path_key;
        std::string rom_path_default;
        c_system::s_system_info &system_info;
        
        std::string rom_path;
        std::string save_path;
        std::vector<std::string> file_list;
    };
    std::vector<s_loadinfo> loadinfo;
 
    for (auto &si : system_registry::get_registry()) {
        loadinfo.push_back({
            .rom_path_key = si.identifier + ".rom_path",
            .save_path_key = si.identifier + ".save_path",
            .rom_path_default = si.is_arcade ? arcade_path + "\\" + si.identifier : "c:\\roms\\" + si.identifier,
            .system_info = si,
        });
    }

    bool global_mask_sides = config->get_bool("mask_sides", false);
    bool global_limit_sprites = config->get_bool("limit_sprites", false);

    for (auto &li : loadinfo)
    {
        li.rom_path = config->get_string(li.rom_path_key, li.rom_path_default);
        li.save_path = config->get_string(li.save_path_key, li.rom_path_default);
        if (!std::filesystem::exists(li.save_path))
            li.save_path = li.rom_path;

        if (li.system_info.is_arcade) {
            if (std::filesystem::exists(li.rom_path)) {
                rom_count++;
                li.file_list.push_back(li.rom_path + ".dir");
            }
        }
        else {
            std::error_code ec;
            for (auto const &dir_entry : std::filesystem::directory_iterator(li.rom_path, ec)) {
                if (dir_entry.is_regular_file() && dir_entry.path().extension() == "." + li.system_info.identifier) {
                    rom_count++;
                    li.file_list.push_back(dir_entry.path().filename().string());
                }
            }
        }
    }
    const int buf_size = 1024 * 64;
    auto buf = std::make_unique_for_overwrite<char[]>(buf_size);
    for (auto &li : loadinfo)
    {
        for (auto &fn : li.file_list)
        {
            num_games++;
            std::string filename = li.rom_path + "\\" + fn;

            if (preload)
            {
                std::ifstream file;
                file.open(filename, std::ios_base::in | std::ios_base::binary);
                if (file.is_open())
                {
                    while (file.read(buf.get(), buf_size));
                    file.close();
                }
            }
            c_system_container *g = new c_system_container(li.system_info, li.rom_path, fn, li.save_path);
            if (li.system_info.name == "Nintendo NES") {
                g->is_nes = true;
            }

            if (li.system_info.title != "") {
                //strcpy(g->title, li.system_info.title.c_str());
                g->title = li.system_info.title;
                g->set_description(g->title);
            }
            else {
                g->set_description(fn);
            }
            gameList.push_back(g);
        }
    }

    std::sort(gameList.begin(), gameList.end(), [](const c_system_container* a, const c_system_container* b) {
        std::string a_title = a->title;
        std::string b_title = b->title;
        auto fn = [](char c) { return toupper(c); };
        std::transform(a_title.begin(), a_title.end(), a_title.begin(), fn);
        std::transform(b_title.begin(), b_title.end(), b_title.begin(), fn);
        return a_title < b_title;
    });
    for (auto &game : gameList)
    {
        mainPanel2->AddItem(game);
    }
    loaded = 1;
}

