module;
#include <vector>
#include <list>
#include "d3d10.h"
#include "d3dx10.h"

export module texture_panel;
export import :item;

export class c_texture_panel
{
public:
    c_texture_panel(int rows, int columns);
    ~c_texture_panel();
    void load_items();
    bool is_first_col();
    bool is_last_col();
    int get_num_items();
    void set_scroll_duration(float duration) { scrollDuration = duration; };
    void Init();
    void AddItem(c_texture_panel_item *item);
    void Update(double dt);
    void Draw(/*float x, float y, float z*/);
    void GetActive(std::list<c_texture_panel_item*> *itemList);
    int NextRow(bool adjusting = false);
    int PrevRow(bool adjusting = false);
    void NextColumn(bool load = true, bool adjusting = false);
    void PrevColumn(bool load = true, bool adjusting = false);
    void move_to_column(int column);
    void move_to_char(char c);
    void Zoom();
    bool Changed();
    int GetSelectedColumn();
    c_texture_panel_item* GetSelected();
    c_texture_panel *prev;
    c_texture_panel *next;
    float x;
    float y;
    float z;
    float zoomDestX;
    float zoomDestY;
    float zoomDestZ;
    //void Suspend();
    //void Resume();
    //void OnResize();

    static const int STATE_NULL = 0;
    static const int STATE_MENU = 1;
    static const int STATE_SCROLLING = 2;
    static const int STATE_ZOOMING = 3;
    static const int STATE_ZOOMED = 4;
    static constexpr float tile_spacing = .02;
    static constexpr float tile_height = (1.0) * 2.0 + (tile_spacing * 2.0);
    static constexpr float tile_width = (4.0 / 3.0) * 2.0 + (tile_spacing * 2.0);
    int state;
    bool dim;
    bool selectable;
    bool in_focus; //is this panel selected/does it have focus?
    int *get_valid_chars();
    void set_sharpness(float factor);

private:

    struct SimpleVertex
    {
        D3DXVECTOR3 pos;
        D3DXVECTOR2 tex;
    };
    class c_item_container
    {
    public:
        c_item_container(c_texture_panel_item *item);
        ~c_item_container();
        D3DXVECTOR3 pos;
        float ratio;
        bool selecting;
        bool selected;
        void Select();
        void Unselect();
        double selectTimer;
        static const float selectDuration;
        enum {
            DIR_SELECTING,
            DIR_UNSELECTING
        };
        int select_dir;
        c_texture_panel_item *item;
    };

    int scrolls;
    int rows;
    int columns;
    int prevState;
    int numItems;

    bool changed; //has the panel's items changed?
    bool prev_in_focus; //previous selected state
    bool on_first_page; //are we on the first page of this panel?
    int first_item; //index to the first item on the current page
    int last_item; //index to the last item on the current page
    int selected_item; //index to the currently selected item
    //int prevSelectedItem; //index to the previously selected item

    void DrawItem(c_item_container *item, int draw_border, float x, float y, float z, float c = 1.0f);

    float scrollOffset;
    bool scroll_changed_page;
    bool scrollRepeat;
    void update_scroll(double dt);

    void update_menu(double dt);
    void update_zoom(double dt);
    void update_border_color(double dt);

    std::vector<c_item_container*> item_containers;

    double scrollDuration;
    static const float zoomDuration;
    static const float borderDuration;

    enum {
        SCROLL_LEFT,
        SCROLL_RIGHT
    };
    int scroll_dir;
    double scrollTimer;
    double zoomTimer;
    enum {
        ZOOMING_OUT,
        ZOOMING_IN
    };
    int zoom_dir;
    float zoomStartX;
    float zoomStartY;
    float zoomStartZ;

    double borderTimer;
    int borderDir;

    HRESULT hr;

    ID3D10Effect *effect;
    ID3D10Texture2D *tex;
    ID3D10Texture2D *tex_blank;
    ID3D10EffectTechnique *technique;
    ID3D10EffectMatrixVariable *varWorld;
    ID3D10EffectMatrixVariable *varView;
    ID3D10EffectMatrixVariable *varProj;
    ID3D10EffectScalarVariable *varColor;
    ID3D10EffectVectorVariable *var_output_size;
    ID3D10EffectVectorVariable *var_border_color;
    ID3D10EffectVectorVariable *var_overscan_color;
    ID3D10EffectScalarVariable *var_sharpness;
    ID3D10EffectScalarVariable *var_max_y;
    ID3D10EffectScalarVariable *var_max_x;

    ID3D10EffectShaderResourceVariable *varTex;

    ID3D10Buffer *vertexBuffer;
    ID3D10ShaderResourceView *texRv;
    ID3D10InputLayout *vertexLayout;

    D3DXMATRIX matrixWorld;

    int valid_chars[27];

    D3D10_BUFFER_DESC bd;

    D3DXVECTOR3 border_color, border_color_start, border_color_end;
    D3DXVECTOR3 invalid_border_color, invalid_border_color_start, invalid_border_color_end;
    D3DXVECTOR3 border_colors[4];
};
