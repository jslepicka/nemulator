#include "mem_viewer.h"
#include "mem_access_log.h"
#include "nes_input_handler.h"
#include "effect2.fxo.h"
extern ID3D10Device *d3dDev;
extern D3DXMATRIX matrixView;
extern D3DXMATRIX matrixProj;

extern c_input_handler *g_ih;

c_mem_viewer::c_mem_viewer()
{
	x = y = z = 0.0f;
	size = 1;
	mem_access_log = NULL;
	selected_location = 0;
}

c_mem_viewer::~c_mem_viewer()
{
}

void c_mem_viewer::set_mem_access_log(c_mem_access_log *log)
{
	mem_access_log = log;
}

void c_mem_viewer::init(void *params)
{
	HRESULT hr;
	D3D10_INPUT_ELEMENT_DESC elementDesc[] =
	{
		{ TEXT("POSITION"), 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D10_INPUT_PER_VERTEX_DATA, 0 },
		{ TEXT("TEXCOORD"), 0, DXGI_FORMAT_R32G32_FLOAT,    0, 12, D3D10_INPUT_PER_VERTEX_DATA, 0 },
	};
	UINT numElements = sizeof(elementDesc)/sizeof(elementDesc[0]);
	hr = D3DX10CreateEffectFromMemory((LPCVOID)effect2, sizeof(effect2), "effect1", NULL, NULL, "fx_4_0", 0, 0, d3dDev, NULL, NULL, &effect, NULL, NULL);
	if (FAILED(hr))
	{
		MessageBox(NULL, "D3DX10CreateEffectFromFile failed", 0, 0);
		PostQuitMessage(0);
		return;
	}
	technique = effect->GetTechniqueByName("Render");
	var_tex = effect->GetVariableByName("txDiffuse")->AsShaderResource();
	var_world = effect->GetVariableByName("World")->AsMatrix();
	var_proj = effect->GetVariableByName("Projection")->AsMatrix();
	var_view = effect->GetVariableByName("View")->AsMatrix();

	D3D10_PASS_DESC passDesc;
	technique->GetPassByIndex(0)->GetDesc(&passDesc);

	hr = d3dDev->CreateInputLayout(elementDesc, numElements, passDesc.pIAInputSignature, passDesc.IAInputSignatureSize, &vertex_layout);
	if (FAILED(hr))
	{
		MessageBox(NULL, "CreateInputLayout failed", 0, 0);
		PostQuitMessage(0);
		return;
	}

	const float scale = .5;

	SimpleVertex vertices[] = 
	{
		{ D3DXVECTOR3( -1.0f * scale, -1.0f * scale, 0.0f ), D3DXVECTOR2(0.0f, 1.0f) },
		{ D3DXVECTOR3( -1.0f * scale,  1.0f * scale, 0.0f ), D3DXVECTOR2(0.0f, 0.0f) },
		{ D3DXVECTOR3(  1.0f * scale, -1.0f * scale, 0.0f ), D3DXVECTOR2(1.0f, 1.0f) },
		{ D3DXVECTOR3(  1.0f * scale,  1.0f * scale, 0.0f ), D3DXVECTOR2(1.0f, 0.0f) },
	};

	D3D10_BUFFER_DESC bd;
	bd.Usage = D3D10_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(SimpleVertex) * 4;
	bd.BindFlags = D3D10_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
	bd.MiscFlags = 0;
	D3D10_SUBRESOURCE_DATA initData;
	initData.pSysMem = vertices;
	hr = d3dDev->CreateBuffer(&bd, &initData, &vertex_buffer);
	if (FAILED(hr))
	{
		MessageBox(NULL, "CreateBuffer failed", 0, 0);
		PostQuitMessage(0);
		return;
	}

	D3D10_TEXTURE2D_DESC texDesc;
	texDesc.Width = tex_size;
	texDesc.Height = tex_size;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Usage = D3D10_USAGE_DYNAMIC;
	//texDesc.Usage = D3D10_USAGE_DEFAULT;
	texDesc.BindFlags = D3D10_BIND_SHADER_RESOURCE;
	texDesc.CPUAccessFlags = D3D10_CPU_ACCESS_WRITE;
	//texDesc.CPUAccessFlags = 0;
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
	hr = d3dDev->CreateShaderResourceView(tex, &descRv, &tex_rv);
	if (FAILED(hr))
	{
		MessageBox(NULL, "CreateShaderResourceView failed", 0, 0);
		PostQuitMessage(0);
		return;
	}

	D3DXMatrixIdentity(&matrix_world);
	var_tex->SetResource(tex_rv);

}

int c_mem_viewer::update(double dt, int child_result, void *params)
{
	int result_mask = c_input_handler::RESULT_DOWN | c_input_handler::RESULT_REPEAT_SLOW | c_input_handler::RESULT_REPEAT_FAST | c_input_handler::RESULT_REPEAT_EXTRAFAST;
	if (mem_access_log)
	{
		if (g_ih->get_result(BUTTON_1DOWN, true) & result_mask)
		{
			selected_location += 256;
			g_ih->ack();
		}
		else if (g_ih->get_result(BUTTON_1UP, true) & result_mask)
		{
			selected_location -= 256;
			g_ih->ack();
		}
		else if (g_ih->get_result(BUTTON_1LEFT, true) & result_mask)
		{
			selected_location -= 1;
			g_ih->ack();
		}
		else if (g_ih->get_result(BUTTON_1RIGHT, true) & result_mask)
		{
			selected_location +=1;
			g_ih->ack();
		}

		if (selected_location < 0)
			selected_location = 0;
		else if (selected_location > 65535)
			selected_location = 65535;

		D3D10_MAPPED_TEXTURE2D map;
		tex->Map(0, D3D10_MAP_WRITE_DISCARD, NULL, &map);
		int *p = (int*)map.pData;
		int row = 0;
		int last_row = -1;
		int last_i = -1;
		for (int y = 0; y < tex_size; y++)
		{
			row = y/size;
			for (int x = 0; x < tex_size; x++)
			{
				int i = x/size + row*256;
				if (mem_access_log[i].timestamp > 0.0)
				{
					unsigned char color = (mem_access_log[i].timestamp / 1000.0) * 0xFF;
					*p++ = 0x99000000 | (color << (8 * mem_access_log[i].type)) | ( i == selected_location ? 0x00FF0000 : 0); 
					if (row != last_row && last_i != i)
					{
						mem_access_log[i].timestamp -= dt;
					}
				}
				else
					*p++ = 0x99000000  | ( i == selected_location ? 0x00FF0000 : 0);
				last_i = i;
			}
			last_row = row;
		}

		//for (int i = 0; i < tex_size*tex_size; i++)
		//{
		//	int mem_index = 
		//	if (mem_access_log[i].timestamp > 0.0)
		//	{
		//		unsigned char color = (mem_access_log[i].timestamp / 1000.0) * 0xFF;
		//		*p++ = 0x99000000 | (color << (8 * mem_access_log[i].type));
		//		mem_access_log[i].timestamp -= dt;
		//	}
		//	else
		//	{
		//		*p++ = 0x99000000;
		//	}
		//}
		tex->Unmap(0);
	}
	return child_result;
}

void c_mem_viewer::draw()
{
	if (mem_access_log)
	{
		var_proj->SetMatrix((float*)&matrixProj);
		var_view->SetMatrix((float*)&matrixView);

		UINT stride = sizeof(SimpleVertex);
		UINT offset = 0;
		d3dDev->IASetInputLayout(vertex_layout);
		d3dDev->IASetVertexBuffers(0, 1, &vertex_buffer, &stride, &offset);
		d3dDev->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

		D3DXMatrixTranslation(&matrix_world, x, y, z);
		var_world->SetMatrix((float*)&matrix_world);
		technique->GetPassByIndex(0)->Apply(0);
		d3dDev->Draw(4, 0);
	}
}

float c_mem_viewer::InterpolateLinear(float start, float end, float mu)
{
	if (mu > 1.0f)
		mu = 1.0f;
	return start*(1-mu) + end*mu;
}