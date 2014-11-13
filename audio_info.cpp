#include "audio_info.h"
#include "effect2.fxo.h"

extern ID3D10Device *d3dDev;
extern D3DXMATRIX matrixView;
extern D3DXMATRIX matrixProj;


c_audio_info::c_audio_info()
{
	count = 0;
	read_pointer = 0;
	for (int i = 0; i < tex_size; i++)
	{
		freq[i] = -1;
		buf[i] = -1;
		max_freq[i] = -1;
		min_freq[i] = -1;
	}
	stable = 0;
	x = y = z = 0.0f;
}

c_audio_info::~c_audio_info()
{
}

void c_audio_info::report(int freq, int buf, int stable, int max_freq, int min_freq)
{
	this->stable = stable;
	this->freq[count] = freq;
	this->buf[count] = buf;
	this->max_freq[count] = max_freq;
	this->min_freq[count] = min_freq;
	read_pointer = count;
	count++;
	if (count == tex_size)
		count = 0;
}

void c_audio_info::init(void *params)
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

	SimpleVertex vertices[] = 
	{
		{ D3DXVECTOR3( -1.0f, -.3f, 0.0f ), D3DXVECTOR2(0.0f, 1.0f) },
		{ D3DXVECTOR3( -1.0f, .3f, 0.0f ), D3DXVECTOR2(0.0f, 0.0f) },
		{ D3DXVECTOR3( 1.0f, -.3f, 0.0f ), D3DXVECTOR2(1.0f, 1.0f) },
		{ D3DXVECTOR3( 1.0f, .3f, 0.0f ), D3DXVECTOR2(1.0f, 0.0f) },
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

void c_audio_info::resize()
{
}

int c_audio_info::update(double dt, int child_result, void *params)
{
	return child_result;
}

void c_audio_info::draw()
{
	var_proj->SetMatrix((float*)&matrixProj);
	var_view->SetMatrix((float*)&matrixView);

	UINT stride = sizeof(SimpleVertex);
	UINT offset = 0;
	d3dDev->IASetInputLayout(vertex_layout);
	d3dDev->IASetVertexBuffers(0, 1, &vertex_buffer, &stride, &offset);
	d3dDev->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	D3D10_MAPPED_TEXTURE2D map;
	tex->Map(0, D3D10_MAP_WRITE_DISCARD, NULL, &map);
	int *p = (int*)map.pData;



	for (int y = 0; y < tex_size; y++)
	{
		for (int x = 0; x < tex_size; x++)
		{
			int range = 10000-0;
			int t = 1600*3;
			int min = (int)((tex_size - 1) - (((range - (10000 - (t-1600)))/(double)range) * (tex_size - 1)));
			int max = (int)((tex_size - 1) - (((range - (10000 - (t+1600)))/(double)range) * (tex_size - 1)));
			int target = (int)((tex_size - 1) - (((range - (10000 - t))/(double)range) * (tex_size - 1)));


			if (y <= min && y >= max)
			{
				int trans = (1.0 - ((double)abs(y - target) / ((min-max)/2))) * 0xFF;
				//trans <<= 24;

				if (stable)
					*p++ = 0x99009900 | trans;
				else
					*p++ = 0x99000000 | trans;
			}
			else
				*p++ = 0x99000000;
		}
	}


	for (int i = 0; i < tex_size; i++)
	{
		if (freq[read_pointer] != -1)
		{
			int range = (48000*1.02)-(48000*.98);
			int y = (int)(((range - ((48000*1.02) - freq[read_pointer]))/(double)range) * (tex_size - 1));
			y = ((tex_size - 1) - y);
			if (y > (tex_size - 1))
				y = (tex_size - 1);
			else if (y < 0)
				y = 0;

			int *pp = (int*)map.pData;
			*(pp + (y*tex_size) + i) = 0xFF00FF00;
		}

		int range = 10000-0;
		if (buf[read_pointer] != -1)
		{
			int y = (int)(((range - (10000 - buf[read_pointer]))/(double)range) * (tex_size - 1));
			y = ((tex_size - 1) - y);

			if (y > (tex_size - 1))
				y = (tex_size - 1);
			else if (y < 0)
				y = 0;

			int *pp = (int*)map.pData;
			*(pp + (y*tex_size) + i) = 0xFF0000FF;
		}

		read_pointer = (read_pointer + 1) % tex_size;
	}

	tex->Unmap(0);

	D3DXMatrixTranslation(&matrix_world, x, y, z);
	var_world->SetMatrix((float*)&matrix_world);
	technique->GetPassByIndex(0)->Apply(0);
	d3dDev->Draw(4, 0);
}