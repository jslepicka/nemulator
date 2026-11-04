module;
#include <string>
#include "d3d11.h"
#include <DirectXMath.h>
#include "transform_vs.fxo.h"
#include "tile_ps.fxo.h"
#include "tile_border_ps.fxo.h"
#define ReleaseCOM(x) { if(x) {x->Release(); x = 0; } }


module texture_panel;
import interpolate;
using namespace interpolate;


extern ID3D11Device *d3dDev;
extern ID3D11DeviceContext *d3dContext;
extern DirectX::XMFLOAT4X4 matrixView;
extern DirectX::XMFLOAT4X4 matrixProj;

const float c_texture_panel::c_item_container::selectDuration = 120.0f;
const float c_texture_panel::zoomDuration = 250.0f;
const float c_texture_panel::borderDuration = 750.0f;


c_texture_panel::c_item_container::c_item_container(c_texture_panel_item *item)
{
    pos = { 0.0f, 0.0f, 0.0f };
    ratio = 1.333f;
    this->item = item;
    selecting = false;
    selectTimer = 0.0f;
    selected = false;
    select_dir = DIR_SELECTING;
}

c_texture_panel::c_item_container::~c_item_container()
{
}

void c_texture_panel::c_item_container::Select()
{
    selecting = true;
    selectTimer = 0.0f;
    select_dir = DIR_SELECTING;
}

void c_texture_panel::c_item_container::Unselect()
{
    if (selected || (selecting && select_dir == DIR_SELECTING))
    {
        selecting = true;
        selectTimer = 0.0f;
        select_dir = DIR_UNSELECTING;
    }
}

c_texture_panel::c_texture_panel(int rows, int columns)
{
    changed = false;
    prev_in_focus = false;
    in_focus = false;
    on_first_page = true;
    first_item = 0;
    last_item = 0;
    numItems = 0;
    selected_item = 0;
    prevState = STATE_NULL;
    state = STATE_MENU;
    next = NULL;
    prev = NULL;
    scrollTimer = 0.0f;
    scroll_changed_page = false;
    scrollRepeat = false;
    zoomTimer = 0.0f;
    zoom_dir = ZOOMING_OUT;
    scrolls = 0;
    selectable = false;

    scanlines = false;

    //todo: document where these coords come from
    //doesn't really matter since they're recalculated before use in nemulator.cpp
    zoomDestX = 16.2f;
    zoomDestY = -7.14f;
    zoomDestZ = -25.0f + 2.414f;
    
    border_colors[0] = { .71f * .05f, .78f * .05f, 1.0f * .05f }; //selectable start color
    border_colors[1] = { .71f * .9f, .78f * .9f, 1.0f * .9f }; //selectable end color
    border_colors[2] = { .05f, 0.0f, 0.0f }; //unselectable start color
    border_colors[3] = { 1.0f, 0.0f, 0.0f }; //unselectable end color

    border_color = border_colors[0];
    invalid_border_color = border_colors[2];

    scrollDuration = 180.0f;

    borderTimer = 0.0f;
    borderDir = 0;

    this->rows = rows;
    this->columns = columns;

    dim = false;

    memset(valid_chars, 0, sizeof(valid_chars));

    Init();

    scrollOffset = 0.0f;
}

c_texture_panel::~c_texture_panel()
{
    ReleaseCOM(transform_buffer);
    ReleaseCOM(constants_buffer);
    ReleaseCOM(sampler);
    for (auto &item : item_containers)
    {
        delete item;
    }
}


int c_texture_panel::get_num_items()
{
    return (int)item_containers.size();
}

void c_texture_panel::Init()
{
    D3D11_INPUT_ELEMENT_DESC elementDesc[] =
    {
        { TEXT("POSITION"), 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { TEXT("TEXCOORD"), 0, DXGI_FORMAT_R32G32_FLOAT,    0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };
    UINT numElements = sizeof(elementDesc) / sizeof(elementDesc[0]);

    D3D11_BLEND_DESC no_blend = no_blend_desc();
    D3D11_BLEND_DESC alpha_blend = alpha_blend_desc();
    D3D11_DEPTH_STENCIL_DESC depth = {};
    depth.DepthEnable = TRUE;
    depth.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    depth.DepthFunc = D3D11_COMPARISON_LESS;
    bool created = tile_pass.create({g_transform_vs, sizeof(g_transform_vs), g_tile_ps, sizeof(g_tile_ps), &no_blend, &depth});
    //the border is at the same depth as the tile it's drawn over
    depth.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
    created = created && border_pass.create({g_transform_vs, sizeof(g_transform_vs), g_tile_border_ps, sizeof(g_tile_border_ps),
                                             &alpha_blend, &depth});
    transform_buffer = create_constant_buffer<s_transform_constants>();
    constants_buffer = create_constant_buffer<s_constants>();
    sampler = create_linear_sampler(D3D11_TEXTURE_ADDRESS_BORDER);
    if (!created || !transform_buffer || !constants_buffer || !sampler)
    {
        MessageBox(NULL, "Unable to create shaders", 0, 0);
        PostQuitMessage(0);
        return;
    }

    hr = d3dDev->CreateInputLayout(elementDesc, numElements, g_transform_vs, sizeof(g_transform_vs), &vertexLayout);
    if (FAILED(hr)) {
        MessageBox(NULL, "CreateInputLayout failed", 0, 0);
        PostQuitMessage(0);
        return;
    }

    SimpleVertex vertices[] = {
        { {-4.0f / 3.0f, -1.0f, 0.0f}, {0.0f, 1.0f} },
        { {-4.0f / 3.0f, 1.0f, 0.0f}, {0.0f, 0.0f} },
        { {4.0f / 3.0f, -1.0f, 0.0f}, {1.0f, 1.0f} },
        { {4.0f / 3.0f, 1.0f, 0.0f}, {1.0f, 0.0f} },
    };


    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(SimpleVertex) * 4;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = 0;
    bd.MiscFlags = 0;

    D3D11_SUBRESOURCE_DATA initData;
    initData.pSysMem = vertices;
    hr = d3dDev->CreateBuffer(&bd, &initData, &vertexBuffer);

    D3D11_TEXTURE2D_DESC texDesc;
    texDesc.Width = 512;
    texDesc.Height = 512;
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 1;
    texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    texDesc.SampleDesc.Count = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Usage = D3D11_USAGE_DYNAMIC;
    texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    texDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    texDesc.MiscFlags = 0;

    hr = d3dDev->CreateTexture2D(&texDesc, 0, &tex);
    if (FAILED(hr)) {
        MessageBox(NULL, "CreateTexture2D failed", 0, 0);
        PostQuitMessage(0);
        return;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC descRv;
    descRv.Format = texDesc.Format;
    descRv.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    descRv.Texture2D.MipLevels = 1;
    descRv.Texture2D.MostDetailedMip = 0;
    hr = d3dDev->CreateShaderResourceView(tex, &descRv, &texRv);
    if (FAILED(hr)) {
        MessageBox(NULL, "CreateShaderResourceView failed", 0, 0);
        PostQuitMessage(0);
        return;
    }

    set_sharpness(0.0f);
}

void c_texture_panel::set_sharpness(float factor)
{
    if (factor < 0.0f)
        factor = 0.0f;
    else if (factor > 1.0f)
        factor = 1.0f;

    constants.sharpness = factor;
}

bool c_texture_panel::Changed()
{
    if (changed) {
        changed = false;
        return true;
    }
    else {
        return false;
    }
}

void c_texture_panel::AddItem(c_texture_panel_item *item)
{
    item_containers.push_back(new c_item_container(item));

    int num_items_on_page = last_item - first_item;
    int max_items_on_page = (rows * columns) - (on_first_page ? rows : 0);

    //if (last_item - first_item < rows*columns - rows)
    if (num_items_on_page < max_items_on_page) {
        item->Activate(true);
        last_item++;
    }
    numItems++;
    changed = true;
    selectable = true;

    char c = toupper(item->get_description().c_str()[0]);
    if (c < 'A')
        c = '0';
    else if (c > 'Z')
        c = 'Z';
    valid_chars[c == '0' ? 0 : c - 64] = 1;
}

//removes all items and unloads any that are active
void c_texture_panel::clear()
{
    for (auto &container : item_containers) {
        //items can be activated more than once (and left active off-page by move_to_char),
        //so deactivate until the item is actually unloaded
        while (container->item->is_active)
            container->item->Deactivate();
        delete container;
    }
    item_containers.clear();

    numItems = 0;
    first_item = 0;
    last_item = 0;
    selected_item = 0;
    on_first_page = true;
    state = STATE_MENU;
    prevState = STATE_NULL;
    scrollTimer = 0.0f;
    scrollOffset = 0.0f;
    scroll_changed_page = false;
    scrollRepeat = false;
    scrolls = 0;
    selectable = false;
    prev_in_focus = false; //selects the first item on the next update
    memset(valid_chars, 0, sizeof(valid_chars));
    changed = true;
}

int *c_texture_panel::get_valid_chars()
{
    return valid_chars;
}

void c_texture_panel::GetActive(std::list<c_texture_panel_item*> *itemList)
{
    if (state == STATE_ZOOMED) {
        if (in_focus)
            itemList->push_back(item_containers[selected_item]->item);
    }
    else {
        for (int i = first_item; i < last_item; i++)
            itemList->push_back(item_containers[i]->item);
    }
}

void c_texture_panel::load_items()
{
    for (int i = first_item; i < last_item; i++)
        item_containers[i]->item->Load();
    changed = true;
}

void c_texture_panel::update_scroll(double dt)
{
    scrollOffset = 0.0f;
    scrollTimer += dt;
    if (scrollTimer >= scrollDuration) {
        state = STATE_MENU;
        scrollTimer = 0.0f;
        if (scroll_changed_page) {
            if (scroll_dir == SCROLL_RIGHT) {
                for (int i = 0; i < rows; ++i)
                    item_containers[first_item++]->item->Deactivate();

            }
            else if (scroll_dir == SCROLL_LEFT) {
                for (int i = 0; i < rows; i++) {
                    if (last_item - first_item > rows * columns) {
                        item_containers[last_item - 1]->item->Deactivate();
                        last_item--;
                    }
                }
            }
            scroll_changed_page = false;
            changed = true;
        }
        if (--scrolls > 0)
            scrollRepeat = true;
    }
    else {
        double mu = scrollTimer / scrollDuration;
        if (scroll_dir == SCROLL_RIGHT)
            scrollOffset = interpolate_cosine(tile_width, 0.0f, mu);
        else
            scrollOffset = interpolate_cosine(-tile_width, 0.0f, mu);
    }
    if (scroll_changed_page && scroll_dir == SCROLL_RIGHT)
        scrollOffset -= tile_width;
}

void c_texture_panel::update_menu(double dt)
{
    if (state != prevState) {
        changed = true;
        prevState = state;
    }
    int p = on_first_page ? rows : 0;

    for (int i = first_item; i < last_item; ++i, ++p) {
        float tile_x = x + scrollOffset + (p / rows) * tile_width;
        float tile_y = y + (p % rows) * -tile_height;

        s_vec3 tile_pos = { tile_x, tile_y, 0.0f };
        s_vec3 popped_pos = lerp(tile_pos, { zoomDestX, zoomDestY, zoomDestZ }, .3);

        if (item_containers[i]->selecting) {
            s_vec3 pos[2] = { tile_pos, popped_pos };
            item_containers[i]->selectTimer += dt;
            int dir = item_containers[i]->select_dir;

            double mu = (item_containers[i]->selectTimer / item_containers[i]->selectDuration);
            item_containers[i]->pos = interpolate_cosine(pos[0 ^ dir], pos[1 ^ dir], mu);

            if (item_containers[i]->selectTimer >= item_containers[i]->selectDuration) {
                item_containers[i]->selecting = false;
                
                //if we were selecting, mark it as selected at the end of the pop
                //otherwise, mark it as not selected
                if (item_containers[i]->select_dir == c_item_container::DIR_SELECTING) {
                    item_containers[i]->selected = true;
                }
                else {
                    item_containers[i]->selected = false;
                }

                item_containers[i]->selectTimer = 0.0f;
            }
        }
        else if (item_containers[i]->selected) {
            //this really shouldn't be necessary as the only way to get into a selected state is by
            //going through the interpolation above, but there is some difference in the interpolated
            //and final value.  Maybe floating point error?
            item_containers[i]->pos = popped_pos;
        }
        else {
            item_containers[i]->pos = tile_pos;
        }

    }
}

void c_texture_panel::update_zoom(double dt)
{
    float non_selected_pos[2] = { 0.0f, 10.0f };
    float non_selected_z = 0.0f;
    if (state != prevState) {
        if (zoom_dir == ZOOMING_OUT) {
            zoomStartX = item_containers[selected_item]->pos.x;
            zoomStartY = item_containers[selected_item]->pos.y;
            zoomStartZ = item_containers[selected_item]->pos.z;
        }
        prevState = state;
        changed = true;
    }

    zoomTimer += dt;
    double mu = zoomTimer / zoomDuration;
    non_selected_z = lerp(non_selected_pos[0 ^ zoom_dir], non_selected_pos[1 ^ zoom_dir], mu);
    if (in_focus) {
        c_item_container *i = item_containers[selected_item];
        s_vec3 zoom_pos[2] = { { zoomStartX, zoomStartY, zoomStartZ }, { zoomDestX, zoomDestY, zoomDestZ } };
        item_containers[selected_item]->pos = lerp(zoom_pos[0 ^ zoom_dir], zoom_pos[1 ^ zoom_dir], mu);
    }

    if (zoomTimer >= zoomDuration) {
        zoomTimer = 0.0f;
        prevState = state;
        state = zoom_dir == ZOOMING_OUT ? STATE_ZOOMED : STATE_MENU;
        changed = true;

    }

    for (int i = first_item; i < last_item; ++i) {
        if (!(in_focus && i == selected_item))
            item_containers[i]->pos.z = non_selected_z;
    }
}

void c_texture_panel::update_border_color(double dt)
{
    borderTimer += dt;
    double mu = borderTimer / borderDuration;
    border_color = interpolate_cosine(border_colors[0 ^ borderDir], border_colors[1 ^ borderDir], mu);
    invalid_border_color = interpolate_cosine(border_colors[2 ^ borderDir], border_colors[3 ^ borderDir], mu);
    if (borderTimer >= borderDuration) {
        borderDir ^= 1;
        borderTimer = 0.0f;
    }
}

void c_texture_panel::Update(double dt)
{
    if (item_containers.size() == 0)
        return;
    //this allows for more than one scroll to be enqueued per frame
    if (scrollRepeat) {
        if (scroll_dir == SCROLL_RIGHT)
            NextColumn();
        else
            PrevColumn();
        scrollRepeat = false;
    }

    if (in_focus != prev_in_focus) {
        if (in_focus)
            item_containers[selected_item]->Select();
        else
            item_containers[selected_item]->Unselect();
        prev_in_focus = in_focus;
    }

    update_border_color(dt);

    switch (state)
    {
    case STATE_SCROLLING:
        update_scroll(dt);
        update_menu(dt);
        break;
    case STATE_MENU:
        update_menu(dt);
        break;
    case STATE_ZOOMING:
        update_zoom(dt);
        break;
    }
}

void c_texture_panel::Draw()
{
    transform.projection = matrixProj;
    transform.view = matrixView;
    //nothing else draws until the panel is done, so these stay set for every item
    d3dContext->VSSetConstantBuffers(0, 1, &transform_buffer);
    d3dContext->PSSetConstantBuffers(1, 1, &constants_buffer);
    d3dContext->PSSetShaderResources(0, 1, &texRv);
    d3dContext->PSSetSamplers(0, 1, &sampler);

    switch (state)
    {
    case STATE_SCROLLING:
    case STATE_MENU:
    case STATE_ZOOMING:
        for (int i = first_item; i < last_item; ++i) {
            c_item_container *p = item_containers[i];
            constants.scanlines = FALSE;
            if (in_focus && i == selected_item) {
                constants.border_color = p->item->Selectable() ? border_color : invalid_border_color;
                DrawItem(p, state != STATE_ZOOMING, p->pos.x, p->pos.y, p->pos.z, 1.0f);
            }
            else {
                DrawItem(p, 0, p->pos.x, p->pos.y, p->pos.z, .45f);
            }
        }
        break;
    case STATE_ZOOMED:
        if (in_focus) {
            constants.scanlines = scanlines;
            c_item_container *p = item_containers[selected_item];
            DrawItem(p, 0, p->pos.x, p->pos.y, p->pos.z);
        }
        break;
    }

}

void c_texture_panel::DrawItem(c_item_container *item, int draw_border, float x, float y, float z, float c)
{
    UINT stride = sizeof(SimpleVertex);
    UINT offset = 0;
    d3dContext->IASetInputLayout(vertexLayout);
    ID3D11Buffer *b = item->item->get_vertex_buffer(0);
    
    d3dContext->IASetVertexBuffers(0, 1, &b, &stride, &offset);
    d3dContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    if (dim) {
        c *= .09f;
    }

    constants.color = c;

    D3D11_VIEWPORT vp;
    UINT num_vp = 1;
    d3dContext->RSGetViewports(&num_vp, &vp);
    auto project = [&vp](float px, float py, float pz) {
        return DirectX::XMVector3Project(DirectX::XMVectorSet(px, py, pz, 1.0f), vp.TopLeftX, vp.TopLeftY,
                                         vp.Width, vp.Height, vp.MinDepth, vp.MaxDepth,
                                         DirectX::XMLoadFloat4x4(&matrixProj), DirectX::XMLoadFloat4x4(&matrixView),
                                         DirectX::XMMatrixIdentity());
    };
    DirectX::XMVECTOR topleft = project(x - 4.0f / 3.0f, y - 1.0f, z);
    DirectX::XMVECTOR bottomright = project(x + 4.0f / 3.0f, y + 1.0f, z);
    constants.output_size = {DirectX::XMVectorGetX(bottomright) - DirectX::XMVectorGetX(topleft),
                             DirectX::XMVectorGetY(topleft) - DirectX::XMVectorGetY(bottomright)};
    constants.max_y = (float)item->item->get_height();
    constants.max_x = (float)item->item->get_width();

    item->item->DrawToTexture(tex);
    DirectX::XMStoreFloat4x4(&transform.world, DirectX::XMMatrixTranslation(x, y, z));
    d3dContext->UpdateSubresource(transform_buffer, 0, NULL, &transform, 0, 0);
    d3dContext->UpdateSubresource(constants_buffer, 0, NULL, &constants, 0, 0);
    tile_pass.apply();
    d3dContext->Draw(4, 0);
    if (draw_border) {
        d3dContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
        d3dContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
        border_pass.apply();
        d3dContext->Draw(4, 0);
    }
}

int c_texture_panel::NextRow(bool adjusting)
{
    if (!adjusting && state != STATE_MENU)
        return 0;
    if ((selected_item + 1) % rows != 0 && selected_item + 1 != numItems)
    {
        item_containers[selected_item]->Unselect();

        selected_item++;
        if (!adjusting) item_containers[selected_item]->Select();
        return 0;
    }
    else
    {
        if (next && next->selectable)
        {
            item_containers[selected_item]->Unselect();

            in_focus = false;
            next->in_focus = true;
            return 1;
        }
        return 0;
    }

}

bool c_texture_panel::is_first_col()
{
    return selected_item < rows;
}

bool c_texture_panel::is_last_col()
{
    return (selected_item / rows == (numItems - 1) / rows);
}

int c_texture_panel::PrevRow(bool adjusting)
{
    if (!adjusting && state != STATE_MENU)
        return 0;
    if (selected_item % rows != 0)
    {
        item_containers[selected_item]->Unselect();
        selected_item--;
        if (!adjusting) item_containers[selected_item]->Select();
        return 0;
    }
    else
    {
        if (prev && prev->selectable)
        {
            item_containers[selected_item]->Unselect();
            in_focus = false;
            prev->in_focus = true;
            return 1;
        }
        return 1;
    }
}

void c_texture_panel::move_to_char(char c)
{
    c = toupper(c);
    int current_column = selected_item / rows;
    int current_row = selected_item % rows;
    //find column that c is in
    int destination_column = 0;
    int destination_row = 0;
    for (int i = 0; i < item_containers.size(); i++)
    {
        char d = toupper(item_containers[i]->item->get_description().c_str()[0]);
        if (d < 'A')
            d = '0';
        else if (d > 'Z')
            d = 'Z';
        if (d == c || d < c)
        {
            destination_column = i / rows;
            destination_row = i % rows;
            if (d == c)
                break;
        }
        else
            break;
    }

    int column_move = destination_column - current_column;
    int row_move = destination_row - current_row;

    if (column_move == 0 && row_move == 0)
        return;

    if (column_move > 0)
    {
        for (int i = 0; i < column_move; i++)
        {
            NextColumn(false, true);
            if (scroll_changed_page)
            {
                for (int i = 0; i < rows; ++i)
                    item_containers[first_item++]->item->Deactivate();
                scroll_changed_page = false;
            }
        }
    }
    else if (column_move < 0)
    {
        for (int i = 0; i < column_move * -1; i++)
        {
            PrevColumn(false, true);
            if (scroll_changed_page)
            {
                for (int i = 0; i < rows; i++)
                {
                    if (last_item - first_item > rows*columns)
                    {
                        item_containers[last_item - 1]->item->Deactivate();
                        last_item--;
                    }
                }
            }
        }
    }

    state = STATE_MENU;

    if (row_move > 0)
    {
        for (int i = 0; i < row_move; i++)
            NextRow(true);
    }
    else if (row_move < 0)
    {
        for (int i = 0; i < row_move * -1; i++)
            PrevRow(true);
    }


    item_containers[selected_item]->Select();
    for (int i = first_item; i < last_item; i++)
    {
        if (!item_containers[i]->item->is_active)
            item_containers[i]->item->Activate(true);
    }
    scrolls = 0;

}

void c_texture_panel::move_to_column(int column)
{
    int current_column = GetSelectedColumn();
    while (current_column != column)
    {
        if (column > current_column)
        {
            NextColumn(true, true);
            current_column++;
        }
        else
        {
            PrevColumn(true, true);
            current_column--;
        }
    }
}

void c_texture_panel::NextColumn(bool load, bool adjusting)
{
    if (scroll_dir == SCROLL_LEFT)
    {
        scrolls = 0;
        //state = STATE_MENU;
    }
    if (scroll_dir == SCROLL_RIGHT && state == STATE_SCROLLING && scrolls < 2)
        scrolls++;
    //don't allow changes while already scrolling
    if (!adjusting && state != STATE_MENU)
        return;
    //needed to prevent runaway scrolling
    if (scrolls)
        scrolls--;
    int change = selected_item;
    if (selected_item / rows != (numItems - 1) / rows) //not in the last row
    {
        item_containers[selected_item]->Unselect();
        selected_item += rows;
        if (selected_item > (numItems - 1))
            selected_item = numItems - 1;
        if (!adjusting) item_containers[selected_item]->Select();
        change = selected_item - change;
        if (selected_item - first_item >= (rows*columns) - rows) //if in the last row on this page
        {
            state = STATE_SCROLLING;
            scrolls++;
            scroll_dir = SCROLL_RIGHT;
            scroll_changed_page = true;
            for (int i = 0; i < rows; i++)
            {
                if (last_item < numItems /*&& !adjusting*/)
                {
                    if (!adjusting) item_containers[last_item]->item->Activate(load);
                    last_item++;
                }
            }
            changed = true;
        }
    }
    if (on_first_page && selected_item / rows > columns - 3)
    {
        state = STATE_SCROLLING;
        scrolls++;
        scroll_dir = SCROLL_RIGHT;
        for (int i = 0; i < rows; i++)
        {
            if (last_item < numItems/* && !adjusting*/)
            {
                if (!adjusting) item_containers[last_item]->item->Activate(load);
                last_item++;
            }
        }
        on_first_page = false;
        changed = true;
    }
}

void c_texture_panel::PrevColumn(bool load, bool adjusting)
{
    if (scroll_dir == SCROLL_RIGHT)
    {
        scrolls = 0;
        //state = STATE_MENU;
    }
    if (scroll_dir == SCROLL_LEFT && state == STATE_SCROLLING && scrolls < 2)
        scrolls++;
    if (!adjusting && state != STATE_MENU)
        return;
    if (scrolls)
        scrolls--;
    int change = selected_item;
    if (selected_item >= rows) //not in first column
    {
        item_containers[selected_item]->Unselect();
        selected_item -= rows;
        if (selected_item < 0)
            selected_item = 0;
        if (!adjusting) item_containers[selected_item]->Select();
        change = change - selected_item;
        if (first_item > 0 && selected_item - first_item < rows)
        {
            state = STATE_SCROLLING;
            scrolls++;
            scroll_dir = SCROLL_LEFT;
            scroll_changed_page = true;
            for (int i = 0; i < rows; i++)
            {
                --first_item;
                if (!adjusting) item_containers[first_item]->item->Activate(load);
                //first_item--;
            }
            changed = true;
        }
    }
    //transition to first page
    if (selected_item < rows && !on_first_page)
    {
        state = STATE_SCROLLING;
        scrolls++;
        scroll_dir = SCROLL_LEFT;
        for (int i = 0; i < rows; i++)
        {
            if (last_item - first_item > rows*columns - rows)
            {
                if (!adjusting) item_containers[last_item - 1]->item->Deactivate();
                last_item--;
            }

        }
        on_first_page = true;
        changed = true;
    }
}

int c_texture_panel::GetSelectedColumn()
{
    int c = (selected_item - first_item) / rows;
    if (on_first_page)
        c += 1;
    return c;
}

void c_texture_panel::Zoom()
{
    if (state == STATE_ZOOMED)
        zoom_dir = ZOOMING_IN;
    else
        zoom_dir = ZOOMING_OUT;
    prevState = state;
    state = STATE_ZOOMING;
}

c_texture_panel_item* c_texture_panel::GetSelected()
{
    return item_containers[selected_item]->item;
}