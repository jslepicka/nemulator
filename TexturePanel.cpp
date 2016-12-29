#include "TexturePanel.h"
#include "effect.fxo.h"
#include <string>

extern ID3D10Device *d3dDev;
extern D3DXMATRIX matrixView;
extern D3DXMATRIX matrixProj;

extern int clientHeight;
extern int clientWidth;

const float TexturePanel::c_item_container::selectDuration = 120.0f;
//const float TexturePanel::scrollDuration = 180.0f;
const float TexturePanel::zoomDuration = 250.0f;
const float TexturePanel::borderDuration = 750.0f;
//const float TexturePanel::c_item_container::selectDuration = 1000.0f;
//const float TexturePanel::scrollDuration = 3000.f;

static const float tile_width = 2.7f;//2.32f; //2.7f

TexturePanel::c_item_container::c_item_container(TexturePanelItem *item)
{
	x = y = z = 0.0f;
	ratio = 1.333f;
	this->item = item;
	selecting = false;
	selectTimer = 0.0f;
	selected = false;
}

TexturePanel::c_item_container::~c_item_container()
{
}

void TexturePanel::c_item_container::Select()
{
	selecting = true;
	selectTimer = 0.0f;
	selectDir = 0;
}

void TexturePanel::c_item_container::Unselect()
{
	if (selected || (selecting && selectDir == 0))
	{
		selecting = true;
		selectTimer = 0.0f;
		selectDir = 1;
	}
}

TexturePanel::TexturePanel(int rows, int columns)
{
	changed = false;
	prevSelected = false;
	selected = false;
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
	scrollCleanup = false;
	scrollRepeat = false;
	zoomTimer = 0.0f;
	zoomDir = 0;
	scrolls = 0;
	selectable = false;

	zoomDestX = 16.2f;
	zoomDestY = -7.14f;
	zoomDestZ = -25.0f + 2.414f;
	//zoomDestZ = -10.747f;
	borderR = borderR1 = 0.31f;
	borderG = borderG1 = 0.34f;
	borderB = borderB1 = 0.44f;

	borderInvalidR = borderInvalidR1 = .40f;
	borderInvalidG = borderInvalidG1 = 0.0f;
	borderInvalidB = borderInvalidB1 = 0.0f;
	borderInvalidR2 = 1.0f;
	borderInvalidG2 = 0.0f;
	borderInvalidB2 = 0.0f;

	scrollDuration = 180.0f;
	borderR2 = 0.71f;
	borderG2 = 0.78f;
	borderB2 = 1.0f;

	borderTimer = 0.0f;
	borderDir = 0;

	this->rows = rows;
	this->columns = columns;

	r = .08f;
	g = .08f;
	b = .10f;
	zoomedR = 0.0f;
	zoomedG = 0.0f;
	zoomedB = 0.0f;

	bgColor = D3DXCOLOR(r, g, b, 1.0f);
	dim = false;

	camera_distance = 0.0;

	memset(valid_chars, 0, sizeof(valid_chars));

	Init();
	stretch_to_fit = false;
}

TexturePanel::~TexturePanel(void)
{
	//for (std::vector<c_item_container*>::iterator i = item_containers.begin(); i != item_containers.end(); ++i)
	//	delete *i;
	for (auto &item : item_containers)
	{
		delete item;
	}
}


int TexturePanel::get_num_items()
{
	return item_containers.size();
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
	if (FAILED(hr))
	{
		MessageBox(NULL, "CreateInputLayout failed", 0, 0);
		PostQuitMessage(0);
		return;
	}

	SimpleVertex vertices[] =
	{
		{ D3DXVECTOR3(-4.0f / 3.0f, -1.0f, 0.0f), D3DXVECTOR2(0.0f, 1.0f) },
		{ D3DXVECTOR3(-4.0f / 3.0f, 1.0f, 0.0f), D3DXVECTOR2(0.0f, 0.0f) },
		{ D3DXVECTOR3(4.0f / 3.0f, -1.0f, 0.0f), D3DXVECTOR2(1.0f, 1.0f) },
		{ D3DXVECTOR3(4.0f / 3.0f, 1.0f, 0.0f), D3DXVECTOR2(1.0f, 0.0f) },
	};

	memcpy(vertices2, vertices, sizeof(SimpleVertex) * 4);
	vertexBuffer2 = NULL;


	bd.Usage = D3D10_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(SimpleVertex) * 4;
	bd.BindFlags = D3D10_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
	bd.MiscFlags = 0;

	build_stretch_buffer(4.0f / 3.0f);

	D3D10_SUBRESOURCE_DATA initData;
	initData.pSysMem = vertices;
	hr = d3dDev->CreateBuffer(&bd, &initData, &vertexBuffer);

	D3D10_TEXTURE2D_DESC texDesc;
	texDesc.Width = 256;
	texDesc.Height = 256;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Usage = D3D10_USAGE_DYNAMIC;
	texDesc.BindFlags = D3D10_BIND_SHADER_RESOURCE;
	texDesc.CPUAccessFlags = D3D10_CPU_ACCESS_WRITE;
	texDesc.MiscFlags = 0;

	hr = d3dDev->CreateTexture2D(&texDesc, 0, &tex);
	if (FAILED(hr))
	{
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
	if (FAILED(hr))
	{
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

void TexturePanel::OnResize()
{
	if (stretch_to_fit && state == STATE_ZOOMED)
	{
		build_stretch_buffer((float)clientWidth / (float)clientHeight);
	}
}

bool TexturePanel::Changed()
{
	if (changed)
	{
		changed = false;
		return true;
	}
	else
		return false;
}

void TexturePanel::AddItem(TexturePanelItem *item)
{
	item_containers.push_back(new c_item_container(item));

	int num_items_on_page = last_item - first_item;
	int max_items_on_page = (rows * columns) - (on_first_page ? rows : 0);

	//if (last_item - first_item < rows*columns - rows)
	if (num_items_on_page < max_items_on_page)
	{
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
	if (state == STATE_ZOOMED)
	{
		if (selected)
			itemList->push_back(item_containers[selected_item]->item);
	}
	else
		for (int i = first_item; i < last_item; i++)
			itemList->push_back(item_containers[i]->item);
}

void TexturePanel::load_items()
{
	for (int i = first_item; i < last_item; i++)
		item_containers[i]->item->Load();
	changed = true;
}

void TexturePanel::Update(double dt)
{
	if (item_containers.size() == 0)
		return;
	if (scrollRepeat)
	{
		if (scrollDir == 1)
			NextColumn();
		else
			PrevColumn();
		scrollRepeat = false;
	}
	borderTimer += dt;
	if (borderTimer >= borderDuration)
	{
		if (borderDir == 0)
		{
			borderDir = 1;
			borderR = borderR2;
			borderG = borderG2;
			borderB = borderB2;
			borderInvalidR = borderInvalidR2;
			borderInvalidG = borderInvalidG2;
			borderInvalidB = borderInvalidB2;
		}
		else if (borderDir == 1)
		{
			borderDir = 0;
			borderR = borderR1;
			borderG = borderG1;
			borderB = borderB1;
			borderInvalidR = borderInvalidR1;
			borderInvalidG = borderInvalidG1;
			borderInvalidB = borderInvalidB1;
		}

		borderTimer = 0.0f;
	}
	else
	{
		if (borderDir == 0)
		{
			borderR = InterpolateCosine(borderR1, borderR2, (float)(borderTimer / borderDuration));
			borderG = InterpolateCosine(borderG1, borderG2, (float)(borderTimer / borderDuration));
			borderB = InterpolateCosine(borderB1, borderB2, (float)(borderTimer / borderDuration));
			borderInvalidR = InterpolateCosine(borderInvalidR1, borderInvalidR2, (float)(borderTimer / borderDuration));
			borderInvalidG = InterpolateCosine(borderInvalidG1, borderInvalidG2, (float)(borderTimer / borderDuration));
			borderInvalidB = InterpolateCosine(borderInvalidB1, borderInvalidB2, (float)(borderTimer / borderDuration));
		}
		else if (borderDir == 1)
		{
			borderR = InterpolateCosine(borderR2, borderR1, (float)(borderTimer / borderDuration));
			borderG = InterpolateCosine(borderG2, borderG1, (float)(borderTimer / borderDuration));
			borderB = InterpolateCosine(borderB2, borderB1, (float)(borderTimer / borderDuration));
			borderInvalidR = InterpolateCosine(borderInvalidR2, borderInvalidR1, (float)(borderTimer / borderDuration));
			borderInvalidG = InterpolateCosine(borderInvalidG2, borderInvalidG1, (float)(borderTimer / borderDuration));
			borderInvalidB = InterpolateCosine(borderInvalidB2, borderInvalidB1, (float)(borderTimer / borderDuration));
		}
	}

	if (selected != prevSelected)
	{
		if (selected)
			item_containers[selected_item]->Select();
		else
			item_containers[selected_item]->Unselect();
		prevSelected = selected;
	}

	float scrollOffset = 0.0f;
	switch (state)
	{
	case STATE_SCROLLING:
		scrollTimer += dt;
		if (scrollTimer >= scrollDuration)
		{
			state = STATE_MENU;
			scrollTimer = 0.0f;
			if (scrollCleanup)
			{
				if (scrollDir == 1)
				{
					for (int i = 0; i < rows; ++i)
						item_containers[first_item++]->item->Deactivate();

				}
				else if (scrollDir == 0)
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
				scrollCleanup = false;
				changed = true;
			}
			if (--scrolls > 0)
				scrollRepeat = true;
		}
		else
			if (scrollDir)
				scrollOffset = InterpolateCosine(tile_width, 0.0f, (float)(scrollTimer / scrollDuration));
			else
				scrollOffset = InterpolateCosine(-tile_width, 0.0f, (float)(scrollTimer / scrollDuration));
		if (scrollCleanup && scrollDir == 1)
			scrollOffset -= tile_width;
	case STATE_MENU:
	{
		if (state != prevState)
		{
			changed = true;
			prevState = state;
		}
		int p = on_first_page ? rows : 0;

		for (int i = first_item; i < last_item; ++i, ++p)
		{
			float xx = x + scrollOffset + (p / rows) * tile_width;
			float yy = y + (p % rows) * -2.04f;

			float xxx = InterpolateLinear(xx, zoomDestX, .3f);
			float yyy = InterpolateLinear(yy, zoomDestY, .3f);
			float zzz = InterpolateLinear(0.0f, zoomDestZ, .3f);

			if (item_containers[i]->selecting)
			{
				item_containers[i]->selectTimer += dt;
				if (item_containers[i]->selectTimer >= item_containers[i]->selectDuration)
				{
					item_containers[i]->selecting = false;
					if (item_containers[i]->selectDir == 0)
					{
						item_containers[i]->selected = true;
						item_containers[i]->x = xxx;
						item_containers[i]->y = yyy;
						item_containers[i]->z = zzz;
					}
					else
					{
						item_containers[i]->selected = false;
						item_containers[i]->x = xx;
						item_containers[i]->y = yy;
						item_containers[i]->z = 0.0f;
					}
					item_containers[i]->selectTimer = 0.0f;

				}
				else
					if (item_containers[i]->selectDir == 0)
					{
						item_containers[i]->x = InterpolateCosine(xx, xxx, (float)(item_containers[i]->selectTimer / item_containers[i]->selectDuration));
						item_containers[i]->y = InterpolateCosine(yy, yyy, (float)(item_containers[i]->selectTimer / item_containers[i]->selectDuration));
						item_containers[i]->z = InterpolateCosine(0.0f, zzz, (float)(item_containers[i]->selectTimer / item_containers[i]->selectDuration));
					}
					else
					{
						item_containers[i]->x = InterpolateCosine(xxx, xx, (float)(item_containers[i]->selectTimer / item_containers[i]->selectDuration));
						item_containers[i]->y = InterpolateCosine(yyy, yy, (float)(item_containers[i]->selectTimer / item_containers[i]->selectDuration));
						item_containers[i]->z = InterpolateCosine(zzz, 0.0f, (float)(item_containers[i]->selectTimer / item_containers[i]->selectDuration));
					}
			}
			else if (item_containers[i]->selected)
			{
				item_containers[i]->x = xxx;
				item_containers[i]->y = yyy;
				item_containers[i]->z = zzz;
			}

			else
			{
				item_containers[i]->x = xx;
				item_containers[i]->y = yy;
				item_containers[i]->z = 0.0f;
			}

		}
	}
	break;
	case STATE_ZOOMING:
	{
		float zz = 0.0f;
		if (state != prevState)
		{
			if (zoomDir == 0)
			{
				zoomStartX = item_containers[selected_item]->x;
				zoomStartY = item_containers[selected_item]->y;
				zoomStartZ = item_containers[selected_item]->z;
			}
			prevState = state;
			changed = true;
		}
		zoomTimer += dt;
		if (zoomTimer >= zoomDuration)
		{
			zoomTimer = 0.0f;
			if (zoomDir == 0)
			{
				prevState = state;
				state = STATE_ZOOMED;
				if (selected)
				{
					item_containers[selected_item]->x = zoomDestX;
					item_containers[selected_item]->y = zoomDestY;
					item_containers[selected_item]->z = zoomDestZ;
				}
				bgColor = D3DXCOLOR(zoomedR, zoomedG, zoomedB, 1.0f);
				zz = 10.0f;
				if (stretch_to_fit)
				{
					build_stretch_buffer((double)clientWidth / clientHeight);
				}

			}
			else
			{
				prevState = state;
				state = STATE_MENU;
				if (selected)
				{
					item_containers[selected_item]->x = zoomStartX;
					item_containers[selected_item]->y = zoomStartY;
					item_containers[selected_item]->z = zoomStartZ;
				}
				bgColor = D3DXCOLOR(r, g, b, 1.0f);
				zz = 0.0f;
				if (stretch_to_fit)
				{
					build_stretch_buffer(4.0f / 3.0f);
				}
			}
			changed = true;

		}
		else
		{
			if (zoomDir == 0)
			{
				zz = InterpolateLinear(0.0f, 10.0f, (float)(zoomTimer / zoomDuration));
				if (selected)
				{
					item_containers[selected_item]->x = InterpolateLinear(zoomStartX, zoomDestX, (float)(zoomTimer / zoomDuration));
					item_containers[selected_item]->y = InterpolateLinear(zoomStartY, zoomDestY, (float)(zoomTimer / zoomDuration));
					item_containers[selected_item]->z = InterpolateLinear(zoomStartZ, zoomDestZ, (float)(zoomTimer / zoomDuration));
					if (stretch_to_fit)
					{
						//item_containers[selected_item]->ratio = InterpolateLinear(4.0f/3.0f, (double)clientWidth/clientHeight, (float)(zoomTimer/zoomDuration));
						build_stretch_buffer(InterpolateLinear(4.0f / 3.0f, (double)clientWidth / clientHeight, (float)(zoomTimer / zoomDuration)));
					}
				}
				float rr = InterpolateLinear(r, zoomedR, (float)(zoomTimer / zoomDuration));
				float gg = InterpolateLinear(g, zoomedG, (float)(zoomTimer / zoomDuration));
				float bb = InterpolateLinear(b, zoomedB, (float)(zoomTimer / zoomDuration));
				bgColor = D3DXCOLOR(rr, gg, bb, 1.0f);
			}
			else if (zoomDir == 1)
			{
				zz = InterpolateLinear(10.0f, 0.0f, (float)(zoomTimer / zoomDuration));
				if (selected)
				{
					item_containers[selected_item]->x = InterpolateLinear(zoomDestX, zoomStartX, (float)(zoomTimer / zoomDuration));
					item_containers[selected_item]->y = InterpolateLinear(zoomDestY, zoomStartY, (float)(zoomTimer / zoomDuration));
					item_containers[selected_item]->z = InterpolateLinear(zoomDestZ, zoomStartZ, (float)(zoomTimer / zoomDuration));
					if (stretch_to_fit)
					{
						//item_containers[selected_item]->ratio = InterpolateLinear((double)clientWidth/clientHeight, 4.0f/3.0f, (float)(zoomTimer/zoomDuration));
						build_stretch_buffer(InterpolateLinear((double)clientWidth / clientHeight, 4.0f / 3.0f, (float)(zoomTimer / zoomDuration)));
					}
				}
				float rr = InterpolateLinear(zoomedR, r, (float)(zoomTimer / zoomDuration));
				float gg = InterpolateLinear(zoomedG, g, (float)(zoomTimer / zoomDuration));
				float bb = InterpolateLinear(zoomedB, b, (float)(zoomTimer / zoomDuration));
				bgColor = D3DXCOLOR(rr, gg, bb, 1.0f);
			}
		}
		for (int i = first_item; i < last_item; ++i)
		{
			if (!(selected && i == selected_item))
				item_containers[i]->z = zz;
		}
	}
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
		for (int i = first_item; i < last_item; ++i)
		{
			c_item_container *p = item_containers[i];
			if (selected && i == selected_item)
			{

				//if (state != STATE_ZOOMING)
				//	varBorder->SetBool(true);
				//else
				//	varBorder->SetBool(false);
				if (p->item->Selectable())
				{
					D3DXVECTOR3 border_color(borderR, borderG, borderB);
					var_border_color->SetFloatVector((float*)&border_color);
				}
				else
				{
					D3DXVECTOR3 border_color(borderInvalidR, borderInvalidG, borderInvalidB);
					var_border_color->SetFloatVector((float*)&border_color);
				}

				DrawItem(p, state != STATE_ZOOMING, p->x, p->y, p->z, 1.0f);
			}
			else
			{
				//varBorder->SetBool(false);
				DrawItem(p, 0, p->x, p->y, p->z, .45f);
			}
		}
		break;
	case STATE_ZOOMED:
		if (selected)
		{
			c_item_container *p = item_containers[selected_item];
			//varBorder->SetBool(false);
			//if (stretch_to_fit)
			//	p->ratio = (double)clientWidth/clientHeight;
			DrawItem(p, 0, p->x, p->y, p->z);
		}
		break;
	}

}

void TexturePanel::build_stretch_buffer(float ratio)
{
	if (vertexBuffer2)
	{
		vertexBuffer2->Release();
		vertexBuffer2 = NULL;
	}
	vertices2[0].pos.x = -ratio;
	vertices2[1].pos.x = -ratio;
	vertices2[2].pos.x = ratio;
	vertices2[3].pos.x = ratio;

	D3D10_SUBRESOURCE_DATA initData;
	initData.pSysMem = vertices2;
	hr = d3dDev->CreateBuffer(&bd, &initData, &vertexBuffer2);
}

void TexturePanel::DrawItem(c_item_container *item, int draw_border, float x, float y, float z, float c)
{
	UINT stride = sizeof(SimpleVertex);
	UINT offset = 0;
	d3dDev->IASetInputLayout(vertexLayout);
	//item->item->build_stretch_buffer((double)clientWidth / clientHeight);
	ID3D10Buffer *b;
	if (false && stretch_to_fit && item->selected)
	{
		double ratio = (double)clientWidth / clientHeight;
		if (state == STATE_ZOOMING)
		{
			ratio = InterpolateLinear((double)clientWidth / clientHeight, 4.0f / 3.0f, (float)(zoomTimer / zoomDuration));
		}
		item->item->build_stretch_buffer(ratio);
		b = item->item->get_vertex_buffer(1);
	}
	else
	{
		b = item->item->get_vertex_buffer(0);
	}
	
	d3dDev->IASetVertexBuffers(0, 1, &b, &stride, &offset);
	d3dDev->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);


	if (dim)
	{
		//c *= .33f;
		c *= .09f;
	}

	varColor->SetFloat(c);

	//double yy = (camera_distance - zoomDestZ)/(camera_distance - z) *
	//	clientHeight;
	//double xx = yy * (4.0/3.0);
	//D3DXVECTOR2 o(xx, yy); 
	//var_output_size->SetFloatVector((float*)&o);

	D3D10_VIEWPORT vp;
	UINT num_vp = 1;
	d3dDev->RSGetViewports(&num_vp, &vp);
	D3DXVECTOR3 topleft, bottomright;
	D3DXMATRIX identity;
	D3DXMatrixIdentity(&identity);
	D3DXVec3Project(&topleft, &D3DXVECTOR3(x - 4.0 / 3.0, y - 1.0, z), &vp, &matrixProj, &matrixView, &identity);
	D3DXVec3Project(&bottomright, &D3DXVECTOR3(x + 4.0 / 3.0, y + 1.0, z), &vp, &matrixProj, &matrixView, &identity);
	double quad_width = bottomright.x - topleft.x;
	double quad_height = topleft.y - bottomright.y;
	var_output_size->SetFloatVector((float*)&D3DXVECTOR2(quad_width, quad_height));
	//D3DXCOLOR oc = item->item->get_overscan_color();
	//var_overscan_color->SetFloatVector((float*)&item->item->get_overscan_color());
    var_max_y->SetFloat((double)item->item->get_height());
	var_max_x->SetFloat((double)item->item->get_width());

	item->item->DrawToTexture(tex);
	D3DXMatrixTranslation(&matrixWorld, x, y, z);
	varWorld->SetMatrix((float*)&matrixWorld);
	technique->GetPassByIndex(0)->Apply(0);
	d3dDev->Draw(4, 0);
	if (draw_border)
	{
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

			selected = false;
			next->selected = true;
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
			selected = false;
			prev->selected = true;
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
			if (scrollCleanup)
			{
				for (int i = 0; i < rows; ++i)
					item_containers[first_item++]->item->Deactivate();
				scrollCleanup = 0;
			}
		}
	}
	else if (column_move < 0)
	{
		for (int i = 0; i < column_move * -1; i++)
		{
			PrevColumn(false, true);
			if (scrollCleanup)
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
	if (scrollDir == 0)
	{
		scrolls = 0;
		//state = STATE_MENU;
	}
	if (scrollDir == 1 && state == STATE_SCROLLING && scrolls < 2)
		scrolls++;
	if (!adjusting && state != STATE_MENU)
		return;
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
			scrollDir = 1;
			scrollCleanup = true;
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
		scrollDir = 1;
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
	if (scrollDir == 1)
	{
		scrolls = 0;
		//state = STATE_MENU;
	}
	if (scrollDir == 0 && state == STATE_SCROLLING && scrolls < 2)
		scrolls++;
	if (!adjusting && state != STATE_MENU)
		return;
	if (scrolls)
		scrolls--;
	int change = selected_item;
	if (selected_item >= rows)
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
			scrollDir = 0;
			scrollCleanup = true;
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
		scrollDir = 0;
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
		zoomDir = 1;
	else
		zoomDir = 0;
	prevState = state;
	state = STATE_ZOOMING;
}

float TexturePanel::InterpolateLinear(float start, float end, float mu)
{
	if (mu > 1.0f)
		mu = 1.0f;
	return start*(1 - mu) + end*mu;
}

float TexturePanel::InterpolateCosine(float start, float end, float mu)
{
	if (mu > 1.0f)
		mu = 1.0f;
	float mu2 = (1 - cos(mu*(float)D3DX_PI)) / 2.0f;
	return InterpolateLinear(start, end, mu2);
}

void TexturePanel::Suspend()
{
	prevState = state;
	state = STATE_SUSPENDED;
}

void TexturePanel::Resume()
{
	state = prevState;
}

TexturePanelItem* TexturePanel::GetSelected()
{
	return item_containers[selected_item]->item;
}