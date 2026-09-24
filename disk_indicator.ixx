module;
#include "d3d10.h"
#include "d3dx10.h"

export module nemulator:disk_indicator;
import nemulator.std;
import task;
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
    ID3D10Effect *effect;
    ID3D10EffectTechnique *technique;
    ID3D10EffectVectorVariable *var_size;
    ID3D10EffectScalarVariable *var_progress;
    ID3D10EffectScalarVariable *var_writing;
    ID3D10EffectScalarVariable *var_spin;
    ID3D10EffectScalarVariable *var_alpha;
    ID3D10InputLayout *layout;
    ID3D10Buffer *vertices;
    ID3DX10Font *font;
    void load_font();

    c_system::s_disk_activity shown; //the last activity, kept while the indicator lingers and fades
    bool switching;
    double hold_timer; //ms left to keep showing the indicator after the disk stops
    double alpha;
    double spin;
    double blink_timer;
};
