import interpolate;
using namespace interpolate;

#include "TexturePanel.h"
#include "effect.fxo.h"
#include <string>


extern ID3D10Device *d3dDev;
extern D3DXMATRIX matrixView;
extern D3DXMATRIX matrixProj;

const float TexturePanel::c_item_container::selectDuration = 120.0f;
const float TexturePanel::zoomDuration = 250.0f;
const float TexturePanel::borderDuration = 750.0f;


TexturePanel::c_item_container::c_item_container(TexturePanelItem *item)
{
    pos = { 0.0f, 0.0f, 0.0f };
    ratio = 1.333f;
    this->item = item;
    selecting = false;
    selectTimer = 0.0f;
    selected = false;
    select_dir = DIR_SELECTING;
}

TexturePanel::c_item_container::~c_item_container()
{
}

void TexturePanel::c_item_container::Select()
{
    selecting = true;
    selectTimer = 0.0f;
    select_dir = DIR_SELECTING;
}

void TexturePanel::c_item_container::Unselect()
{
    if (selected || (selecting && select_dir == DIR_SELECTING))
    {
        selecting = true;
        selectTimer = 0.0f;
        select_dir = DIR_UNSELECTING;
    }
}

TexturePanel::TexturePanel(int rows, int columns)
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

TexturePanel::~TexturePanel()
{
    for (auto &item : item_containers)
    {
        delete item;
    }
}


int TexturePanel::get_num_items()
{
    return (int)item_containers.size();
}

void TexturePanel::Init()
{
    D3D10_INPUT_ELEMENT_DESC elementDesc[] =
    {
        { TEXT("POSITION"), 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D10_INPUT_PER_VERTEX_DATA, 0 },
        { TEXT("TEXCOORD"), 0, DXGI_FORMAT_R32G32_FLOAT,    0, 12, D3D10_INPUT_PER_VERTEX_DATA, 0 }
    };
    UINT numElements = sizeof(elementDesc) / sizeof(elementDesc[0]);

    //hr = D3DX10CreateEffectFromFile("effect.fx", NULL, NULL, "fx_4_0", 0, 0, d3dDev, NULL, NULL, &effect, NULL, NULL);
    hr = D3DX10CreateEffectFromMemory((LPCVOID)g_effect1, sizeof(g_effect1), "effect1", NULL, NULL, "fx_4_0", 0, 0, d3dDev, NULL, NULL, &effect, NULL, NULL);
    if (FAILED(hr))
    {
        MessageBox(NULL, "D3DX10CreateEffectFromFile failed", 0, 0);
        PostQuitMessage(0);
        return;
    }
    technique = effect->GetTechniqueByName("Render");

    varWorld = effect->GetVariableByName("World")->AsMatrix();
    varView = effect->GetVariableByName("View")->AsMatrix();
    varProj = effect->GetVariableByName("Projection")->AsMatrix();
    varTex = effect->GetVariableByName("txDiffuse")->AsShaderResource();
    varColor = effect->GetVariableByName("color")->AsScalar();
    var_output_size = effect->GetVariableByName("output_size")->AsVector();
    var_border_color = effect->GetVariableByName("border_color")->AsVector();
    var_sharpness = effect->GetVariableByName("sharpness")->AsScalar();
    var_max_y = effect->GetVariableByName("max_y")->AsScalar();
    var_max_x = effect->GetVariableByName("max_x")->AsScalar();

    D3D10_PASS_DESC passDesc;
    technique->GetPassByIndex(0)->GetDesc(&passDesc);

    hr = d3dDev->CreateInputLayout(elementDesc, numElements, passDesc.pIAInputSignature, passDesc.IAInputSignatureSize, &vertexLayout);
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


    bd.Usage = D3D10_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(SimpleVertex) * 4;
    bd.BindFlags = D3D10_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = 0;
    bd.MiscFlags = 0;

    D3D10_SUBRESOURCE_DATA initData;
    initData.pSysMem = vertices;
    hr = d3dDev->CreateBuffer(&bd, &initData, &vertexBuffer);

    D3D10_TEXTURE2D_DESC texDesc;
    texDesc.Width = 512;
    texDesc.Height = 512;
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 1;
    texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    texDesc.SampleDesc.Count = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Usage = D3D10_USAGE_DYNAMIC;
    texDesc.BindFlags = D3D10_BIND_SHADER_RESOURCE;
    texDesc.CPUAccessFlags = D3D10_CPU_ACCESS_WRITE;
    texDesc.MiscFlags = 0;

    hr = d3dDev->CreateTexture2D(&texDesc, 0, &tex);
    if (FAILED(hr)) {
        MessageBox(NULL, "CreateTexture2D failed", 0, 0);
        PostQuitMessage(0);
        return;
    }

    D3D10_SHADER_RESOURCE_VIEW_DESC descRv;
    descRv.Format = texDesc.Format;
    descRv.ViewDimension = D3D10_SRV_DIMENSION_TEXTURE2D;
    descRv.Texture2D.MipLevels = 1;
    descRv.Texture2D.MostDetailedMip = 0;
    hr = d3dDev->CreateShaderResourceView(tex, &descRv, &texRv);
    if (FAILED(hr)) {
        MessageBox(NULL, "CreateShaderResourceView failed", 0, 0);
        PostQuitMessage(0);
        return;
    }

    D3DXMatrixIdentity(&matrixWorld);
    varView->SetMatrix((float*)&matrixView);
    varProj->SetMatrix((float*)&matrixProj);
    varTex->SetResource(texRv);

    set_sharpness(0.0f);
}

void TexturePanel::set_sharpness(float factor)
{
    if (factor < 0.0f)
        factor = 0.0f;
    else if (factor > 1.0f)
        factor = 1.0f;

    var_sharpness->SetFloat(factor);
}

bool TexturePanel::Changed()
{
    if (changed) {
        changed = false;
        return true;
    }
    else {
        return false;
    }
}

void TexturePanel::AddItem(TexturePanelItem *item)
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

int *TexturePanel::get_valid_chars()
{
    return valid_chars;
}

void TexturePanel::GetActive(std::list<TexturePanelItem*> *itemList)
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

void TexturePanel::load_items()
{
    for (int i = first_item; i < last_item; i++)
        item_containers[i]->item->Load();
    changed = true;
}

void TexturePanel::update_scroll(double dt)
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

void TexturePanel::update_menu(double dt)
{
    if (state != prevState) {
        changed = true;
        prevState = state;
    }
    int p = on_first_page ? rows : 0;

    for (int i = first_item; i < last_item; ++i, ++p) {
        float tile_x = x + scrollOffset + (p / rows) * tile_width;
        float tile_y = y + (p % rows) * -tile_height;

        D3DXVECTOR3 tile_pos = { tile_x, tile_y, 0.0f };
        D3DXVECTOR3 popped_pos = lerp(tile_pos, { zoomDestX, zoomDestY, zoomDestZ }, .3);

        if (item_containers[i]->selecting) {
            D3DXVECTOR3 pos[2] = { tile_pos, popped_pos };
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

void TexturePanel::update_zoom(double dt)
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
        D3DXVECTOR3 zoom_pos[2] = { { zoomStartX, zoomStartY, zoomStartZ }, { zoomDestX, zoomDestY, zoomDestZ } };
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

void TexturePanel::update_border_color(double dt)
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

void TexturePanel::Update(double dt)
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

void TexturePanel::Draw()
{
    varProj->SetMatrix((float*)&matrixProj);
    varView->SetMatrix((float*)&matrixView);

    switch (state)
    {
    case STATE_SCROLLING:
    case STATE_MENU:
    case STATE_ZOOMING:
        for (int i = first_item; i < last_item; ++i) {
            c_item_container *p = item_containers[i];
            if (in_focus && i == selected_item) {
                var_border_color->SetFloatVector((float *)(p->item->Selectable() ? &border_color : &invalid_border_color));
                DrawItem(p, state != STATE_ZOOMING, p->pos.x, p->pos.y, p->pos.z, 1.0f);
            }
            else {
                DrawItem(p, 0, p->pos.x, p->pos.y, p->pos.z, .45f);
            }
        }
        break;
    case STATE_ZOOMED:
        if (in_focus) {
            c_item_container *p = item_containers[selected_item];
            DrawItem(p, 0, p->pos.x, p->pos.y, p->pos.z);
        }
        break;
    }

}

void TexturePanel::DrawItem(c_item_container *item, int draw_border, float x, float y, float z, float c)
{
    UINT stride = sizeof(SimpleVertex);
    UINT offset = 0;
    d3dDev->IASetInputLayout(vertexLayout);
    ID3D10Buffer *b = item->item->get_vertex_buffer(0);
    
    d3dDev->IASetVertexBuffers(0, 1, &b, &stride, &offset);
    d3dDev->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    if (dim) {
        c *= .09f;
    }

    varColor->SetFloat(c);

    D3D10_VIEWPORT vp;
    UINT num_vp = 1;
    d3dDev->RSGetViewports(&num_vp, &vp);
    D3DXVECTOR3 topleft, bottomright;
    D3DXMATRIX identity;
    D3DXMatrixIdentity(&identity);
    auto v3 = D3DXVECTOR3(x - 4.0f / 3.0f, y - 1.0f, z);
    D3DXVec3Project(&topleft, &v3, &vp, &matrixProj, &matrixView, &identity);
    v3 = D3DXVECTOR3(x + 4.0f / 3.0f, y + 1.0f, z);
    D3DXVec3Project(&bottomright, &v3, &vp, &matrixProj, &matrixView, &identity);
    float quad_width = bottomright.x - topleft.x;
    float quad_height = topleft.y - bottomright.y;
    auto v2 = D3DXVECTOR2(quad_width, quad_height);
    var_output_size->SetFloatVector((float*)&v2);
    //D3DXCOLOR oc = item->item->get_overscan_color();
    //var_overscan_color->SetFloatVector((float*)&item->item->get_overscan_color());
    var_max_y->SetFloat((float)item->item->get_height());
    var_max_x->SetFloat((float)item->item->get_width());

    item->item->DrawToTexture(tex);
    D3DXMatrixTranslation(&matrixWorld, x, y, z);
    varWorld->SetMatrix((float*)&matrixWorld);
    technique->GetPassByIndex(0)->Apply(0);
    d3dDev->Draw(4, 0);
    if (draw_border) {
        d3dDev->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
        d3dDev->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
        technique->GetPassByIndex(1)->Apply(0);
        d3dDev->Draw(4, 0);
    }
}

int TexturePanel::NextRow(bool adjusting)
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

bool TexturePanel::is_first_col()
{
    return selected_item < rows;
}

bool TexturePanel::is_last_col()
{
    return (selected_item / rows == (numItems - 1) / rows);
}

int TexturePanel::PrevRow(bool adjusting)
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

void TexturePanel::move_to_char(char c)
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

void TexturePanel::move_to_column(int column)
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

void TexturePanel::NextColumn(bool load, bool adjusting)
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

void TexturePanel::PrevColumn(bool load, bool adjusting)
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

int TexturePanel::GetSelectedColumn()
{
    int c = (selected_item - first_item) / rows;
    if (on_first_page)
        c += 1;
    return c;
}

void TexturePanel::Zoom()
{
    if (state == STATE_ZOOMED)
        zoom_dir = ZOOMING_IN;
    else
        zoom_dir = ZOOMING_OUT;
    prevState = state;
    state = STATE_ZOOMING;
}

TexturePanelItem* TexturePanel::GetSelected()
{
    return item_containers[selected_item]->item;
}