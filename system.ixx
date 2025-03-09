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
    virtual ~c_system() {};
    virtual int load() = 0;
    virtual int is_loaded() = 0;
    virtual int emulate_frame() = 0;
    virtual int reset() = 0;
    int get_crc() { return crc32; }
    virtual int get_sound_bufs(const short **buf_l, const short **buf_r) = 0;
    virtual void set_audio_freq(double freq) = 0;
    virtual void set_input(int input) = 0;
    virtual void enable_mixer() {}
    virtual void disable_mixer() {}
    virtual int *get_video() = 0;
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
        //std::function<c_system *()> constructor;
        std::function <std::unique_ptr<c_system>()> constructor;
    };

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
