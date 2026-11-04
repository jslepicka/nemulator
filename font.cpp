module;
#include <windows.h>
#include <usp10.h>
#include <d3d11.h>
#include "text_vs.fxo.h"
#include "text_ps.fxo.h"

#pragma comment(lib, "usp10.lib")

#define ReleaseCOM(x) { if(x) { x->Release(); x = 0; } }

module font;

extern ID3D11Device *d3dDev;
extern ID3D11DeviceContext *d3dContext;
extern int clientWidth;
extern int clientHeight;

//GDI's 17 coverage levels (GGO_GRAY4_BITMAP) as alpha.  the curve keeps the edges of text solid when it's blended
//on the srgb back buffer.
static const unsigned char coverage_to_alpha[17] = {0x00, 0x00, 0x68, 0x7c, 0x8c, 0x9a, 0xa7, 0xb2, 0xbd,
                                                    0xc7, 0xd0, 0xd9, 0xe1, 0xe9, 0xf0, 0xf8, 0xff};

namespace
{
struct s_text_vertex
{
    float x;
    float y;
    float u;
    float v;
    s_color color;
};

//what's needed to draw text with any font.  held while any font exists.
struct s_pipeline
{
    static constexpr int max_vertices = 6 * 2048;

    int refs = 0;
    ID3D11VertexShader *vertex_shader = nullptr;
    ID3D11PixelShader *pixel_shader = nullptr;
    ID3D11InputLayout *input_layout = nullptr;
    ID3D11Buffer *vertex_buffer = nullptr;
    ID3D11BlendState *blend = nullptr;
    ID3D11DepthStencilState *depth = nullptr;
    ID3D11SamplerState *sampler = nullptr;
    int next_vertex = 0;

    void create()
    {
        HRESULT hr = d3dDev->CreateVertexShader(g_text_vs, sizeof(g_text_vs), NULL, &vertex_shader);
        if (SUCCEEDED(hr))
            hr = d3dDev->CreatePixelShader(g_text_ps, sizeof(g_text_ps), NULL, &pixel_shader);

        D3D11_INPUT_ELEMENT_DESC elements[] = {
            {"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0},
        };
        if (SUCCEEDED(hr))
            hr = d3dDev->CreateInputLayout(elements, 3, g_text_vs, sizeof(g_text_vs), &input_layout);

        D3D11_BUFFER_DESC buffer = {};
        buffer.Usage = D3D11_USAGE_DYNAMIC;
        buffer.ByteWidth = sizeof(s_text_vertex) * max_vertices;
        buffer.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        buffer.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        if (SUCCEEDED(hr))
            hr = d3dDev->CreateBuffer(&buffer, NULL, &vertex_buffer);

        D3D11_BLEND_DESC blend_desc = {};
        D3D11_RENDER_TARGET_BLEND_DESC &target = blend_desc.RenderTarget[0];
        target.BlendEnable = TRUE;
        target.SrcBlend = D3D11_BLEND_SRC_ALPHA;
        target.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        target.BlendOp = D3D11_BLEND_OP_ADD;
        target.SrcBlendAlpha = D3D11_BLEND_ONE;
        target.DestBlendAlpha = D3D11_BLEND_ONE;
        target.BlendOpAlpha = D3D11_BLEND_OP_ADD;
        target.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        if (SUCCEEDED(hr))
            hr = d3dDev->CreateBlendState(&blend_desc, &blend);

        D3D11_DEPTH_STENCIL_DESC depth_desc = {};
        depth_desc.DepthEnable = FALSE;
        depth_desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
        depth_desc.DepthFunc = D3D11_COMPARISON_LESS;
        if (SUCCEEDED(hr))
            hr = d3dDev->CreateDepthStencilState(&depth_desc, &depth);

        D3D11_SAMPLER_DESC sampler_desc = {};
        sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
        sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
        sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        sampler_desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
        sampler_desc.MaxLOD = D3D11_FLOAT32_MAX;
        if (SUCCEEDED(hr))
            hr = d3dDev->CreateSamplerState(&sampler_desc, &sampler);

        if (FAILED(hr)) {
            MessageBox(NULL, "Unable to load fonts", "Error", MB_OK);
            exit(1);
        }
    }

    void release()
    {
        ReleaseCOM(vertex_shader);
        ReleaseCOM(pixel_shader);
        ReleaseCOM(input_layout);
        ReleaseCOM(vertex_buffer);
        ReleaseCOM(blend);
        ReleaseCOM(depth);
        ReleaseCOM(sampler);
        next_vertex = 0;
    }

    void acquire()
    {
        if (refs++ == 0)
            create();
    }

    void drop()
    {
        if (--refs == 0)
            release();
    }

    //sets the state text needs.  callers restore anything they depend on: depth state in particular is left disabled.
    void apply()
    {
        UINT stride = sizeof(s_text_vertex);
        UINT offset = 0;
        d3dContext->IASetInputLayout(input_layout);
        d3dContext->IASetVertexBuffers(0, 1, &vertex_buffer, &stride, &offset);
        d3dContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        d3dContext->VSSetShader(vertex_shader, NULL, 0);
        d3dContext->GSSetShader(NULL, NULL, 0);
        d3dContext->PSSetShader(pixel_shader, NULL, 0);
        d3dContext->PSSetSamplers(0, 1, &sampler);
        float blend_factor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
        d3dContext->OMSetBlendState(blend, blend_factor, 0xFFFFFFFF);
        d3dContext->OMSetDepthStencilState(depth, 0);
    }

    //draws count vertices, which must all use view
    void draw_vertices(ID3D11ShaderResourceView *view, const s_text_vertex *vertices, int count)
    {
        d3dContext->PSSetShaderResources(0, 1, &view);
        while (count > 0) {
            int batch = (std::min)(count, max_vertices);
            D3D11_MAP map = D3D11_MAP_WRITE_NO_OVERWRITE;
            if (next_vertex + batch > max_vertices) {
                map = D3D11_MAP_WRITE_DISCARD;
                next_vertex = 0;
            }
            D3D11_MAPPED_SUBRESOURCE mapped;
            if (FAILED(d3dContext->Map(vertex_buffer, 0, map, 0, &mapped)))
                return;
            std::copy_n(vertices, batch, (s_text_vertex *)mapped.pData + next_vertex);
            d3dContext->Unmap(vertex_buffer, 0);
            d3dContext->Draw(batch, next_vertex);
            next_vertex += batch;
            vertices += batch;
            count -= batch;
        }
    }
};

s_pipeline pipeline;
} //namespace

c_font::c_font(int height, const char *face, int weight)
{
    pipeline.acquire();
    dc = CreateCompatibleDC(NULL);
    font = CreateFontA(height, 0, 0, 0, weight, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                       ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, face);
    previous_font = SelectObject(dc, font);
    script_cache = NULL;

    TEXTMETRICA metrics;
    GetTextMetricsA(dc, &metrics);
    line_height = metrics.tmHeight;
    ascent = metrics.tmAscent;

    //several glyphs to a page, even for a font as tall as the letter shown while fast scrolling, where the page
    //has to be at least large enough for one
    page_size = std::clamp((int)std::bit_ceil((unsigned)(std::max)(height, 1) * 4), 256, 2048);
    page_size = (std::max)(page_size, (int)std::bit_ceil((unsigned)(std::max)(height, 1) * 2));
}

c_font::~c_font()
{
    for (auto &page : pages) {
        ReleaseCOM(page.view);
        ReleaseCOM(page.texture);
    }
    ScriptFreeCache(&script_cache);
    SelectObject(dc, previous_font);
    DeleteObject(font);
    DeleteDC(dc);
    pipeline.drop();
}

//laid out with Uniscribe, whose kerning stops at spaces and punctuation, unlike GDI's GetCharacterPlacement.
//trailing spaces take up no room.
c_font::s_line c_font::layout_line(const char *text, int length)
{
    s_line line = {{}, 0};
    while (length > 0 && text[length - 1] == ' ')
        length--;
    if (length == 0)
        return line;

    int wide_length = MultiByteToWideChar(CP_ACP, 0, text, length, NULL, 0);
    std::vector<wchar_t> wide(wide_length);
    MultiByteToWideChar(CP_ACP, 0, text, length, wide.data(), wide_length);

    std::vector<SCRIPT_ITEM> items(wide_length + 1);
    int num_items = 0;
    if (FAILED(ScriptItemize(wide.data(), wide_length, (int)items.size(), NULL, NULL, items.data(), &num_items)))
        return line;

    std::vector<WORD> glyph_indices;
    std::vector<WORD> clusters;
    std::vector<SCRIPT_VISATTR> attributes;
    std::vector<int> advances;
    std::vector<GOFFSET> offsets;
    for (int i = 0; i < num_items; i++) {
        int start = items[i].iCharPos;
        int run_length = items[i + 1].iCharPos - start;
        int max_glyphs = run_length * 3 / 2 + 16;
        glyph_indices.resize(max_glyphs);
        clusters.resize(run_length);
        attributes.resize(max_glyphs);
        int num_glyphs = 0;
        SCRIPT_ANALYSIS &analysis = items[i].a;
        HRESULT hr = ScriptShape(dc, &script_cache, wide.data() + start, run_length, max_glyphs, &analysis,
                                 glyph_indices.data(), clusters.data(), attributes.data(), &num_glyphs);
        if (hr == USP_E_SCRIPT_NOT_IN_FONT) {
            //show whatever the font has for these characters
            analysis.eScript = SCRIPT_UNDEFINED;
            hr = ScriptShape(dc, &script_cache, wide.data() + start, run_length, max_glyphs, &analysis, glyph_indices.data(),
                             clusters.data(), attributes.data(), &num_glyphs);
        }
        if (FAILED(hr))
            continue;
        advances.resize(num_glyphs);
        offsets.resize(num_glyphs);
        ABC abc;
        if (FAILED(ScriptPlace(dc, &script_cache, glyph_indices.data(), num_glyphs, attributes.data(), &analysis,
                               advances.data(), offsets.data(), &abc)))
            continue;
        for (int g = 0; g < num_glyphs; g++) {
            line.glyphs.push_back({glyph_indices[g], line.width + offsets[g].du});
            line.width += advances[g];
        }
    }
    return line;
}

std::vector<c_font::s_line> c_font::layout(const char *text, UINT format)
{
    std::vector<s_line> lines;
    if (format & DT_SINGLELINE) {
        lines.push_back(layout_line(text, (int)strlen(text)));
        return lines;
    }
    while (true) {
        const char *end = strchr(text, '\n');
        lines.push_back(layout_line(text, end ? (int)(end - text) : (int)strlen(text)));
        if (!end)
            return lines;
        text = end + 1;
    }
}

int c_font::measure(const char *text)
{
    int width = 0;
    for (auto &line : layout(text, 0))
        width = (std::max)(width, line.width);
    return width;
}

//finds room for a glyph, starting a new page if it's needed.  glyphs fill rows from left to right.
bool c_font::place_in_page(int width, int height, int &page, int &x, int &y)
{
    if (width > page_size || height > page_size)
        return false;
    if (!pages.empty()) {
        s_page &p = pages.back();
        if (p.shelf_x + width > page_size) {
            p.shelf_y += p.shelf_height;
            p.shelf_x = 0;
            p.shelf_height = 0;
        }
        if (p.shelf_y + height <= page_size) {
            page = (int)pages.size() - 1;
            x = p.shelf_x;
            y = p.shelf_y;
            p.shelf_x += width;
            p.shelf_height = (std::max)(p.shelf_height, height);
            return true;
        }
    }

    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = page_size;
    desc.Height = page_size;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    s_page p = {};
    if (FAILED(d3dDev->CreateTexture2D(&desc, NULL, &p.texture)))
        return false;
    if (FAILED(d3dDev->CreateShaderResourceView(p.texture, NULL, &p.view))) {
        ReleaseCOM(p.texture);
        return false;
    }
    p.shelf_x = width;
    p.shelf_height = height;
    pages.push_back(p);
    page = (int)pages.size() - 1;
    x = 0;
    y = 0;
    return true;
}

const c_font::s_glyph &c_font::get_glyph(WORD index)
{
    auto found = glyphs.find(index);
    if (found != glyphs.end())
        return found->second;

    s_glyph glyph = {};
    GLYPHMETRICS metrics;
    static const MAT2 identity = {{0, 1}, {0, 0}, {0, 0}, {0, 1}};
    const UINT outline_format = GGO_GLYPH_INDEX | GGO_GRAY4_BITMAP;
    DWORD size = GetGlyphOutlineW(dc, index, outline_format, &metrics, 0, NULL, &identity);
    if (size != GDI_ERROR && size > 0) {
        std::vector<unsigned char> bitmap(size);
        GetGlyphOutlineW(dc, index, outline_format, &metrics, size, bitmap.data(), &identity);
        int width = metrics.gmBlackBoxX;
        int height = metrics.gmBlackBoxY;
        int source_pitch = (width + 3) & ~3;
        if (place_in_page(width, height, glyph.page, glyph.tex_x, glyph.tex_y)) {
            glyph.width = width;
            glyph.height = height;
            glyph.origin_x = metrics.gmptGlyphOrigin.x;
            glyph.origin_y = ascent - metrics.gmptGlyphOrigin.y;
            std::vector<unsigned char> alpha(width * height);
            for (int y = 0; y < height; y++) {
                for (int x = 0; x < width; x++)
                    alpha[y * width + x] = coverage_to_alpha[bitmap[y * source_pitch + x]];
            }
            D3D11_BOX box = {(UINT)glyph.tex_x, (UINT)glyph.tex_y, 0, (UINT)(glyph.tex_x + width), (UINT)(glyph.tex_y + height), 1};
            d3dContext->UpdateSubresource(pages[glyph.page].texture, 0, &box, alpha.data(), width, 0);
        }
    }
    return glyphs.emplace(index, glyph).first->second;
}

void c_font::draw(const char *text, const RECT &rect, UINT format, const s_color &color)
{
    std::vector<s_line> lines = layout(text, format);

    int total_height = (int)lines.size() * line_height;
    int line_y = rect.top;
    if (format & DT_VCENTER)
        line_y = rect.top + (rect.bottom - rect.top - total_height) / 2;
    else if (format & DT_BOTTOM)
        line_y = rect.bottom - total_height;

    //quads for each page, drawn in page order.  text doesn't overlap itself, so the order doesn't show.
    std::vector<std::vector<s_text_vertex>> page_vertices;
    const float scale_x = 2.0f / clientWidth;
    const float scale_y = 2.0f / clientHeight;
    for (auto &line : lines) {
        int line_x = rect.left;
        if (format & DT_CENTER)
            line_x = rect.left + (rect.right - rect.left - line.width) / 2;
        else if (format & DT_RIGHT)
            line_x = rect.right - line.width;

        for (auto &placed : line.glyphs) {
            const s_glyph &glyph = get_glyph(placed.glyph);
            if (glyph.width == 0)
                continue;
            if ((int)page_vertices.size() <= glyph.page)
                page_vertices.resize(glyph.page + 1);

            float left = (line_x + placed.x + glyph.origin_x) * scale_x - 1.0f;
            float right = (line_x + placed.x + glyph.origin_x + glyph.width) * scale_x - 1.0f;
            float top = 1.0f - (line_y + glyph.origin_y) * scale_y;
            float bottom = 1.0f - (line_y + glyph.origin_y + glyph.height) * scale_y;
            float u0 = (float)glyph.tex_x / page_size;
            float u1 = (float)(glyph.tex_x + glyph.width) / page_size;
            float v0 = (float)glyph.tex_y / page_size;
            float v1 = (float)(glyph.tex_y + glyph.height) / page_size;

            auto &out = page_vertices[glyph.page];
            out.push_back({left, top, u0, v0, color});
            out.push_back({right, top, u1, v0, color});
            out.push_back({left, bottom, u0, v1, color});
            out.push_back({left, bottom, u0, v1, color});
            out.push_back({right, top, u1, v0, color});
            out.push_back({right, bottom, u1, v1, color});
        }
        line_y += line_height;
    }

    pipeline.apply();
    for (size_t page = 0; page < page_vertices.size(); page++) {
        if (!page_vertices[page].empty())
            pipeline.draw_vertices(pages[page].view, page_vertices[page].data(), (int)page_vertices[page].size());
    }
}
