module;
#include "d3d10.h"
#include "d3dx10.h"

export module nemulator:stats;
import nemulator.std;
import task;

export class c_stats:
    public c_task
{
public:
    c_stats();
    ~c_stats();
    virtual void init (void  *params);
    virtual int update(double dt, int child_result, void *params);
    virtual void draw();
    void resize();
    void report_stat(std::string stat_name, int stat_value);
    void report_stat(std::string stat_name, double stat_value);
    void report_stat(std::string stat_name, std::string stat_value);
    void report_stat(std::string stat_name, uint64_t stat_value);
    void clear();
  private:
    bool reported;
    void draw_text(char *text, double x, double y, D3DXCOLOR color);
    ID3DX10Font *font;
    void load_fonts();
    std::map<std::string, std::string> stats;
};
