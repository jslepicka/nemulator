module;
#include "d3d10.h"
#include <d3dx10.h>
#include <string>
export module texture_panel:item;

export class c_texture_panel_item
{
public:
    c_texture_panel_item();
    virtual ~c_texture_panel_item();
    void Activate(bool load);
    void Deactivate();
    void Load();
    virtual void DrawToTexture(ID3D10Texture2D *tex) {}
    virtual bool Selectable() { return true;}
    void set_description(std::string s) { description = s; }
    std::string &get_description() { return description; }
    int is_active;
    virtual ID3D10Buffer *get_vertex_buffer(int stretched) = 0;
    virtual double get_height() = 0;
    virtual double get_width() = 0;
private:
    virtual void OnActivate(bool load) {};
    virtual void OnDeactivate() {};
    virtual void OnLoad() {};
    std::string description;
};
