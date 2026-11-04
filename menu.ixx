module;
#include "d3d11.h"

export module nemulator.menu;
import nemulator.std;
import task;
import font;
import vec;

export class c_menu :
    public c_task
{
public:
    c_menu();
    virtual ~c_menu();
    virtual void init(void *params);
    virtual int update(double dt, int child_result, void *params);
    virtual void draw();
    virtual void resize();
    struct s_menu_items
    {
        int num_items;
        char **items;
        int selected = 0; //the item selected when the menu opens
    };

protected:
    double text_height;
    double text_spacing;
    double y_offset;
    void draw_text(char *text, double x, double y, s_color color);
    c_font *font;
    int selected_item;
    void load_fonts();
    s_menu_items *menu_items;
};

//a list of settings showing each one's value.  Changes are applied through each item's callback as
//they're made; the menu returns TASK_RESULT_CANCEL when it's closed.
export class c_options_menu :
    public c_task
{
public:
    struct s_item
    {
        std::string label;
        //the value to display; items without one are actions, e.g., reset to defaults
        std::function<std::string()> get_value;
        //called with -1 or 1 for Left/Right (settings only), or 0 for Enter
        std::function<void(int)> change;
        bool repeat = false; //Left/Right repeat while held
        std::string confirm_label; //if set, Enter must be pressed again while this label is shown
    };
    struct s_params
    {
        std::string title;
        std::vector<s_item> items;
        //a warning shown below the list, e.g. when a setting doesn't suit the hardware.  empty for none
        std::function<std::string()> get_note;
        bool shadow = false; //draws text with a shadow so it can be read over an undimmed game
    };
    ~c_options_menu();
    void init(void *params);
    int update(double dt, int child_result, void *params);
    void draw();
    void resize();

private:
    void load_fonts();
    void draw_text(c_font *f, const std::string &text, double left, double right, double y, UINT format,
                   s_color color);

    static constexpr double TITLE_Y = .06;
    static constexpr double LIST_Y = .24;
    static constexpr double ROW_HEIGHT = .065;
    static constexpr double NOTE_Y = .80;
    static constexpr double HINT_Y = .88;

    std::string title;
    std::vector<s_item> items;
    int selected_item = 0;
    bool confirm = false;
    std::function<std::string()> get_note;
    bool shadow = false;
    c_font *title_font = nullptr;
    c_font *font = nullptr;
    c_font *hint_font = nullptr;
};
