module;
#include "d3d11.h"

export module nemulator:disk_indicator;
import nemulator.std;
import task;
import font;
import vec;
import shader_pass;
import system;

//shown in the corner of the screen while the running game's disk is being accessed, since loading from a
//disk (e.g., on the Famicom Disk System) is slow enough that the game can appear to have hung
export class c_disk_indicator : public c_task
{
  public:
    c_disk_indicator();
    ~c_disk_indicator();
    void init(void *params);
    int update(double dt, int child_result, void *params);
    void resize();
    void draw();
    //called each frame with what the running game's disk is doing.  with visible false (no game with a
    //disk is running, it's paused, or the indicator is turned off), it's hidden at once
    void report(double dt, bool visible, const c_system::s_disk_activity &activity);

  private:
    struct s_vertex
    {
        float x, y, z;
        float u, v;
    };
    //what shape_disk_ps.hlsl expects
    struct s_constants
    {
        s_vec2 size;
        float progress;
        float writing;
        float spin;
        float alpha;
        float padding[2];
    };
    c_shader_pass pass;
    ID3D11Buffer *constants_buffer;
    ID3D11InputLayout *layout;
    ID3D11Buffer *vertices;
    c_font *font;
    void load_font();

    c_system::s_disk_activity shown; //the last activity, kept while the indicator lingers and fades
    bool switching;
    double hold_timer; //ms left to keep showing the indicator after the disk stops
    double alpha;
    double spin;
    double blink_timer;
};
