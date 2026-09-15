module;
#include "d3d10.h"
#include <d3dx10.h>

export module nemulator:input_config;
import nemulator.std;

import task;
import input_handler;
import nemulator.buttons;

//Button assignments for each system, saved to nemulator.ini.  Buttons without an assignment for a
//system use the defaults from nemulator.ini, which are never modified.
export class c_input_bindings
{
  public:
    using s_binding = c_input_handler::s_binding;
    //nemulator.ini key components for a button, e.g., joy1 and a for joy1.a
    struct s_config_name
    {
        std::string base;
        std::string name;
    };
    //buttons that aren't part of any system are configured under this identifier and apply globally.
    //their joy1 keys stay the default, with an override saved under the general prefix
    static constexpr const char *general_identifier = "general";
    //snapshot the defaults and load each system's assignments; call after the defaults have been
    //loaded from nemulator.ini
    void init(const std::map<int, s_config_name> &config_names);
    s_binding get(const std::string &identifier, int button);
    //set and reset save the system's assignments to nemulator.ini, returning false if that fails
    bool set(const std::string &identifier, int button, const s_binding &binding);
    bool reset(const std::string &identifier);
    //assign a system's buttons when entering a game
    void apply(const std::string &identifier);
    //return a system's buttons to the defaults when leaving a game
    void restore(const std::string &identifier);

  private:
    struct s_config_keys
    {
        std::string key;
        std::string device;
        std::string type;
        std::string joy;
    };
    s_config_keys get_config_keys(const std::string &identifier, int button);
    void init_general();
    void load(const std::string &identifier, int button);
    bool save(const std::string &identifier);

    std::map<int, s_config_name> config_names;
    std::vector<s_binding> defaults;
    //buttons that can be assigned for each system
    std::map<std::string, std::vector<int>> system_buttons;
    std::map<std::string, std::map<int, s_binding>> system_bindings;
};

export class c_input_config : public c_task
{
  public:
    ~c_input_config();
    void init(void *params);
    int update(double dt, int child_result, void *params);
    void draw();
    void resize();

  private:
    using s_binding = c_input_handler::s_binding;
    enum STATE
    {
        STATE_SYSTEMS, //choosing a system
        STATE_BUTTONS, //choosing a button
        STATE_LISTEN,  //waiting for a key or joypad input to assign
        STATE_RELEASE  //waiting for the input that ended listening to be released
    };
    enum COLUMN
    {
        COLUMN_KEYBOARD,
        COLUMN_JOYPAD
    };
    struct s_button_row
    {
        int button;
        std::string label;
    };
    struct s_system
    {
        std::string label;
        std::string identifier;
        int is_arcade;
        int order = 1; //the general entry sorts ahead of the systems
        std::vector<s_button_row> buttons;
    };

    void listen(double dt);
    void load_fonts();
    void draw_text(ID3DX10Font *f, const std::string &text, double left, double right, double y, UINT format,
                   D3DXCOLOR color);
    static void move_selection(int &selected, int count, int delta, int result);
    static void update_scroll(int &scroll, int selected);
    static int get_display_order(int button);
    static std::string get_key_name(int key);
    static std::string get_joy_name(const s_binding &binding);

    static constexpr double LISTEN_TIMEOUT = 5000.0;
    static constexpr int VISIBLE_ROWS = 9;
    static constexpr double TITLE_Y = .06;
    static constexpr double HEADER_Y = .17;
    static constexpr double LIST_Y = .24;
    static constexpr double ROW_HEIGHT = .065;
    static constexpr double ERROR_Y = .83;
    static constexpr double HINT_Y = .88;

    c_input_bindings *bindings = nullptr;
    std::vector<s_system> systems;
    int state = STATE_SYSTEMS;
    int column = COLUMN_KEYBOARD;
    int selected_system = 0;
    //the reset row follows the buttons, so selected_button == buttons.size() selects it
    int selected_button = 0;
    int system_scroll = 0;
    int button_scroll = 0;
    bool confirm_reset = false;
    bool save_failed = false;
    double listen_timer = 0.0;
    std::vector<s_binding> held_inputs;
    s_binding release_input;
    ID3DX10Font *title_font = nullptr;
    ID3DX10Font *font = nullptr;
    ID3DX10Font *hint_font = nullptr;
};
