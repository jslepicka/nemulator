module;
#include <windows.h>
#include <usp10.h>
#include <d3d11.h>

export module font;
import nemulator.std;
import vec;

//text drawn with a GDI font.  glyphs are rasterized by GDI as they're first needed and kept in atlas textures.
export class c_font
{
  public:
    //height and weight are as in CreateFont
    c_font(int height, const char *face, int weight = 0);
    ~c_font();
    c_font(const c_font &) = delete;
    c_font &operator=(const c_font &) = delete;

    //text is in the ansi code page, and is broken into lines at newlines.  format takes these DT_ flags:
    //DT_LEFT (the default), DT_CENTER, DT_RIGHT, DT_TOP (the default), DT_VCENTER, DT_BOTTOM, DT_SINGLELINE.
    //rect only positions the text, which isn't clipped to it.
    void draw(const char *text, const RECT &rect, UINT format, const s_color &color);
    //the width in pixels of the widest line
    int measure(const char *text);

  private:
    struct s_placed_glyph
    {
        WORD glyph;
        int x;
    };
    struct s_line
    {
        std::vector<s_placed_glyph> glyphs;
        int width;
    };
    struct s_glyph
    {
        int page;
        int tex_x;
        int tex_y;
        int width; //0 for a glyph with nothing to draw, such as a space
        int height;
        int origin_x; //from the pen position to the bitmap's left edge
        int origin_y; //from the top of the line to the bitmap's top edge
    };
    struct s_page
    {
        ID3D11Texture2D *texture;
        ID3D11ShaderResourceView *view;
        int shelf_x;
        int shelf_y;
        int shelf_height;
    };

    s_line layout_line(const char *text, int length);
    std::vector<s_line> layout(const char *text, UINT format);
    const s_glyph &get_glyph(WORD index);
    bool place_in_page(int width, int height, int &page, int &x, int &y);

    HDC dc;
    HFONT font;
    HGDIOBJ previous_font;
    SCRIPT_CACHE script_cache;
    int line_height;
    int ascent;
    int page_size;
    std::unordered_map<WORD, s_glyph> glyphs;
    std::vector<s_page> pages;
};
