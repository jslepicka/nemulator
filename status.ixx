module;
#include "d3d11.h"

export module nemulator:status;
import nemulator.std;
import task;
import font;
import vec;

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
    void draw_text(const char *text, double x, double y, s_color color);
    c_font *font;
    void load_fonts();
    struct s_message
    {
        std::string message;
        double timer;
    };

    std::deque<s_message*> messages;

};
