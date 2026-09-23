module;

export module system;
import nemulator.std;

import class_registry;
import nemulator.buttons;

// An emulated system (game console, arcade machine, etc.)
export class c_system
{
public:
    c_system() {};
    virtual ~c_system() = default;
    virtual int load() = 0;
    virtual int is_loaded() = 0;
    virtual int emulate_frame() = 0;
    virtual int reset() = 0;
    int get_crc() { return crc32; }
    virtual int get_sound_buf(const float **buf) = 0;
    virtual void set_audio_freq(double freq) = 0;
    virtual void set_input(int input) = 0;
    virtual void enable_mixer() {}
    virtual void disable_mixer() {}
    virtual int *get_video() = 0;
    //systems with has_sprite_limit set limit sprites per line as the hardware does, unless turned off
    virtual void set_sprite_limit(bool limit_sprites) {}
    virtual bool get_sprite_limit() { return false; }
    std::string path;
    std::string filename;
    std::string path_file;
    std::string sram_filename;
    std::string sram_path;
    std::string sram_path_file;

    struct s_system_info
    {
        int is_arcade = 0;
        std::string name;
        //for consoles, identifier is the file extension (e.g., nes)
        //for arcade games, it's the name of the rom set (e.g., pacman)
        std::string identifier;
        std::string extension;
        std::string title;
        struct s_display_info
        {
            int fb_width = 0;
            int fb_height = 0;
            int crop_left = 0;
            int crop_right = 0;
            int crop_top = 0;
            int crop_bottom = 0;
            int rotation = 0;
            double aspect_ratio = 4.0 / 3.0;
        } display_info;
        std::vector<s_button_map> button_map;
        //systems with the same input identifier share an input configuration (e.g., NES and FDS)
        //if unset, the system's identifier and title or name are used
        struct s_input_info
        {
            std::string identifier;
            std::string name;
        } input_info;
        //buttons that can be set to auto-fire.  Name player 1's buttons; player 2's equivalents are
        //added automatically.  Turbo toggles have no default assignment.
        std::vector<uint32_t> turbo_buttons;
        int num_sound_channels = 1;
        float volume = 1.0f;
        //the rate the system runs at, used to pace frames when syncing to a timer.
        //todo: set the real rate for each system; they all use 60.0 for now
        double frame_rate = 60.0;
        //the system has a per-line sprite limit that can be turned off in settings > system.  systems
        //that share an input identifier (e.g., NES and FDS) share their system settings as well
        bool has_sprite_limit = false;
        std::function <std::unique_ptr<c_system>()> constructor;

        const std::string &get_input_identifier() const
        {
            return input_info.identifier.empty() ? identifier : input_info.identifier;
        }
    };
    int crop_left;
    int crop_right;
    int crop_top;
    int crop_bottom;

  protected:
    int crc32 = 0;
};

export class system_registry : public c_class_registry<std::vector<c_system::s_system_info>>
{
  public:
    static void _register(std::vector<c_system::s_system_info> system_info)
    {
        for (auto &s : system_info) {
            get_registry().push_back(s);
        }
    }
};
