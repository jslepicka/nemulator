module;
#include "d3d10.h"
#include "d3dx10.h"

export module nemulator:status;
import std.compat;
import task;

export class c_status:
    public c_task
{
public:
    c_status();
    ~c_status();
    void init (void  *params);
    int update(double dt, int child_result, void *params);
    void draw();
    void resize();
    void add_message(std::string message);
private:
    void draw_text(const char *text, double x, double y, D3DXCOLOR color);
    ID3DX10Font *font;
    void load_fonts();
    struct s_message
    {
        std::string message;
        double timer;
    };

    std::deque<s_message*> messages;

};
