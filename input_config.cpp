module;
#include "windows.h"
#include "d3d10.h"
#include "D3DX10.h"

#define ReleaseCOM(x) { if(x) {x->Release(); x = 0; } }

module nemulator:input_config;

import system;
import config;
import nemulator.buttons;
import input_handler;

extern ID3D10Device *d3dDev;
extern int clientHeight;
extern int clientWidth;
extern std::unique_ptr<c_input_handler> g_ih;
extern c_config *config;

void c_input_bindings::init(const std::map<int, s_config_name> &names)
{
    config_names = names;
    defaults.clear();
    for (int button = 0; button < BUTTON_COUNT; button++) {
        defaults.push_back(g_ih->get_binding(button));
    }

    for (auto &si : system_registry::get_registry()) {
        auto &identifier = si.get_input_identifier();
        if (system_buttons.contains(identifier)) {
            continue;
        }
        auto &buttons = system_buttons[identifier];
        for (auto &b : si.button_map) {
            if (config_names.contains(b.button)) {
                buttons.push_back(b.button);
                load(identifier, b.button);
            }
        }
    }
}

//e.g., nes.joy1.a, nes.joy1.joy.a.device, nes.joy1.joy.a.type, and nes.joy1.joy.a
c_input_bindings::s_config_keys c_input_bindings::get_config_keys(const std::string &identifier, int button)
{
    auto &name = config_names.at(button);
    std::string joy = identifier + "." + name.base + ".joy." + name.name;
    return {
        .key = identifier + "." + name.base + "." + name.name,
        .device = joy + ".device",
        .type = joy + ".type",
        .joy = joy,
    };
}

//keyboard and joystick keys are independent, so either can be missing to use the default
void c_input_bindings::load(const std::string &identifier, int button)
{
    auto keys = get_config_keys(identifier, button);
    auto binding = defaults[button];
    std::string value;
    if (config->get_string(keys.key, &value)) {
        binding.key = config->get_int(keys.key, 0);
    }
    if (config->get_string(keys.device, &value)) {
        binding.joy = config->get_int(keys.device, -1);
        binding.joy_type = c_input_handler::TYPE_BUTTON;
        binding.joy_value = 0;
        if (binding.joy >= 0) {
            binding.joy_type = config->get_int(keys.type, c_input_handler::TYPE_BUTTON);
            binding.joy_value = config->get_int(keys.joy, 0);
        }
    }
    if (binding != defaults[button]) {
        system_bindings[identifier][button] = binding;
    }
}

//removes all of a system's keys from nemulator.ini, then appends the parts of each assignment that
//differ from the defaults
bool c_input_bindings::save(const std::string &identifier)
{
    std::vector<std::string> remove;
    std::vector<std::pair<std::string, std::string>> append;
    auto system = system_bindings.find(identifier);
    for (int button : system_buttons[identifier]) {
        auto keys = get_config_keys(identifier, button);
        remove.insert(remove.end(), {keys.key, keys.device, keys.type, keys.joy});
        if (system == system_bindings.end() || !system->second.contains(button)) {
            continue;
        }
        auto &b = system->second[button];
        auto &d = defaults[button];
        if (b.key != d.key) {
            append.push_back({keys.key, std::format("0x{:02X}", b.key)});
        }
        if (b.joy != d.joy || b.joy_type != d.joy_type || b.joy_value != d.joy_value) {
            append.push_back({keys.device, std::to_string(b.joy)});
            if (b.joy >= 0) {
                append.push_back({keys.type, std::to_string(b.joy_type)});
                append.push_back({keys.joy, b.joy_type == c_input_handler::TYPE_BUTTON
                                                ? std::to_string(b.joy_value)
                                                : std::format("0x{:X}", b.joy_value)});
            }
        }
    }
    return config->update_config_file(remove, append);
}

c_input_bindings::s_binding c_input_bindings::get(const std::string &identifier, int button)
{
    auto system = system_bindings.find(identifier);
    if (system != system_bindings.end()) {
        auto binding = system->second.find(button);
        if (binding != system->second.end()) {
            return binding->second;
        }
    }
    return defaults[button];
}

bool c_input_bindings::set(const std::string &identifier, int button, const s_binding &binding)
{
    if (binding == defaults[button]) {
        system_bindings[identifier].erase(button);
    }
    else {
        system_bindings[identifier][button] = binding;
    }
    return save(identifier);
}

bool c_input_bindings::reset(const std::string &identifier)
{
    system_bindings.erase(identifier);
    return save(identifier);
}

void c_input_bindings::apply(const std::string &identifier, const std::vector<s_button_map> &button_map)
{
    for (auto &b : button_map) {
        g_ih->set_binding(b.button, get(identifier, b.button));
    }
}

void c_input_bindings::restore(const std::vector<s_button_map> &button_map)
{
    for (auto &b : button_map) {
        g_ih->set_binding(b.button, defaults[b.button]);
    }
}

c_input_config::~c_input_config()
{
    ReleaseCOM(title_font);
    ReleaseCOM(font);
    ReleaseCOM(hint_font);
    g_ih->disable_input_detection();
}

void c_input_config::init(void *params)
{
    bindings = (c_input_bindings *)params;
    load_fonts();
    g_ih->enable_input_detection();

    for (auto &si : system_registry::get_registry()) {
        //systems that share an input configuration are listed once
        if (std::ranges::contains(systems, si.get_input_identifier(), &s_system::identifier)) {
            continue;
        }
        s_system system = {
            .label = si.input_info.name,
            .identifier = si.get_input_identifier(),
            .is_arcade = si.is_arcade,
        };
        if (system.label.empty()) {
            system.label = si.title.empty() ? si.name : si.title;
        }
        for (auto &b : si.button_map) {
            if (b.name) {
                system.buttons.push_back(b);
            }
        }
        std::ranges::stable_sort(system.buttons, {}, [](const s_button_map &b) { return get_display_order(b.button); });
        systems.push_back(system);
    }

    std::ranges::stable_sort(systems, [](const s_system &a, const s_system &b) {
        return std::tie(a.is_arcade, a.label) < std::tie(b.is_arcade, b.label);
    });
}

void c_input_config::resize()
{
    load_fonts();
}

int c_input_config::update(double dt, int child_result, void *params)
{
    const int nav_mask = c_input_handler::RESULT_DOWN | c_input_handler::RESULT_REPEAT;

    switch (state) {
        case STATE_SYSTEMS:
            if (int down = g_ih->get_result(BUTTON_DOWN, true) & nav_mask) {
                move_selection(selected_system, (int)systems.size(), 1, down);
            }
            else if (int up = g_ih->get_result(BUTTON_UP, true) & nav_mask) {
                move_selection(selected_system, (int)systems.size(), -1, up);
            }
            else if (g_ih->get_result(BUTTON_OK, true) & c_input_handler::RESULT_DOWN) {
                if (!systems[selected_system].buttons.empty()) {
                    selected_button = 0;
                    column = COLUMN_KEYBOARD;
                    confirm_reset = false;
                    state = STATE_BUTTONS;
                }
            }
            else if (g_ih->get_result(BUTTON_CANCEL, true) & c_input_handler::RESULT_DOWN) {
                dead = true;
                g_ih->ack();
                return c_task::TASK_RESULT_CANCEL;
            }
            break;
        case STATE_BUTTONS: {
            auto &system = systems[selected_system];
            //the reset row follows the buttons
            int reset_row = (int)system.buttons.size();
            if (int down = g_ih->get_result(BUTTON_DOWN, true) & nav_mask) {
                move_selection(selected_button, reset_row + 1, 1, down);
                confirm_reset = false;
            }
            else if (int up = g_ih->get_result(BUTTON_UP, true) & nav_mask) {
                move_selection(selected_button, reset_row + 1, -1, up);
                confirm_reset = false;
            }
            else if ((g_ih->get_result(BUTTON_LEFT, true) | g_ih->get_result(BUTTON_RIGHT, true)) &
                     c_input_handler::RESULT_DOWN) {
                column = column == COLUMN_KEYBOARD ? COLUMN_JOYPAD : COLUMN_KEYBOARD;
                confirm_reset = false;
            }
            else if (g_ih->get_result(BUTTON_OK, true) & c_input_handler::RESULT_DOWN) {
                if (selected_button < reset_row) {
                    held_inputs = g_ih->get_active_inputs();
                    listen_timer = LISTEN_TIMEOUT;
                    state = STATE_LISTEN;
                }
                else if (!confirm_reset) {
                    //resetting requires a second press
                    confirm_reset = true;
                }
                else {
                    save_failed = !bindings->reset(system.identifier);
                    confirm_reset = false;
                }
            }
            else if (g_ih->get_result(BUTTON_CANCEL, true) & c_input_handler::RESULT_DOWN) {
                confirm_reset = false;
                state = STATE_SYSTEMS;
            }
            break;
        }
        case STATE_LISTEN:
            listen(dt);
            break;
        case STATE_RELEASE:
            //the input handler may not have seen the press yet, so keep acking until it's released to
            //prevent it from also being handled as a menu action
            if (!std::ranges::contains(g_ih->get_active_inputs(), release_input)) {
                state = STATE_BUTTONS;
            }
            break;
    }

    update_scroll(system_scroll, selected_system);
    update_scroll(button_scroll, selected_button);
    g_ih->ack();
    return c_task::TASK_RESULT_NONE;
}

void c_input_config::listen(double dt)
{
    auto &system = systems[selected_system];
    int button = system.buttons[selected_button].button;
    auto active = g_ih->get_active_inputs();

    auto assign = [&](const s_binding &input) {
        auto binding = bindings->get(system.identifier, button);
        if (column == COLUMN_KEYBOARD) {
            binding.key = input.key;
        }
        else {
            binding.joy = input.joy;
            binding.joy_type = input.joy_type;
            binding.joy_value = input.joy_value;
        }
        save_failed = !bindings->set(system.identifier, button, binding);
    };

    //inputs that were held when listening started (e.g., the button that started it) are
    //ignored until they've been released
    std::erase_if(held_inputs, [&](const s_binding &input) { return !std::ranges::contains(active, input); });

    for (auto &input : active) {
        if (std::ranges::contains(held_inputs, input)) {
            continue;
        }
        bool is_key = input.joy < 0;
        if (is_key && input.key == VK_ESCAPE) {
            //cancel
        }
        else if (is_key && (input.key == VK_DELETE || input.key == VK_BACK)) {
            assign({});
        }
        else if (is_key == (column == COLUMN_KEYBOARD)) {
            assign(input);
        }
        else {
            continue;
        }
        release_input = input;
        state = STATE_RELEASE;
        return;
    }

    listen_timer -= dt;
    if (listen_timer <= 0.0) {
        state = STATE_BUTTONS;
    }
}

//wraps only on a new press so that holding a direction stops at the ends of the list
void c_input_config::move_selection(int &selected, int count, int delta, int result)
{
    int next = selected + delta;
    if (next < 0 || next >= count) {
        if (!(result & c_input_handler::RESULT_DOWN)) {
            return;
        }
        next = (next + count) % count;
    }
    selected = next;
}

void c_input_config::update_scroll(int &scroll, int selected)
{
    if (selected < scroll) {
        scroll = selected;
    }
    else if (selected >= scroll + VISIBLE_ROWS) {
        scroll = selected - VISIBLE_ROWS + 1;
    }
}

//directions first, then the remaining buttons in system order, then player 2
int c_input_config::get_display_order(int button)
{
    switch (button) {
        case BUTTON_1UP:
            return 0;
        case BUTTON_1DOWN:
            return 1;
        case BUTTON_1LEFT:
            return 2;
        case BUTTON_1RIGHT:
            return 3;
        case BUTTON_2UP:
            return 5;
        case BUTTON_2DOWN:
            return 6;
        case BUTTON_2LEFT:
            return 7;
        case BUTTON_2RIGHT:
            return 8;
        case BUTTON_2A:
        case BUTTON_2B:
        case BUTTON_2C:
        case BUTTON_2SELECT:
        case BUTTON_2START:
            return 9;
        default:
            return 4;
    }
}

std::string c_input_config::get_key_name(int key)
{
    if (key == 0) {
        return "-";
    }
    UINT scan_code = MapVirtualKey(key, MAPVK_VK_TO_VSC);
    switch (key) {
        //these share scan codes with other keys (mostly on the numpad) and need the extended bit set
        case VK_LEFT:
        case VK_UP:
        case VK_RIGHT:
        case VK_DOWN:
        case VK_PRIOR:
        case VK_NEXT:
        case VK_END:
        case VK_HOME:
        case VK_INSERT:
        case VK_DELETE:
        case VK_DIVIDE:
        case VK_NUMLOCK:
        case VK_RCONTROL:
        case VK_RMENU:
        case VK_LWIN:
        case VK_RWIN:
        case VK_APPS:
            scan_code |= 0x100;
            break;
    }
    char name[64];
    if (scan_code && GetKeyNameText(scan_code << 16, name, (int)sizeof(name)) > 0) {
        return name;
    }
    return std::format("0x{:02X}", key);
}

std::string c_input_config::get_joy_name(const s_binding &binding)
{
    if (binding.joy < 0) {
        return "-";
    }
    std::string name = "Joy " + std::to_string(binding.joy + 1) + " ";
    switch (binding.joy_type) {
        case c_input_handler::TYPE_AXIS: {
            static const char *axes[] = {"X", "Y", "Z"};
            int axis = binding.joy_value & 0xFF;
            signed char threshold = (binding.joy_value >> 8) & 0xFF;
            return name + (axis < 3 ? axes[axis] : "?") + (threshold < 0 ? "-" : "+");
        }
        case c_input_handler::TYPE_POV: {
            //name the direction closest to the center of the range
            static const char *directions[] = {"Up", "Right", "Down", "Left"};
            int low = binding.joy_value & 0xFFFF;
            int high = (binding.joy_value >> 16) & 0xFFFF;
            if (high < low) {
                high += 36000;
            }
            int center = ((low + high) / 2) % 36000;
            return name + "POV " + directions[((center + 4500) / 9000) % 4];
        }
        default:
            return name + "Button " + std::to_string(binding.joy_value + 1);
    }
}

void c_input_config::load_fonts()
{
    struct s_fonts
    {
        ID3DX10Font **font;
        double scale;
    };

    s_fonts fonts[] = {
        {&title_font, .08},
        {&font, .05},
        {&hint_font, .035},
    };

    for (auto f : fonts) {
        ReleaseCOM((*f.font));
        D3DX10_FONT_DESC fontDesc = {0};
        fontDesc.Height = (int)(clientHeight * f.scale);
        fontDesc.Width = 0;
        fontDesc.Weight = 0;
        fontDesc.MipLevels = 1;
        fontDesc.Italic = false;
        fontDesc.CharSet = DEFAULT_CHARSET;
        fontDesc.OutputPrecision = OUT_DEFAULT_PRECIS;
        fontDesc.Quality = DEFAULT_QUALITY;
        fontDesc.PitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
        strcpy_s(fontDesc.FaceName, "Calibri");
        D3DX10CreateFontIndirect(d3dDev, &fontDesc, f.font);
    }
}

void c_input_config::draw_text(ID3DX10Font *f, const std::string &text, double left, double right, double y,
                               UINT format, D3DXCOLOR color)
{
    RECT r = {(LONG)(clientWidth * left), (LONG)(clientHeight * y), (LONG)(clientWidth * right), clientHeight};
    f->DrawText(NULL, text.c_str(), -1, &r, DT_NOCLIP | format, color);
}

void c_input_config::draw()
{
    const D3DXCOLOR normal(1.0f, 1.0f, 1.0f, .8f);
    const D3DXCOLOR highlight(1.0f, 0.0f, 0.0f, .8f);
    const D3DXCOLOR hint(.46f, .46f, .46f, 1.0f);

    ID3D10DepthStencilState *depth_state;
    UINT oldref;
    d3dDev->OMGetDepthStencilState(&depth_state, &oldref);

    std::string hint_text;
    if (state == STATE_SYSTEMS) {
        draw_text(title_font, "configure input", 0.0, 1.0, TITLE_Y, DT_CENTER, normal);
        int end = system_scroll + VISIBLE_ROWS;
        if (end > (int)systems.size()) {
            end = (int)systems.size();
        }
        for (int i = system_scroll; i < end; i++) {
            double y = LIST_Y + ROW_HEIGHT * (i - system_scroll);
            draw_text(font, systems[i].label, 0.0, 1.0, y, DT_CENTER, i == selected_system ? highlight : normal);
        }
        hint_text = "Enter: select    Esc: back";
    }
    else {
        const double name_left = .12;
        const double keyboard_left = .38;
        const double joypad_left = .64;
        const double joypad_right = .92;

        auto &system = systems[selected_system];
        int reset_row = (int)system.buttons.size();
        draw_text(title_font, system.label, 0.0, 1.0, TITLE_Y, DT_CENTER, normal);
        draw_text(hint_font, "keyboard", keyboard_left, joypad_left, HEADER_Y, DT_CENTER, hint);
        draw_text(hint_font, "joypad", joypad_left, joypad_right, HEADER_Y, DT_CENTER, hint);

        int end = button_scroll + VISIBLE_ROWS;
        if (end > reset_row + 1) {
            end = reset_row + 1;
        }
        for (int i = button_scroll; i < end; i++) {
            double y = LIST_Y + ROW_HEIGHT * (i - button_scroll);
            bool row_selected = i == selected_button;
            if (i == reset_row) {
                draw_text(font, confirm_reset ? "press again to reset to defaults" : "reset to defaults", name_left,
                          joypad_right, y, DT_CENTER, row_selected ? highlight : normal);
                continue;
            }
            auto &button = system.buttons[i];
            auto binding = bindings->get(system.identifier, button.button);
            std::string key_text = get_key_name(binding.key);
            std::string joy_text = get_joy_name(binding);
            if (row_selected && state == STATE_LISTEN) {
                if (column == COLUMN_KEYBOARD) {
                    key_text = "...";
                }
                else {
                    joy_text = "...";
                }
            }
            draw_text(font, button.name, name_left, keyboard_left, y, DT_LEFT, row_selected ? highlight : normal);
            draw_text(font, key_text, keyboard_left, joypad_left, y, DT_CENTER,
                      row_selected && column == COLUMN_KEYBOARD ? highlight : normal);
            draw_text(font, joy_text, joypad_left, joypad_right, y, DT_CENTER,
                      row_selected && column == COLUMN_JOYPAD ? highlight : normal);
        }

        if (state == STATE_LISTEN) {
            int seconds = (int)std::ceil(listen_timer / 1000.0);
            hint_text = std::string(column == COLUMN_KEYBOARD ? "press a key" : "press a joypad button or direction") +
                        " for " + system.buttons[selected_button].name + "    Del: clear    Esc: cancel    (" +
                        std::to_string(seconds) + ")";
        }
        else if (selected_button == reset_row) {
            hint_text = "Enter: reset    Esc: back";
        }
        else {
            hint_text = "Enter: assign    Left/Right: keyboard/joypad    Esc: back";
        }
    }
    if (save_failed) {
        draw_text(hint_font, "unable to save to nemulator.ini", 0.0, 1.0, ERROR_Y, DT_CENTER, highlight);
    }
    draw_text(hint_font, hint_text, 0.0, 1.0, HINT_Y, DT_CENTER, hint);

    d3dDev->OMSetDepthStencilState(depth_state, oldref);
    ReleaseCOM(depth_state);
}
