module;
#include "d3d10.h"
#include "d3dx10.h"

export module nemulator.menu;
import std.compat;
import nemulator.task;

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
    };

protected:
    double text_height;
    double text_spacing;
    double y_offset;
    void draw_text(char *text, double x, double y, D3DXCOLOR color);
    ID3DX10Font *font;
    int selected_item;
    void load_fonts();
    s_menu_items *menu_items;
};
