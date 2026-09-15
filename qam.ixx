module;
#include "d3d10.h"
#include <d3dx10.h>

export module nemulator:qam;
import nemulator.std;

import task;

export class c_qam :
    public c_task
{
public:
    c_qam();
    ~c_qam();
    void init(void *params);
    int update(double dt, int child_result, void *params);
    void draw();
    void resize();
    void load_fonts();
    void set_valid_chars(int *v);
    void set_char(char c);
    void set_systems(const std::vector<std::string> &system_names, int active);
    void activate();
    //after returning TASK_RESULT_RETURN, either a system was chosen or params holds the chosen character
    bool system_chosen() { return result == RESULT_SYSTEM; }
    int get_system() { return selected_system; }
private:
    ID3DX10Font *font;
    ID3DX10Font *system_font;
    //the arrow is drawn as a shape rather than as text, so it can turn over between the rows
    struct s_shape_vertex
    {
        float x, y, z;
        float u, v;
    };
    ID3D10Effect *shape_effect;
    ID3D10EffectTechnique *triangle_technique;
    ID3D10EffectVectorVariable *shape_color;
    ID3D10EffectScalarVariable *shape_angle; //so the shading's light stays put as the shape turns
    ID3D10InputLayout *shape_layout;
    ID3D10Buffer *shape_vertices;
    void init_arrow();
    void draw_arrow(double center_x, double center_y, double size, double angle, D3DXCOLOR color);
    int selected;
    static const char *c;
    double scroll_timer;
    double scroll_pos;
    static const double scroll_delay;
    static const double row_height;
    static const double arrow_height;
    int scroll_target;
    int state;
    int result;
    enum STATE
    {
        STATE_ACTIVATED = 0,
        STATE_SCROLL_IN,
        STATE_SCROLL_OUT,
        STATE_READY,
        STATE_IDLE
    };
    enum RESULT
    {
        RESULT_CANCEL,
        RESULT_CHAR,
        RESULT_SYSTEM
    };
    enum ROW
    {
        ROW_SYSTEM,
        ROW_CHAR
    };
    int row;
    int *valid_chars;
    std::vector<std::string> systems;
    int selected_system;
    int active_system; //the system the menu is currently filtered to
    void layout_systems();
    void update_system_scroll_target();
    std::vector<int> system_lefts; //left edge of each system name, relative to the start of the row
    std::vector<int> system_widths;
    int system_row_left; //the system row is aligned with the letter row
    int system_row_right;
    double system_scroll; //pixels the system row is scrolled left; eases toward system_scroll_target
    double system_scroll_target;
    static const double system_scroll_time;
    //only one row is shown at a time: 0 shows the letters, 1 the systems.  eases toward the selected row
    double row_blend;
    static const double row_blend_time;
};
